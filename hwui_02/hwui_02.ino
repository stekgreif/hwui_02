/******************************************************************************/
/** @date		2016-10-18
	@author	dan@stekgreif.com
	@brief		4 erps and 1 joystick with tactile button
*******************************************************************************/
#include <stdio.h>
#include "MIDIUSB.h"

#define _ID	0x02

uint32_t cur_tick = 0;
uint32_t prev_tick = 0;
uint8_t  sched_cnt  = 0;
uint8_t  usb_midi_msg_cnt = 0;

uint16_t adc_pins[10] = {};
uint16_t adc_values[10] = {};

uint16_t erp_old_pos[4] = {};
uint16_t erp_new_pos[4] = {};
int16_t  erp_diff[4] = {};

uint8_t joystick_old[2] = {};

uint8_t btn_pin = 12;

char print_buffer[64] = {};



/******************************************************************************/
/** @brief		init serial, usb, gpios
*******************************************************************************/
void setup()
{
	Serial.begin(115200);
  
  analogReference(EXTERNAL);
  
	for(int i = 0; i < 10; i++)
	{
		adc_pins[i] = i;
	}

	pinMode(btn_pin, INPUT_PULLUP); 
}



/******************************************************************************/
/** @brief	functions to be called from state machine
*******************************************************************************/
void print_buffer_clean_up(void)
{
	for( int i = 0; i < 64; i++ )
	{
		print_buffer[i] = 0;
	}
}

void adc_read_values(void)
{
	for( int i = 0; i < 10; i++ )
	{
		adc_values[i] = analogRead(adc_pins[i]);
	}
}

void adc_print_values(void)
{
	print_buffer_clean_up();
	sprintf(print_buffer, "%04d %04d  %04d %04d  %04d %04d  %04d %04d  %04d %04d %04d",
	        adc_values[0], adc_values[1], adc_values[2], adc_values[3], adc_values[4], 
	        adc_values[5], adc_values[6], adc_values[7], adc_values[8], adc_values[9], digitalRead(btn_pin) );
	Serial.println(print_buffer);
}

void erp_get_positions(void)
{
	for( int i = 0; i < 4; i++ )
	{	
		erp_new_pos[i] = erp_get_position( adc_values[ i*2 ], adc_values[ (i*2)+1 ] );
		erp_new_pos[i] = erp_new_pos[i] >> 5; // reduce jitter by reducing bits
	}
}

void erp_print_positions(void)
{
	print_buffer_clean_up();
	sprintf(print_buffer, "%04d %04d %04d %04d", erp_new_pos[0], erp_new_pos[1], erp_new_pos[2], erp_new_pos[3] );
	Serial.println(print_buffer);
}

void erp_send_diff(void)
{
	for (uint8_t id = 0; id < 4; id++)
	{
		if (erp_old_pos[id] != erp_new_pos[id])
		{
			erp_diff[id] = erp_old_pos[id] - erp_new_pos[id];

      if(erp_diff[id] <= -50)
      {
        erp_diff[id] = erp_diff[id] + 64;  // erp wrap around
      }
      if(erp_diff[id] >= 50)
      {
        erp_diff[id] = erp_diff[id] - 64;  // erp wrap around
      }
      
			erp_old_pos[id] = erp_new_pos[id];

			erp_diff[id] = erp_diff[id] + 64; // midi offset for incremental mode
			
			if( erp_diff[id] < 0 )
				erp_diff[id] = 0;
			else if ( erp_diff[id] > 127 )
				erp_diff[id] = 127;

#if 0 // for debugging			
			print_buffer_clean_up();
			sprintf(print_buffer, "%02d %04d", id, erp_new_pos[id]);
			Serial.println(print_buffer);
#endif
			
#if 1 // send via USB
			midiEventPacket_t event2 = {0x0B, 0xB0, id, (uint8_t)erp_diff[id]};
			MidiUSB.sendMIDI(event2);
			usb_midi_msg_cnt++;
#endif
		}
	}
}



void joystick_send(void)
{
  uint8_t temp_joy_0 = adc_values[8] >> 5;
  uint8_t temp_joy_1 = adc_values[9] >> 5;
  
  if( temp_joy_0 != joystick_old[0] )
  {
      midiEventPacket_t event = {0x0B, 0xB0, 0x10, adc_values[8]};
      MidiUSB.sendMIDI(event);
      usb_midi_msg_cnt++;
      joystick_old[0] = temp_joy_0;
  }
  
  if( temp_joy_1 != joystick_old[1] )
  {
      midiEventPacket_t event = {0x0B, 0xB0, 0x11, adc_values[9]};
      MidiUSB.sendMIDI(event);
      usb_midi_msg_cnt++;
      joystick_old[1] = temp_joy_1;
  }
}



void usb_midi_read()
{
  midiEventPacket_t rx;
  rx = MidiUSB.read();

  if (rx.header == 0x0A) // hwui identifier message
  {
    if ( rx.byte2 == 0x01)
    {
      midiEventPacket_t event = {0x0A, 0xA0, 0, _ID};
      MidiUSB.sendMIDI(event);
      usb_midi_msg_cnt++;
    }
  }
}


/******************************************************************************/
/** @brief		state machine
*******************************************************************************/
void loop()
{
	cur_tick = millis();

	//if( Serial.available() ) serialEvent();   //Atmega32U4: UART RX polling

	if( cur_tick != prev_tick )
	{
		sched_cnt++;

		switch ( sched_cnt )
		{
			case 1:
			{
				adc_read_values();
				break;
			}
			case 2:
			{
				erp_get_positions();
				break;
			}
			case 3:
			{
				erp_send_diff();
				//erp_print_positions();
				break;
			}
			case 4:
			{
        usb_midi_read();
				break;
			}
      case 5:
      {
        joystick_send();
        break;
      }
			case 10:
			{
				if( usb_midi_msg_cnt > 0 )
				{
					MidiUSB.flush();
				}
				sched_cnt = 0;
			}
			default:
				break;
		}
	}
	prev_tick = cur_tick;
}






/******************************************************************************/
/**	@brief	turns the two ad-values from an erp to a value/position
 	 	 	between 0..2047

	@param x analog outputs A of the erp (10 bit value)
	@param y analog outputs B of the erp (10 bit value)
	@return calculated position (0..2047)
*******************************************************************************/
static uint16_t erp_get_position(uint16_t x, uint16_t y)
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

