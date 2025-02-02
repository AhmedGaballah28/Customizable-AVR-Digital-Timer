#include <avr/io.h>               // Include AVR input/output header
#include <avr/interrupt.h>        // Include AVR interrupt handling
#include <util/delay.h>           // Include AVR delay functions
#define F_CPU 16000000UL          // Define the clock frequency as 16 MHz

// Declare variables for time units and control flags
unsigned char secs1=0, secs2=0, minutes1=0, minutes2=0, hours1=0, hours2=0, flag_toggle=0, flag_pause=0;

void display (void)               // Function to display the time
{
	PORTA=(1<<PA5);               // Set PA5 to display seconds1
	PORTC=(PORTC&0xf0)|(secs1&0x0f); // Display the low nibble of seconds1
	_delay_ms(2);
	PORTA=(1<<PA4);               // Set PA4 to display seconds2
	PORTC=(PORTC&0xf0)|(secs2&0x0f); // Display the low nibble of seconds2
	_delay_ms(2);
	PORTA=(1<<PA3);               // Set PA3 to display minutes1
	PORTC=(PORTC&0xf0)|(minutes1&0x0f); // Display the low nibble of minutes1
	_delay_ms(2);
	PORTA=(1<<PA2);               // Set PA2 to display minutes2
	PORTC=(PORTC&0xf0)|(minutes2&0x0f); // Display the low nibble of minutes2
	_delay_ms(2);
	PORTA=(1<<PA1);               // Set PA1 to display hours1
	PORTC=(PORTC&0xf0)|(hours1&0x0f); // Display the low nibble of hours1
	_delay_ms(2);
	PORTA=(1<<PA0);               // Set PA0 to display hours2
	PORTC=(PORTC&0xf0)|(hours2&0x0f); // Display the low nibble of hours2
	_delay_ms(2);
}

void time_increment(void)         // Function to increment time
{
	secs1++;                       // Increment seconds1
	if(secs1==10)                  // If seconds1 reaches 10, reset and increment seconds2
	{
		secs2++;
		secs1=0;
	}
	if(secs2==6)                   // If seconds2 reaches 6, reset and increment minutes1
	{
		minutes1++;
		secs2=0;
	}
	if(minutes1==10)               // If minutes1 reaches 10, reset and increment minutes2
	{
		minutes2++;
		minutes1=0;
	}
	if(minutes2==6)                // If minutes2 reaches 6, reset and increment hours1
	{
		hours1++;
		minutes2=0;
	}
	if(hours1==10)                 // If hours1 reaches 10, reset and increment hours2
	{
		hours1=0;
		hours2++;
	}
	if((hours2==2)&(hours1==4))     // If hours2 reaches 2, and hours1 reaches 4 reset all counters and repeat timer again
	{
		hours1=hours2=0;

	}
}

void time_decrement(void)         // Function to decrement time
{
	if (secs1 > 0) {               // If seconds1 > 0, decrement seconds1
		secs1--;
	} else {
		if (secs2 > 0) {           // If seconds2 > 0, decrement seconds2 and set seconds1 to 9
			secs2--;
			secs1 = 9;
		} else {
			if (minutes1 > 0) {    // If minutes1 > 0, decrement minutes1, set seconds2 and seconds1
				minutes1--;
				secs2 = 5;
				secs1 = 9;
			} else {
				if (minutes2 > 0) { // If minutes2 > 0, decrement minutes2, set other time units
					minutes2--;
					minutes1 = 9;
					secs2 = 5;
					secs1 = 9;
				} else {
					if (hours1 > 0) { // If hours1 > 0, decrement hours1, set other time units
						hours1--;
						minutes2 = 5;
						minutes1 = 9;
						secs2 = 5;
						secs1 = 9;
					} else {
						if (hours2 > 0) { // If hours2 > 0, decrement hours2, set other time units
							hours2--;
							hours1 = 9;
							minutes2 = 5;
							minutes1 = 9;
							secs2 = 5;
							secs1 = 9;
						}
					}
				}
			}
		}
	}
	if((hours2==0)&(hours1==0)&(minutes2==0)&(minutes1==0)&(secs2==0)&(secs1==0)) // If all time units are zero, activate alarm
	{
		PORTD|=(1<<PD0);           // Activate alarm on PD0
	}
}

