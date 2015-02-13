/*
 *	A program that measures distance with the use of an
 *	SRF05 Ultrasonic Range Finder and the DE0 FPGA development board.
 *	The result of the distance is calculated and displayed on the
 *	7 Segment display of the DE0 FPGA development board.
 *
 *	The program also stores the values of the maximum and minimum distances and resets them when triggered
 *
 *	Development Environment: Altera NIOS II software on Quartus II version 13.0 (Web Edition)
 *	Hardware: DE0 FPGA Development Board, SRF05 Ultrasonic Rangefinder
 *
 *	@author Emmanuel Ogbodo <oe436@greenwich.ac.uk>
 *	@copyright 2015
 *	@license GNU GENERAL PUBLIC LICENSE version 2
 *	@version 1.0.0
 */

#include "altera_avalon_pio_regs.h"  //for the I/O functions
#include "sys/alt_timestamp.h"  //For timer function that calculates the number of cycles spent
#include "system.h"
#include "unistd.h" //for the usleep() function
#include <math.h> //for the roundf() function


#define setHeaderOuts HEADEROUTPUTS_BASE+0x10  	//HEADEROUTPUTS_BASE is defined in system.h of the _bsp file.  It refers to the base address in the Qsys design
												//the hex offset (in this case 0x10, which is 16 in decimal) gives the number of bytes of offset
												//each register is 32 bits, or 4 bytes
												//so to shift to register 4, which is the outset register, we need 4 * (4 bytes) = 16 bytes
#define clearHeaderOuts HEADEROUTPUTS_BASE+0x14 //to shift to register 5 (the 'outclear' register) we need to shift by 5 * (4 bytes) = 20 bytes, (=0x14 bytes)
												// offset of 5 corresponds to the 'outclear' register of the PIO.

float get_distance();
unsigned int hex_encoder(float cmDistance);
unsigned int format_hex(int digit);


int  main(void){

	//Initialise globally accessible variables
 	unsigned int buttons = 0, switches = 0; //the buttons and switches on the DE0 FPGA board
 	float cmDistance = 0.0; //Centimetre distance
 	float max_cmDistance = 0; //maximum centimetre distance
 	float min_cmDistance = 400; //minimum centimetre distance. Default value is the maximum range of the SRF05 Ultrasonic rangefinder
 	int display_SSG; //data to display on the 7 segment display

	while(1){
		cmDistance = get_distance();

		display_SSG = hex_encoder(cmDistance);
		IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,display_SSG);

		//*******************toggle storage and reset mode****************************************************************

		switches=IORD_ALTERA_AVALON_PIO_DATA(DE0SWITCHES_BASE); //read the value of the switches. 1 when on, 0 when off
		if(switches > 0){//a switch is on

			if(switches == 1){//Storage mode is activated
				if(cmDistance > max_cmDistance){
					max_cmDistance = cmDistance;

				}else if(cmDistance < min_cmDistance){
					min_cmDistance = cmDistance;
				}
			}else if(switches == 2){//reset maximum and minimum centimetre distances
				max_cmDistance = 0;
				min_cmDistance = 400;
			}
		}

		//***********************Process buttons to display stored distances**********************************************

		buttons = IORD_ALTERA_AVALON_PIO_DATA(PUSHBUTTONS1_2_BASE);//read the value of the pushbuttons.
        //button 2 gives decimal value of 1 and button 1 gives a decimal value of 2

        while(buttons == 1){//while pushbutton 2 is pressed.
        	//display minimum centimetre distance
        	display_SSG = hex_encoder(min_cmDistance);
        	IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,display_SSG);

        	buttons = IORD_ALTERA_AVALON_PIO_DATA(PUSHBUTTONS1_2_BASE);
        }

        while(buttons == 2){//while pushbutton 1 is pressed.
			//display maximum centimetre distance
			display_SSG = hex_encoder(max_cmDistance);
			IOWR_ALTERA_AVALON_PIO_DATA(SSEG_BASE,display_SSG);

			buttons = IORD_ALTERA_AVALON_PIO_DATA(PUSHBUTTONS1_2_BASE);
		}
	}
}

/*
 * Function to initiate the ultrasonic beam, calculate the distance
 * and return it in centimetre
 *
 * @return float
 */
