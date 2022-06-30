/*
  One MAX7219 connected to an 8x8 LED matrix.
 */
#define F_CPU 16000000UL //16MHz
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h> 

#define CLK_HIGH()		PORTB |= (1<<PORTB0)
#define CLK_LOW()		PORTB &= ~(1<<PORTB0)
#define CS_HIGH()		PORTB |= (1<<PORTB1)
#define CS_LOW()		PORTB &= ~(1<<PORTB1)
#define DATA_HIGH()		PORTB |= (1<<PORTB2)
#define DATA_LOW()		PORTB &= ~(1<<PORTB2)
#define INIT_PORT()		DDRB |= (1<<PORTB0) | (1<<PORTB1) | (1<<PORTB2)


uint8_t cordx = 0B00000001;            /* x coordinate player */
uint8_t cordy = 0b01000000;            /* y coordinate player */

uint8_t level1[8] = {
        0B11111111,
		0B10110001,
		0B10000101,
		0B10110101,
		0B10100101,
		0B10111101,
		0B10100001,
		0B10101111};
uint8_t player_level1[8] = {
        0B11111111,
		0B10110001,
		0B10000101,
		0B10110101,
		0B10100101,
		0B10111101,
		0B10100001,
		0B11101111};
uint8_t player_level1_start[8] = {
        0B11111111,
		0B10110001,
		0B10000101,
		0B10110101,
		0B10100101,
		0B10111101,
		0B10100001,
		0B11101111};
uint8_t w[8] = {
        0B11000011,
		0B11000011,
		0B11000011,
		0B11000011,
		0B11011011,
		0B11111111,
		0B11100111,
		0B11000011};
uint8_t WIN_I[8] = {
        0B00011000,
		0B00011000,
		0B00000000,
		0B00011000,
		0B00011000,
		0B00011000,
		0B00011000,
		0B00011000};
uint8_t n[8] = {
        0B11000011,
		0B11100011,
		0B11110011,
		0B11111011,
		0B11011111,
		0B11001111,
		0B11000111,
		0B11000011};
		
int where(uint8_t XX){
	int wynik=0;
	int i =0;
	for (i=XX;i>0;i=i/2)
	{
		wynik=wynik+1;
	}
	return wynik;
}
void spi_send(uint8_t data)
{
    uint8_t i;

    for (i = 0; i < 8; i++, data <<= 1)
    {
	CLK_LOW();
	if (data & 0x80)
	    DATA_HIGH();
	else
	    DATA_LOW();
	CLK_HIGH();
    }
    
}

void max7219_writec(uint8_t high_byte, uint8_t low_byte)
{
    CS_LOW();
    spi_send(high_byte);
    spi_send(low_byte);
    CS_HIGH();
}

void max7219_clear(void)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
	max7219_writec(i+1, 0);
    }
}

void max7219_init(void)
{
    INIT_PORT();
    // Decode mode: none
    max7219_writec(0x0b, 0);
    // Intensity: 3 (0-15)
    max7219_writec(0x0A, 1);
    // Scan limit: All "digits" (rows) on
    max7219_writec(0x0B, 7);
    // Shutdown register: Display on
    max7219_writec(0x0C, 1);
    // Display test: off
    max7219_writec(0x0F, 0);
    max7219_clear();
}


uint8_t display[8];

void update_display(void)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
	max7219_writec(i+1, display[i]);
    }
}

void image( uint8_t im[8])
{
    uint8_t i;

    for (i = 0; i < 8; i++)
	display[i] = im[i];
}
//This function retreives the ADC value from the given channel
uint16_t get_adc_value(uint8_t channel)
{
	ADMUX &=0xf0;
	//select channel
	ADMUX |= channel;

	//start conversion on channel
	ADCSRA |= (1<<ADSC);

	//wait for conversion to finish
	while((ADCSRA & (1<<ADIF)) == 0);

	return(ADCW); // return the converted value which is in ADCW
}
/* This function reads the joystick analog sensors input and returns the direction
0 - Stationary  1 - Up  2 - Right 3 - Down 4- Left*/
uint8_t player_direction()
{
	uint16_t x_pos, y_pos;
	uint8_t direction = 0; //static to retain direction during subsequent calls
	x_pos = get_adc_value(0);
	y_pos = get_adc_value(1);
	if(direction == 4 || direction == 2 || direction == 0)
		{
			if((x_pos>=0)&&(x_pos < 300))
			direction = 3;

			if((x_pos >= 700)&&(x_pos < 1024))
			direction = 1;
		}
		
		if(direction == 3 || direction == 1 || direction == 0)
		{
			if((y_pos >=0 )&&(y_pos<300))
			direction = 4;

			if((y_pos >= 700)&&(y_pos<1024))
			direction =2;
		}
	return direction;
}


