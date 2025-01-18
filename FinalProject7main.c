//--------------------------------------------------------------------
// MSP430 C Code Template for use with TI Code Composer Studio
// Emma Stensland, EELE371, Final Project 7
// December 6, 2024
//--------------------------------------------------------------------
// PROJECT DESCRIPTION:
// Switches serve as input to decide to move motor forward or reverse,
// and UART announces the movement of the motor. 513 Pulses gets a full
// 360 degrees while 36 degrees is 51 steps
//--------------------------------------------------------------------
// PINOUT on MSP430++:
// Switch 1: 4.1
// Switch 2: 2.3
// LEDS: 3.0-3
// UART: 4.3
//--------------------------------------------------------------------
// CALCULATIONS:
// 1 Full Rotation is 513 steps, 36 degrees is 51 steps.
// The fastest the motor should run is 25 RPM with 5V supply -> width of 4.68 ms
// The reduced speed sets the pulse a width of 25.53 ms
//--------------------------------------------------------------------

#include <msp430.h> 

// TODO -- Update with rotation size: (513 = 360 degrees)
int rspin = 513;
int fspin = 51;
char message1[] = "\n\r Motor moved forward 36 degrees. \r\n";
char message2[] = "\n\r Motor reversed 1 rotation. \r\n";

// Declare Variables/Subroutines:

// Subroutines
int init(void);
int rotateCW(void);
int rotateCCW(void);

// Loop/Status Variables
int i, count;
int dir=3;
unsigned int endof, position;


//--------------- MAIN -------------------------------------------
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    // Initialize Pinouts:
    init();

    // Infinite loop
    while(1){
    }

    return 0;
}
//--------------- End MAIN -------------------------------------------

//--------------------------------------------------------------------
// SUBROUTINES
//--------------------------------------------------------------------

//--------------- Init -----------------------------------------------
// Intialize ADC and I2C on the MSP
//--------------------------------------------------------------------

int init(void){
    // 1. SW RESET
    UCA1CTLW0 |= UCSWRST;

    // 2. CONFIGURE UART CLOCK
    UCA1CTLW0 |= UCSSEL__SMCLK;

    UCA1BRW = 17;
    UCA1MCTLW |= 0x4A00;            // baud 57600

    // 3. CONFIG PORTS
    // SWITCHES
    // SW1
    P4DIR &= ~BIT1;
    P4REN |= BIT1;
    P4OUT |= BIT1;
    P4IES |= BIT1;
    // SW2
    P2DIR &= ~BIT3;
    P2REN |= BIT3;
    P2OUT |= BIT3;
    P2IES |= BIT3;

    // LEDS
    // LED 1-4: P3.0
    // set p3 to dig i/o (sel0&1 = 0)
    P3SEL0 = 0;
    P3SEL1 = 0;
    // set p1 to an output at bit 0
    P3DIR |= BIT0;
    P3DIR |= BIT1;
    P3DIR |= BIT2;
    P3DIR |= BIT3;
    // set/clear outputs
    P3OUT |= BIT0;
    P3OUT &= ~BIT1;
    P3OUT &= ~BIT2;
    P3OUT &= ~BIT3;

    // UART
    P4SEL1 &= ~BIT3;
    P4SEL0 |= BIT3;

    // CLOCK SETUP
    TB0CTL |= TBCLR;                 // TBCLR=1 clears timers and dividers
    TB0CTL |= TBSSEL__SMCLK;         // TBSSEL =10 picks SMCLK as timing source

    TB0CTL |= MC__UP;                // compare setting

    //4. SW RESET
    UCA1CTLW0 &= ~UCSWRST;

    // 5. IRQs
    // TB0:
    TB0CCR0 = 4680;             // count to set the time to 25rpm
    TB0CCTL0 |= CCIFG;           // CCIFG=0 clears interrupt flag
    TB0CCTL0 |= CCIE;            // CCIE=1 enables compare interrupt

    // SW1:
    P4IFG &= ~BIT1;             // clear interrupt flag
    P4IES |= BIT1;             // sets IRQ to high to low
    P4IE |= BIT1;               // asserts local enable
    // SW2:
    P2IFG &= ~BIT3;             // clear interrupt flag
    P2IES |= BIT3;             // sets IRQ to high to low
    P2IE |= BIT3;               // asserts local enable

    // 6. GLOBAL INTERRUPT AND HIGH Z
    __enable_interrupt();
    PM5CTL0 &= ~LOCKLPM5;

    return 0;
}
//--------------- End Init ---------------------------------------