void Timer1_set (void)            // Function to set Timer1
{
	SREG|=(1<<7);                 // Enable global interrupts
	TCCR1A=(1<<COM1B0)|(1<<COM1B1)|(1<<FOC1B); // Set timer mode
	TCCR1B=(1<<WGM12)|(1<<CS10)|(1<<CS12);     // Set timer mode and prescaler
	TCNT1=0;                       // Reset timer count
	OCR1A=15625;                   // Set compare value for Timer1
	TIMSK&=~(1<<OCIE1A);           // Disable Output Compare A Match interrupt
	TIMSK|=(1<<OCIE1B);            // Enable Output Compare B Match interrupt
}

ISR (TIMER1_COMPB_vect)           // Interrupt Service Routine for Timer1 Compare B
{
	if(flag_toggle==0)             // If toggle flag is 0, increment time
	{
		time_increment();
	}
	else if(flag_toggle==1)        // If toggle flag is 1, decrement time
	{
		time_decrement();
	}
}
void Toggle_button (void)         // Function to configure the toggle button
{
	SREG|=(1<<7);                 // Enable global interrupts
	DDRB&=~(1<<PB7);              // Set PB7 as input for the toggle button
	PORTB|=(1<<PB7);              // Enable pull-up resistor on PB7
}

void External_Interrupt_INT0 (void) // Function to configure external interrupt INT0 (Reset button)
{
	SREG|=(1<<7);                 // Enable global interrupts
	DDRD&=~(1<<PD2);              // Set PD2 as input for the reset button
	PORTD|=(1<<PD2);              // Enable pull-up resistor on PD2
	MCUCR|=(1<<ISC01);            // Set INT0 to trigger on falling edge
	MCUCR&=~(1<<ISC00);           // Ensure ISC00 is cleared (falling edge)
	GICR|=(1<<INT0);              // Enable external interrupt INT0
}

ISR (INT0_vect)                   // Interrupt Service Routine for INT0 (Reset button)
{
	if(flag_pause==0)             // If not paused, reset the timer and flags
	{
		TCCR1B=TCCR1A=0;          // Stop Timer1
		hours2=hours1=minutes2=minutes1=secs2=secs1=0; // Reset all time counters
		_delay_ms(2);
		Timer1_set();             // Reinitialize Timer1
		PORTD&=~(1<<PD0);         // Turn off alarm (PD0)
		flag_toggle=0;            // Reset toggle flag
	}
	else if((flag_pause==1)&(flag_toggle==1)) // If paused and toggle flag is 1, reset time and flags
	{
		if((hours2==0)&(hours1==0)&(minutes2==0)&(minutes1==0)&(secs2==0)&(secs1==0)) // Check if time is zero
		{
			flag_pause=0;         // Reset pause flag
			flag_toggle=0;        // Reset toggle flag
			Timer1_set();         // Reinitialize Timer1
			PORTD&=~(1<<PD0);     // Turn off alarm (PD0)
		}
		else                     // If time is not zero, reset time and stop the timer
		{
			TCCR1B=0;            // Stop Timer1
			hours2=hours1=minutes2=minutes1=secs2=secs1=0; // Reset all time counters
			PORTD&=~(1<<PD0);    // Turn off alarm (PD0)
		}
	}
	else{
		hours2=hours1=minutes2=minutes1=secs2=secs1=0;       //Reset Timer
		flag_pause=0;                                        // Reset pause flag
		flag_toggle=0;                                       // Reset toggle flag
		Timer1_set();                                        // Reinitialize Timer1
	}
}

void External_Interrupt_INT1 (void) // Function to configure external interrupt INT1 (Pause button)
{
	SREG|=(1<<7);                 // Enable global interrupts
	DDRD&=~(1<<PD3);              // Set PD3 as input for the pause button
	MCUCR|=(1<<ISC10)|(1<<ISC11); // Set INT1 to trigger on rising edge
	GICR|=(1<<INT1);              // Enable external interrupt INT1
}

