#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define F_CPU 8000000
#define LED1 PB1
#define DOORS PB2
#define IGNITION PC0
#define ENGINE PC1

const unsigned char car_ask = 1;
volatile unsigned char phone_reply = 0;
const uint8_t key_pin = 1;
uint8_t key_1_tmp = 0;
const unsigned char car_ask_again = 1;
volatile unsigned char car_ask_again_reply = 0;
const uint8_t key_pin_again = 1;
uint8_t key_2_tmp = 0;
volatile uint8_t trigger = 0 ;

//turn on interups... turn on sleep... and turn off sleep after ISR(INT0)...
void sleep_start()
{
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();
	sleep_enabled();
	sei();
	sleep_cpu();
	sleep_disabled();
}

//setup interrupt
void interrupt_init()
	{
		PORTD|=(1<<PD2);			//pullup
		MCUCR|=(0<<ISC01)
				|(0<<ISC00);		//low level int0 interupt
		GICR|=(1<<INT0);			//INT0 enable
	}

//interrupt with rs232 initiation
ISR(INT0_vect)
{
	blue_init(1);
	blue_send(car_ask);

	DDRB|=(1<<LED1);		//blink ;)
	PORTB|=(1<<LED1);

	cli();			//interrupt off
}

//setup USART
static void usart_9600(void)
{
   #define BAUD 9600
   #include <util/setbaud.h>
UBRRH = UBRRH_VALUE;
UBRRL = UBRRL_VALUE;
   #if USE_2X  					// Double the USART transmission speed
   UCSRA |= (1 << U2X);  		//8 faster
   #else
   UCSRA &= ~(1 << U2X);		//16 slower
   #endif
   }

//bluetooth initiatnion
void blue_init(int a)
{
	if( a == 1 )
	{
	usart_9600();
	UCSRB|=(1<<RXEN)|(1<<TXEN);						//enabled Tx and Rx
	UCSRC|=(1<<URSEL)|(1<<USBS)|(1<<UCSZ0);			//frame: 8 bit, 2 stop bits, no parity
	}
	if( a == 0 )
	{
	UCSRB&=~(1<<RXEN)
			&~(1<<TXEN);	//bluetooth off (usart)
	}
}

//bluetooth send message
void blue_send(uint8_t date)
{
		while( !( UCSRA & (1<<UDRE)));			//empty bufor - flag
		UDR = date;
		_delay_ms(500);
}

/*
 * void send_big_keys(unsigned char *s)
{
     while(*s)
     {
          usart_transfer(*s);
          s++;
     }
}
 */

//bluetooth recived message
uint8_t blue_recive()
{
	while( !( UCSRA & (1<<RXC)));			//flag
	return UDR;
	_delay_ms(500);
}

//open doors
void doors(uint8_t a)
{
	if(a)
	{
		DDRB|=(1<<DOORS);
		PORTB|=(1<<DOORS);
	}
	else
	{
		DDRB&=~(1<<DOORS);
		PORTB&=~(1<<DOORS);
	}
}

//turn on ignition
 void ignition(uint8_t a)
{
	if(a)
	{
		DDRC|=(1<<IGNITION);
		PORTC|=(1<<IGNITION);
	}
	else
	{
		DDRC&=~(1<<IGNITION);
		PORTC&=~(1<<IGNITION);
	}
}

 //turn off engine
 uint8_t turn_off()
 {
	if(PINC & (1<<ENGINE))
		return 1;
	else
		return 0;
 }

int main (void)
{
	while(1)
	{
		if( trigger != 1 )
		{
			interrupt_init();
			sleep_start();
		}

		//```````````````````````````````````````````````````````````````````````````````` tu wchodzi interrupt

		if( ( blue_recive() == phone_reply ) || ( key_pin == key_1_tmp ) )
		{
			trigger = 1;
			doors(trigger);
			blue_send(car_ask_again);

			if( ( blue_recive() == car_ask_again_reply ) || ( key_pin_again == key_2_tmp ) )
			{
				key_1_tmp = key_pin;
				key_2_tmp = key_pin_again;
				blue_init(0);
				ignition(trigger); 		//zaplon wlaczony
				if(turn_off())
				{
					trigger = 0;
					ignition(trigger);
					key_1_tmp = 0;
					key_2_tmp = 0;
					_delay_ms(10000);
				}
			}
			else
			{
				trigger = 0;
			}
		}
		//``````````````````````````````````````````````````````````````````````````````````
		doors(trigger);
	}
}