//--------------- rotateCW/CCW ---------------------------------------
// Rotates the motor CW or CCW by powering one output at a time.
//----------------------------------------------------------------

//--------------- rotateCW ---------------------------------------

int rotateCW(void){
    if(count<=fspin){
        if(count%4==1){
            P3OUT |= BIT0;
            P3OUT &= ~BIT3;
        }else if(count%4==2){
            P3OUT |= BIT1;
            P3OUT &= ~BIT0;
        }else if(count%4==3){
            P3OUT |= BIT2;
            P3OUT &= ~BIT1;
        }else{
            P3OUT |= BIT3;
            P3OUT &= ~BIT2;
        }
    }else{
        P3OUT |= BIT0;
        P3OUT &= ~BIT1;
        P3OUT &= ~BIT2;
        P3OUT &= ~BIT3;
        dir = 3;
    }
    count++;
    return 0;
}

//--------------- End rotateCW -------------------------------------

//--------------- rotateCCW ----------------------------------------

int rotateCCW(void){
    if(count<=rspin){
        if(count%4==1){
            P3OUT |= BIT3;
            P3OUT &= ~BIT0;
        }else if(count%4==2){
            P3OUT |= BIT2;
            P3OUT &= ~BIT3;
        }else if(count%4==3){
            P3OUT |= BIT1;
            P3OUT &= ~BIT2;
        }else{
            P3OUT |= BIT0;
            P3OUT &= ~BIT1;
        }
    }else{
        P3OUT |= BIT0;
        P3OUT &= ~BIT1;
        P3OUT &= ~BIT2;
        P3OUT &= ~BIT3;
        dir = 3;
    }
    count++;
    return 0;
}

//--------------- End rotateCCW --------------------------------------

//--------------- End SUBROUTINES ------------------------------------

//--------------------------------------------------------------------
// INTERRUPT SERVICE ROUTINES
//--------------------------------------------------------------------

//--------------- Port4_S1 ----------------------------
// s1 isr... starts moving forward with half time
#pragma vector=PORT4_VECTOR
__interrupt void ISR_Port4_S1(void){
    dir = 0;
    count = 1;
    position = 0;
    endof = sizeof(message1)-1;
    TB0CCR0 = 24500;             // move slower forward

    UCA1IE |= UCTXCPTIE;
    UCA1IFG &= ~UCTXCPTIFG;
    UCA1TXBUF = message1[position];
    P4IFG &= ~BIT1;
    for(i=0; i<10000; i=i+1){
         //prevent overwrite on buffer with delay
     }
}
//--------------- End Port4_S1 ----------------------------

//--------------- Port2_S2 ----------------------------
// s2 isr... starts moving backward with quarter time
#pragma vector=PORT2_VECTOR
__interrupt void ISR_Port2_S2(void){
    dir = 1;
    count = 1;
    position = 0;
    endof = sizeof(message2)-1;
    TB0CCR0 = 4900;              //motor reverse at ~25RPM

    UCA1IE |= UCTXCPTIE;
    UCA1IFG &= ~UCTXCPTIFG;
    UCA1TXBUF = message2[position];

    P2IFG &= ~BIT3;
    //for(i=0; i<1000; i=i+1){
         //prevent overwrite on buffer with delay
     //}
}
//--------------- End Port2_S2 ----------------------------

//--------------- EUSCI_A1 ----------------------------
// ucaifg tells when buffer is ready to transmit new char
// if all chars sent, then disables irq, otherwise, sends message
#pragma vector=EUSCI_A1_VECTOR
__interrupt void ISR_EUSCI_A1(void){

    //Print either message 1 or 2 by iterating through array
    if(position == endof){
        UCA1IE &= ~UCTXCPTIE;
    }else{
        position++;
        if(dir==0){
            UCA1TXBUF = message1[position];
        }else{
            UCA1TXBUF = message2[position];
        }
    }

    UCA1IFG &= ~UCTXCPTIFG;
}
//--------------- End EUSCI_A1 ----------------------------

//--------------- ISR_TBO_CCR0 ----------------------------
// will step the motor
#pragma vector=TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void){
    if(dir==0){
        rotateCW();
    }else if(dir==1){
        rotateCCW();
    }

    TB0CCTL0 &= ~CCIFG;                 // clear ifg
}
//------------- End ISR_TBO_CCR0 --------------------------

//--------------- End ISRs --------------------------------
