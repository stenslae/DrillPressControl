// MSP430 C Code Template for use with TI Code Composer Studio
// Emma Stensland, EELE371, Final Project, Part 1
// November 6, 2024

#include <msp430.h> 

unsigned int ADC_Value;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    // ---- SETUP PORTS:
    // Red LED 1: P1.0
    // set p1 to dig i/o (sel0&1 = 0)
    P1SEL0 = 0;
    P1SEL1 = 0;
    // set p1 to an output at bit 0
    P1DIR |= BIT0;
    // set output at bit 0
    P1OUT |= BIT0;

    // Green LED 2: P6.6
    // set p6 to dig i/o (sel0&1 = 0)
    P6SEL0 = 0;
    P6SEL1 = 0;
    // set p6 to an output at bit 6
    P6DIR |= BIT6;
    // clear output at bit 6
    P6OUT &= ~BIT6;

    // ADC pin setup:
    P1SEL1 |= BIT4;                     // configure p1.4 pin for a4
    P1SEL0 |= BIT4;

    // ----- CONFIGURE ADC:
    ADCCTL0 &= ~ADCSHT;                 // clear adcsht from def
    ADCCTL0 |= ADCSHT_2;                // conversion cycles = 16
    ADCCTL0 |= ADCON;                   // turn adc on

    ADCCTL1 |= ADCSSEL_2;               // adc clock source = smclk
    ADCCTL1 |= ADCSHP;                  // sample signal source= sampling timer

    ADCCTL2 &= ~ADCRES;                 // clear adcres from def of adcres=01
    ADCCTL2 |= ADCRES_2;                // resolution = 12bit (ADCRES=10)

    ADCMCTL0 |= ADCINCH_4;              // adc input channel = A4 (P1.4)

    // ------ CONFIGURE INTERRUPTS:
    ADCIE |= ADCIE0;                    // enable adc irq

    PM5CTL0 &= ~LOCKLPM5;               // clear high z
    __enable_interrupt();

    while(1){
        ADCCTL0 |= ADCENC | ADCSC;      // enable and start conversion
        while((ADCIFG & ADCIFG0)==0);   // wait until conv. is complete
    }

    return 0;
}
//--------------- End MAIN ----------------------------------
//--------------------------------------------------------------------
// Interrupt Service Routines
//--------------------------------------------------------------------
//------- ADC_ISR ---------------------------

//A voltage reading is found from pin 1.4
//When the reading is below 1V, the Green LED indicates a “safe” operating condition.
//When the reading surpasses 1V, the LEDs turn off to indicate “warning” state.
//When the reading surpasses 2V, the Red LED indicates an “unsafe” operating condition.

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void){
    ADC_Value = ADCMEM0;                // read adc value

    // 2409 is expected to be at 2V, but is actually at 1.94V
    // 1205 is expected to be at 1V, but is actually at 0.94V
    // Resolution error is +/- 8.30mV
    // Offset ADC_Value and noise result in some error, that is accounted for

    if(ADC_Value>2499){           // a4 > 2V, the red led turns on
        P1OUT |= BIT0;
        P6OUT &= ~BIT6;
    }else if(ADC_Value>=1245){           // a2> 1V, the led turns off
        P1OUT &= ~BIT0;
        P6OUT &= ~BIT6;
    }else{                              // a2 <= 1V, the green led is on
        P1OUT &= ~BIT0;
        P6OUT |= BIT6;
    }
}
//------- End ADC_ISR ---------------------------
//------- END ISRs ---------------------------
