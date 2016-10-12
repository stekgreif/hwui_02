/******************************************************************************/
/** @date		2016-10-12
	@brief		4 erps and 1 joystick
*******************************************************************************/
#include <stdio.h>

uint16_t adc_pins[10] = {};
uint16_t adc_values[10] = {};

char print_buffer[64] = {};


void setup()
{
	Serial.begin(9600);
	
	adc_pins[0] = 0;
  adc_pins[1] = 1;
  adc_pins[2] = 2;
  adc_pins[3] = 3;
  adc_pins[4] = 4;
  adc_pins[5] = 5;
  adc_pins[6] = 6;
  adc_pins[7] = 7;
  adc_pins[8] = 8; 
  adc_pins[9] = 9; 
}


void loop()
{
    adc_values[0] = analogRead(adc_pins[0]);
    adc_values[1] = analogRead(adc_pins[1]);
		adc_values[2] = analogRead(adc_pins[2]);
    adc_values[3] = analogRead(adc_pins[3]);
    adc_values[4] = analogRead(adc_pins[4]);
    adc_values[5] = analogRead(adc_pins[5]);    
    adc_values[6] = analogRead(adc_pins[6]);
    adc_values[7] = analogRead(adc_pins[7]);
    adc_values[8] = analogRead(adc_pins[8]);
    adc_values[9] = analogRead(adc_pins[9]);


  	sprintf(print_buffer, "%04d %04d  %04d %04d  %04d %04d  %04d %04d  %04d %04d",
  	        adc_values[0], adc_values[1], 
            adc_values[2], adc_values[3], 
            adc_values[4], adc_values[5], 
            adc_values[6], adc_values[7], 
            adc_values[8], adc_values[9]);
  	Serial.println(print_buffer);

    delay(50);
}






/******************************************************************************/
/**	@brief	turns the two ad-values from an erp to a value/position
 	 	 	between 0..2047

	@param x analog outputs A of the erp (10 bit value)
	@param y analog outputs B of the erp (10 bit value)
	@return calculated position (0..2047)
*******************************************************************************/
static uint16_t GetPosition(uint16_t x, uint16_t y)
{
	/* variables for temporary calculation*/
	uint32_t x_c;
	uint32_t y_c;

	uint32_t w = 0;	/* output value */
	uint32_t a;	/* fading coefficient */

	/* B1 */
	if ( ((384<y) && (y<=640)) && (x>768) )
	{
		w = y-256;
	}

	/* B12 */
	else if ( ((640<y) && (y<=896)) && (x>512) )
	{
		a = y-640;					/* calculate coefficient */
		x_c = 1280-x; 				/* mirror and shift x */
		y_c = y-256;				/* shift y */
		w = x_c*a + y_c*(255-a);	/* fading */
		w = w >> 8;					/* normalize result */
	}

	/* B2 */
	else if ( (896<y) && ((256<x) && (x<768)) )
	{
		w = 1280-x;
	}

	/* B23 */
	else if ( ((640<y) && (y<=896)) && (x<512) )
	{
		a = 896-y;						/* calculate coefficient */
		x_c = 1280-x;					/* mirror and shift x */
		y_c = 1792-y;					/* mirror y*/
		w = (y_c*a) + (x_c*(255-a));	/* fading */
		w = w >> 8;						/* normalize result */
	}

	/* B3 */
	else if ( ((384<y) && (y<=640)) && (x<256) )
	{
		w = 1790-y;
	}

	/* B34 */
	else if ( ((128<y) && (y<=384)) && (x<512) )
	{
		a = 384-y;					/* calculate coefficient */
		x_c = 1280+x;				/* shift x */
		y_c = 1792-y;				/* mirror and shift y*/
		w = x_c*a + y_c*(255-a);	/* fading */
		w = w >> 8;					/* normalize result  w = w + 1280; */
	}

	/* B4 */
	else if ( (y<=128) && ((256<x) && (x<768)) )
	{
		w = 1280+x;
	}

	/* B41 */
	else if ( ((128<y) && (y<=384)) && (x>512) )
	{
		a = y-128;					/* calculate coefficient */
		x_c = 1280+x;				/* shift x */
		y_c = 1792+y;				/* mirror and shift y*/
		w = y_c*a + x_c*(255-a);	/* fading */
		w = w >> 8;					/* normalize result */

		if (w > 2047)
		{
			w = w - 2047;
		}
	}

	return (uint16_t) w;
}