ISR (INT1_vect)                   // Interrupt Service Routine for INT1 (Pause button)
{
	TCCR1B=0;                     // Stop Timer1
	PORTD&=~(1<<PD0);             // Turn off alarm (PD0)
	if(flag_toggle==1)            // If toggle flag is 1, set pause flag
	{
		flag_pause=1;
	}
}

void External_Interrupt_INT2 (void) // Function to configure external interrupt INT2 (Resume button)
{
	SREG|=(1<<7);                 // Enable global interrupts
	DDRB|=(1<<PB2);               // Set PB2 as input for the resume button
	PORTB|=(1<<PB2);              // Enable pull-up resistor on PB2
	MCUCR&=~(1<<ISC2);            // Ensure ISC2 is cleared (falling edge trigger)
	GICR|=(1<<INT2);              // Enable external interrupt INT2
}

ISR (INT2_vect)                   // Interrupt Service Routine for INT2 (Resume button)
{
	TCCR1B=(1<<WGM12)|(1<<CS10)|(1<<CS12); // Restart Timer1
	flag_pause=0;                 // Reset pause flag
}

void sec_buttons (void)           // Function to configure buttons for seconds adjustment
{
	DDRB&=~(1<<PB6)&~(1<<PB5);    // Set PB6 and PB5 as inputs for seconds adjustment buttons
	PORTB|=(1<<PB6)|(1<<PB5);     // Enable pull-up resistors on PB6 and PB5
}

void minute_buttons(void)         // Function to configure buttons for minutes adjustment
{
	DDRB&=~(1<<PB4)&~(1<<PB3);    // Set PB4 and PB3 as inputs for minutes adjustment buttons
	PORTB|=(1<<PB4)|(1<<PB3);     // Enable pull-up resistors on PB4 and PB3
}

void hour_buttons(void)           // Function to configure buttons for hours adjustment
{
	DDRB&=~(1<<PB1)&~(1<<PB0);    // Set PB1 and PB0 as inputs for hours adjustment buttons
	PORTB|=(1<<PB1)|(1<<PB0);     // Enable pull-up resistors on PB1 and PB0
}

void time_setting (void)          // Function to handle time setting using buttons
{
	if(!(PINB&(1<<PB6)))          // If seconds increment button (PB6) is pressed
	{
		_delay_ms(30);            // Debounce the button press
		if(!(PINB&(1<<PB6)))      // Check if the button is still pressed
		{
			secs1++;              // Increment seconds1
			if(secs1==10)         // Reset seconds1 and increment seconds2 if necessary
			{
				secs1=0;
				secs2++;
			}
			if(secs2==6)          // Reset seconds2 if necessary
			{
				secs2=0;
			}
		}
		while(!(PINB&(1<<PB6)))   // Display the time while button is pressed
		{
			display();
		};
	}
	else if(!(PINB&(1<<PB5)))     // If seconds decrement button (PB5) is pressed
	{
		_delay_ms(30);            // Debounce the button press
		if(!(PINB&(1<<PB5)))      // Check if the button is still pressed
		{
			if((secs1==0)&(secs2==0)) // Handle decrementing seconds
			{
				secs1=9;
				secs2=5;
			}
			else if((secs2>0)&(secs1==0))
			{
				secs2--;
				secs1=9;
			}
			else{
				secs1--;
			}
		}
		while(!(PINB&(1<<PB5)))   // Display the time while button is pressed
		{
			display();
		}
	}
	if(!(PINB&(1<<PB4)))          // If minutes increment button (PB4) is pressed
	{
		_delay_ms(30);            // Debounce the button press
		if(!(PINB&(1<<PB4)))      // Check if the button is still pressed
		{
			minutes1++;           // Increment minutes1
			if(minutes1==10)      // Reset minutes1 and increment minutes2 if necessary
			{
				minutes1=0;
				minutes2++;
			}
			if(minutes2==6)       // Reset minutes2 if necessary
			{
				minutes2=0;
			}
		}
		while(!(PINB&(1<<PB4)))   // Display the time while button is pressed
		{
			display();
		}
	}
	else if(!(PINB&(1<<PB3)))     // If minutes decrement button (PB3) is pressed
	{
		_delay_ms(30);            // Debounce the button press
		if(!(PINB&(1<<PB3)))      // Check if the button is still pressed
		{
			if((minutes1==0)&(minutes2==0)) // Handle decrementing minutes
			{
				minutes1=9;
				minutes2=5;
			}
			else if((minutes2>0)&(minutes1==0))
			{
				minutes2--;
				minutes1=9;
			}
			else{
				minutes1--;
			}
		}
		while(!(PINB&(1<<PB3)))   // Display the time while button is pressed
		{
			display();
		}
	}
	if(!(PINB&(1<<PB1)))          // If hours increment button (PB1) is pressed
	{
		_delay_ms(30);            // Debounce the button press
		if(!(PINB&(1<<PB1)))      // Check if the button is still pressed
		{
			hours1++;             // Increment hours1
			if(hours1==10)        // Reset hours1 and increment hours2 if necessary
			{
				hours1=0;
				hours2++;
			}
			if((hours2==2)&(hours1==4)) // Reset hours to 00 after 23
			{
				hours2=hours1=0;
			}
		}
		while(!(PINB&(1<<PB1)))   // Display the time while button is pressed
		{
			display();
		}
	}
	else if(!(PINB&(1<<PB0)))     // If hours decrement button (PB0) is pressed
	{
		_delay_ms(30);            // Debounce the button press
		if(!(PINB&(1<<PB0)))      // Check if the button is still pressed
		{
			if((hours1==0)&(hours2==0)) // Handle decrementing hours
			{
				hours1=3;
				hours2=2;
			}
			else if((hours2>0)&(hours1==0))
			{
				hours2--;
				hours1=9;
			}
			else{
				hours1--;
			}
		}
		while(!(PINB&(1<<PB0)))   // Display the time while button is pressed
		{
			display();
		}
	}
}

