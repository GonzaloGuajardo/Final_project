/*************************************************************************************
* Gonzalo Alberto Guajardo Galindo
*
* Device: Atmega328P 
* Rev: 1 
* Date: 11/21/22 
*
* Description:
*   4 MODES
        * When connected, automatic counter from 0 to 159
        * The counter 0 to 159 should increase manually from an "ascending" button.
        * The counter 159 to 0 should decrease manually from a "descending" button.
        * The displays should show the ADC potentiometer reading (0V ? 00, 5V ? 999)
**************************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// ========== CONSTANTS ==========
// INPUTS
#define SW0 PIND1       // MODE
#define SWM PIND2       // INCREASE
#define SWL PIND3       // DECREASE
// OUTPUTS
#define DISPLAY PORTB
#define SWEEP_PORT PORTC
#define SWEEP_M PINC0
#define SWEEP_C PINC1
#define SWEEP_D PINC2
#define SWEEP_U PINC3

// ========== FUNCTIONS ==========
void init_ports(void);
void init_timer0(void);
void on_timer0(void);
void off_timer0(void);
void ADC_init(void);
void ADC_on(void);
void init_ExtInt(void);
uint8_t debounce_sw0(uint8_t pin);

// ========== VARIABLES ==========
// STATES
enum states {E0, E1, E2, E3} modes;
// MODES
uint8_t counter1 = -1;
// DISPLAY NUMBERS
uint8_t numbers[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
// FIRST DISPLAY
uint8_t thousands;
// SECOND DISPLAY
uint8_t hundreds;
// THIRD DISPLAY
uint8_t tens;
// FOURTH DISPLAY
uint8_t units;
// VOLATILES
volatile uint8_t i = 0;        // CTC
uint8_t count = 0;              // 1s = 10 times 0.1s
volatile uint16_t see = 0;      // Display
volatile uint16_t seeNum = 0;   // State machine
uint16_t converter;             // ADC Function
uint8_t counterM = 0;           // Add
uint8_t counterL = 159;         // Subtract
// DEBOUNCING
uint8_t currentState0 = 0;
uint8_t previousState0 = 0;

// ========== MAIN ==========
int main(void) {
    cli();
    init_ports();
    init_timer0();
    ADC_init();
    sei();
    on_timer0();
    
    while (1) {
        // Decompose the number
        thousands = seeNum / 1000;
        hundreds = (seeNum % 1000) / 100;
        tens = (seeNum % 100) / 10;
        units = seeNum % 10;
        
        // Mode Button
        if (debounce_sw0(SW0)) {
            if (counter1 >= 3) {
                counter1 = 0;
            } else {
                counter1++;
            }
        }
        
        switch (modes) {
            case E0: // Automatic counter
                seeNum = see;
                if (count >= 5) {
                    see++;
                    count = 0;
                    if (see >= 160) {
                        see = 0;
                    }
                }
                if (counter1 == 1) {
                    modes = E1;
                } else {
                    modes = E0;
                }
                break;
            case E1: // Manual counter (increase)
                init_ExtInt();
                seeNum = counterM;
                if (counter1 == 2) {
                    modes = E2;
                } else {
                    modes = E1;
                }
                break;
            case E2: // Manual counter (decrease)
                init_ExtInt();
                seeNum = counterL;
                if (counter1 == 3) {
                    modes = E3;
                } else {
                    modes = E2;
                }
                break;
            case E3: // ADC counter
                ADC_on();
                seeNum = converter;
                if (i == 0) {
                    SWEEP_PORT = ~(1 << i);
                    DISPLAY = numbers[thousands];
                } else if (i == 1) {
                    SWEEP_PORT = ~(1 << i);
                    DISPLAY = numbers[hundreds];
                } else if (i == 2) {
                    SWEEP_PORT = ~(1 << i);
                    DISPLAY = numbers[tens];
                } else if (i == 3) {
                    SWEEP_PORT = ~(1 << i);
                    DISPLAY = numbers[units];
                }
                if (i >= 3) {
                    i = 0;
                } else {
                    i++;
                }
                if (counter1 == 0) {
                    modes = E0;
                    see = 0;
                } else {
                    modes = E3;
                }
                break;
        }
    }
}

void init_ports(void) {
    // INPUTS
    DDRD &= ~_BV(SW0) | ~_BV(SWM) | ~_BV(SWL);
    PORTD |= _BV(SW0) | _BV(SWM) | _BV(SWL);
    // OUTPUTS
    DDRB |= 0x0F;
    PORTB |= 0X00;
    DDRC |= _BV(SWEEP_M) | _BV(SWEEP_C) | _BV(SWEEP_D) | _BV(SWEEP_U);
}

// ========== TIMER ==========
void init_timer0(void) {
    // CTC Mode
    TCCR0B &= ~_BV(WGM02);
    TCCR0A |= _BV(WGM01);
    TCCR0A &= ~_BV(WGM00);
    // TOP 0.1 second = 97
    // OCR0A = 156;
    // OCR0A = 97;
    OCR0A = 10;
    TIMSK0 |= _BV(OCIE0A);
}

void on_timer0(void) {
    // Restart count
    TCNT0 = 0;
    // Prescaler 1024
    TCCR0B |= _BV(CS02);
    TCCR0B &= ~_BV(CS01);
    TCCR0B |= _BV(CS00);
}

void off_timer0(void) {
    TCCR0B &= ~_BV(CS02);
    TCCR0B &= ~_BV(CS01);
    TCCR0B &= ~_BV(CS00);
}

ISR(TIMER0_COMPA_vect) {
    // Timer counter
    if (i == 0) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[thousands];
    } else if (i == 1) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[hundreds];
    } else if (i == 2) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[tens];
    } else if (i == 3) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[units];
    }
    if (i >= 3) {
        i = 0;
    } else {
        i++;
    }
    count++;
}

// ========== DEBOUNCING ==========
uint8_t debounce_sw0(uint8_t pin) {
    previousState0 = currentState0;
    currentState0 = bit_is_clear(PIND, pin);
    // If there is a change from 0 to 1 returns a 1
    if (previousState0 == 0 && currentState0 == 1) {
        return 1;
    }
    // Any other combination returns a 0
    else {
        return 0;
    }
}

// ========== EXTERNAL INTERRUPTS ==========
void init_ExtInt(void) {
    // INT0 (decrease)
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    // Activate
    EIMSK |= (1 << INT0);
    // INT1 (increase)
    EICRA |= (1 << ISC11);
    EICRA &= ~(1 << ISC10);
    // Activate
    EIMSK |= (1 << INT1);
}

ISR(INT0_vect) {
    if (counterM >= 159) {
        counterM = 159;
    } else {
        counterM++;
    }
    // Timer counter
    if (i == 0) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[thousands];
    } else if (i == 1) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[hundreds];
    } else if (i == 2) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[tens];
    } else if (i == 3) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[units];
    }
    if (i >= 3) {
        i = 0;
    } else {
        i++;
    }
}

ISR(INT1_vect) {
    if (counterL <= 0) {
        counterL = 0;
    } else {
        counterL--;
    }
    // Timer counter
    if (i == 0) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[thousands];
    } else if (i == 1) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[hundreds];
    } else if (i == 2) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[tens];
    } else if (i == 3) {
        SWEEP_PORT = ~(1 << i);
        DISPLAY = numbers[units];
    }
    if (i >= 3) {
        i = 0;
    } else {
        i++;
    }
}

// ========== ADC ==========
void ADC_init(void) {
    // Avcc as reference pin
    ADMUX &= ~_BV(REFS1);   // 1
    ADMUX |= _BV(REFS0);    // 0
    // Adjust to 8 bits
    ADMUX |= (1 << ADLAR);
    // Choose PIN to read ADC5
    ADMUX &= ~_BV(MUX3);    // 1
    ADMUX |= _BV(MUX2);     // 0
    ADMUX &= ~_BV(MUX1);    // 1
    ADMUX |= _BV(MUX0);     // 0
    // Freeruning
    ADCSRA |= _BV(ADATE);
    // Enable interrupt
    ADCSRA |= _BV(ADIE);
    // 1 MHz clock / 8 = 125 kHz ADC
    ADCSRA &= ~_BV(ADPS2);  // 1 
    ADCSRA |= _BV(ADPS1);   // 0
    ADCSRA |= _BV(ADPS0);   // 0
}

void ADC_on(void) {
    // Turn on
    ADCSRA |= _BV(ADEN);
    // Start
    ADCSRA |= _BV(ADSC);
}

ISR(ADC_vect) {
    // Range from 0 to 999
    // converter = (ADCH / 255) * 999;
    converter = ADCH * 3.92;
}