int main(void)
{
    max7219_init();
	DDRC = 0;										//port a as input
	ADMUX |=0b01 << REFS0;
	ADCSRA=(1<<ADEN) | (1<<ADIE)| (1<<ADPS2) | (1<<ADPS1)| (1<<ADPS0);		// enable ADC , sampling freq = clk/128
    DIDR0 |= 1<< PINC0;
    DIDR0 |= 1<< PINC1;
	while(1)
    {
		image(level1);
		update_display();
		_delay_ms(500);
		image(player_level1);
		update_display();
		_delay_ms(500);
		if (player_direction()>0)
		{
			uint8_t move = player_direction();
			if(move == 1){
				player_level1[8-where(cordx)]=level1[8-where(cordx)];
				cordx=cordx<<1;
				if ((player_level1[8-where(cordx)]&cordy)>0)
				{
					player_level1[8-where(cordx)]=player_level1_start[8-where(cordx)];
					cordx = 0B00000001;
					cordy = 0b01000000;
					image(player_level1_start);
					update_display();
					move=0;
				}
				else{
				player_level1[8-where(cordx)]=level1[8-where(cordx)] |cordy;
				image(player_level1);
				update_display();
				move=0;}
			}
			if(move == 3){
				player_level1[8-where(cordx)]=level1[8-where(cordx)];
				cordx=cordx>>1;
				if ((player_level1[8-where(cordx)]&cordy)>0)
				{
					player_level1[8-where(cordx)]=player_level1_start[8-where(cordx)];
					cordx = 0B00000001;
					cordy = 0b01000000;
					image(player_level1_start);
					update_display();
					move=0;
				}
				else{
				player_level1[8-where(cordx)]=level1[8-where(cordx)] |cordy;
				image(player_level1);
				update_display();
				move=0;}
			}
			if(move == 2){
				player_level1[8-where(cordx)]=level1[8-where(cordx)];
				cordy=cordy>>1;
				if ((player_level1[8-where(cordx)]&cordy)>0)
				{
					player_level1[8-where(cordx)]=player_level1_start[8-where(cordx)];
					cordx = 0B00000001;            
					cordy = 0b01000000;  
					image(player_level1_start);
					update_display();
					move=0;
				}
				else{
				player_level1[8-where(cordx)]=level1[8-where(cordx)] |cordy;
				image(player_level1);
				update_display();
				move=0;}
			}
			if(move == 4){
				player_level1[8-where(cordx)]=level1[8-where(cordx)];
				cordy=cordy<<1;
				if ((player_level1[8-where(cordx)]&cordy)>0)
				{
					player_level1[8-where(cordx)]=player_level1_start[8-where(cordx)];
					cordx = 0B00000001;
					cordy = 0b01000000;
					image(player_level1_start);
					update_display();
					move=0;
				}
				else{
				player_level1[8-where(cordx)]=level1[8-where(cordx)] |cordy;
				image(player_level1);
				update_display();
				move=0;}
			}
			if ((cordx==0B00000001)&(cordy == 0b00010000))
			{
				uint8_t j ;
				for (j=5;j>0;j=j-1)
				{
					_delay_ms(500);
					image(w);
					update_display();
					_delay_ms(500);
					image(WIN_I);
					update_display();
					_delay_ms(500);
					image(n);
					update_display();
					_delay_ms(500);
				}
				player_level1[8-where(cordx)]=player_level1_start[8-where(cordx)];
				cordx = 0B00000001;
				cordy = 0b01000000;
				image(player_level1_start);
				update_display();
				move=0;
			}
		}
    }
}