int main (void)
{
	DDRA = (1<<PA0) | (1<<PA1) | (1<<PA2) | (1<<PA3) | (1<<PA4); // Set specific pins of PORTA as output
	PORTA = 0b00111111;       // Initialize PORTA with this binary value
	DDRC = DDRC | (0x0f);     // Set the lower 4 bits of PORTC as output
	PORTC = 0;                // Initialize PORTC to zero
	/* Alarm Setting */
	{
		DDRD |= (1<<PD0);     // Set PD0 as output for alarm
		PORTD &= ~(1<<PD0);   // Clear PD0 (turn off alarm)
	}
	Timer1_set();             // Set up Timer1 with compare mode B
	External_Interrupt_INT0(); // Initialize external interrupt for the reset button
	External_Interrupt_INT1(); // Initialize external interrupt for the pause button
	External_Interrupt_INT2(); // Initialize external interrupt for the resume button
	Toggle_button();           // Initialize the toggle button (PB7)
	sec_buttons();             // Initialize the second adjustment buttons
	minute_buttons();          // Initialize the minute adjustment buttons
	hour_buttons();            // Initialize the hour adjustment buttons
	for(;;)                    // Infinite loop
	{
		display();             // Continuously update the display
		time_setting();        // Handle time setting based on button presses
		if(!(PINB & (1<<PB7))) // Check if the toggle button (PB7) is pressed
		{
			_delay_ms(30);     // Debounce the button
			if(!(PINB & (1<<PB7))) // If button is still pressed
			{
				flag_toggle = flag_toggle ^ 1; // Toggle between increment and decrement modes
				if(flag_toggle == 0)           // If in increment mode
				{
					TCCR1A = (1<<COM1B0) | (1<<COM1B1) | (1<<FOC1B); // Set timer compare mode
					TCCR1B = (1<<WGM12) | (1<<CS10) | (1<<CS12);     // Configure timer
					PORTD &= ~(1<<PD0); // Turn off alarm
				}
				else if(flag_toggle == 1)       // If in decrement mode
				{
					TCCR1A = (1<<COM1A0) | (1<<COM1A1) | (1<<FOC1B); // Set different timer mode
					TCCR1B = (1<<WGM12) | (1<<CS10) | (1<<CS12);     // Configure timer
				}
			}
			while(!(PINB & (1<<PB7))) // Wait until the button is released
			{
				display();             // Keep updating the display
			}
		}
	}
	return 0;
}