float get_distance(){
	unsigned int cycles = 0; //timer counter
	char input=0;//returned input from the ultrasonic range finder


	IOWR_ALTERA_AVALON_PIO_DATA(clearHeaderOuts,0x01); //turn off the output pin

	usleep(50000); //Wait 50ms before every trigger. This is to ensure the previous ultrasonic beam has faded away and will not cause a false echo on the next reading.


	//************************** Trigger ultrasonic beam *****************************************************************************

	IOWR_ALTERA_AVALON_PIO_DATA(setHeaderOuts,0x01);	//turn on the output pin to trigger an Ultrasonic beam

	cycles = alt_timestamp_start(); //start the timer. Timer increments each clock cycle.

	do{
		cycles = alt_timestamp();
	}while( cycles < 501 );// read the timer until it comes to 500 (10us) being the time taken for the SRF05 to trigger the ultrasonic beam

	IOWR_ALTERA_AVALON_PIO_DATA(clearHeaderOuts,0x01); //Stop the trigger pulse when it gets to 10us

	cycles = 0; //reset timer


	//***********************************************************************************************************************************

	do{
		input=IORD_ALTERA_AVALON_PIO_DATA(HEADERINPUTS_BASE);
	}while(((input)&(0x1)) == 0 ); //wait before taking cycle reading until until there is no echo. This prevents reading a false echo immediately it is sent


	//************************** Read the total cycles taken to retrieve echo ************************************************************

	cycles = alt_timestamp_start(); // start the timer in the beginning of the echo signal.

	do{
		input=IORD_ALTERA_AVALON_PIO_DATA(HEADERINPUTS_BASE);
	}while(((input)&(0x1)) == 1 ); //keep checking until there is an echo

	cycles = alt_timestamp(); //read the timer at the end of the echo signal.

	//***********************************************************************************************************************************


	/*
	 * As speed of sound is 340.29 meters per second which is equal to 0.034029 centimetres per microsecond
	 * And the DE0 board is 50 cycles per microsecond
	 * The time taken to receive echo immediately after it is sent = count/50
	 *
	 * And the centimetre reading of the time taken for the echo to reach the object is = (0.034029 * (count/50))/2
	 * It is divided by 2 because the actual distance is only the time taken for the echo to reach the object
	 *
	 * Therefore, the final formula for calculating the distance in centimetre is
	 * cmDistance = 0.00034029 * count
	 */
	return roundf((0.00034029 * cycles)*10.0f)/10.0f; //calculate the centimetre value, rounding it up to 1 decimal point
}

/*
 * Function to encode the distances and convert them to hexadecimal values for display
 *
 * @return int
 */
unsigned int hex_encoder(float cmDistance){

    //seven segment display digits from right to left
	int ssd0,ssd1,ssd2,ssd3;

    int mmDistance = cmDistance * 10;//Calculate distance in millimetre
    int icmDistance = (int)cmDistance; //take the integer value of the centimetre distance

	//Limit the centimetre display to 9
	if(icmDistance > 9){
		ssd0 = 191;//display a minus sign if the centimetre distance is above 9
	}else{
		ssd0 = ((icmDistance)-((icmDistance/10)*10));
	}

	ssd1 = ((mmDistance)-((mmDistance/10)*10));
	ssd2 = ((mmDistance/10)-((mmDistance/100)*10));
	ssd3 = ((mmDistance/100)-((mmDistance/1000)*10));


	//convert to hexadecimal characters
	ssd0 = format_hex(ssd0) ;
	ssd1 = format_hex(ssd1) ;
	ssd2 = format_hex(ssd2) ;
	ssd3 = format_hex(ssd3) ;

	//shift to left displays
	ssd1 = ssd1 << 8;
	ssd2 = ssd2 << 16;
	ssd3 = ssd3 << 24;

    return ssd0 + ssd1 + ssd2 + ssd3;
}

/*
 * Function to exchange integer digits to hexadecimal values
 *
 * @return int
 */
unsigned int format_hex(int digit){
    unsigned int led ;
    switch(digit)
    {
    case 0:
        led = 0xc0 ;
        break;

    case 1:
        led = 0xf9 ;
        break;

    case 2:
        led = 0xa4 ;
        break;

    case 3:
        led = 0xb0 ;
        break;

    case 4:
        led = 0x99 ;
        break;

    case 5:
        led = 0x92 ;
        break;

    case 6:
        led = 0x82 ;
        break;

    case 7:
        led = 0xf8 ;
        break;

    case 8:
        led = 0x80 ;
        break;

    case 9:
        led = 0x90 ;
        break;

    case 191:
           led = 0xBF ;
           break;

    default:
        led = 0xff ;
        break;
    }

    return led ;
}
