// MSP430 C Code Template for use with TI Code Composer Studio
// Emma Stensland, EELE371, Final Project 3
// November 13, 2024

#include <msp430.h> 

int i, count;
int dir=3;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    //1. Configure ports
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

    // LED 1-4: P1.0
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

    // tb0 clock setup small enough so can have a valid compare value
    TB0CTL |= TBCLR;                 // TBCLR=1 clears timers and dividers
    TB0CTL |= TBSSEL__SMCLK;         // TBSSEL =10 picks SMCLK as timing source

    TB0CTL |= MC__UP;                // compare setting
    TB0CTL |= ID__8;                 // choose divider D1 (8)
    TB0EX0 |= TBIDEX__7;             // choose divider d2 (7)

    // set up compare value so that flag triggers at 0.75s
    //TODO -- update timer
    TB0CCR0 = 13393;
    // tb0 flag setup
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

    __enable_interrupt();
    PM5CTL0 &= ~LOCKLPM5;               // clear high z

    while(1){
    }

    return 0;
}
//--------------- End MAIN ----------------------------------
//--------------------------------------------------------------------
// Subroutines
//
// Keeps the leds doing a rlc/rrc, depending on switch pressed
//
//--------------------------------------------------------------------

int rotateCW(void){
    if(count<=12){
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
    }
    count++;
    return 0;
}

int rotateCCW(void){
    if(count<=16){
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
    }
    count++;
    return 0;
}

//---------------- End Subroutines --------------
//--------------------------------------------------------------------
// Interrupt Service Routines
//--------------------------------------------------------------------

//--------------- Port4_S1 ----------------------------
// s1 isr... sets position to zero and puts first char
// initializes uca1 ifg, and lights led1 on
#pragma vector=PORT4_VECTOR
__interrupt void ISR_Port4_S1(void){
    dir = 0;
    count = 1;

    P4IFG &= ~BIT1;
    for(i=0; i<100; i=i+1){
         //prevent overwrite on buffer with delay
     }
}
//--------------- End Port4_S1 ----------------------------

//--------------- Port2_S2 ----------------------------
// s2 isr... sets position to zero and puts first char
// initializes uca1 ifg, and lights led2 on
#pragma vector=PORT2_VECTOR
__interrupt void ISR_Port2_S2(void){
    dir = 1;
    count = 1;

    P2IFG &= ~BIT3;
    for(i=0; i<100; i=i+1){
         //prevent overwrite on buffer with delay
     }
}
//--------------- End Port2_S2 ----------------------------

//--------------- EUSCI_A1 ----------------------------
// every 1s, rotates
#pragma vector=TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void){
    if(dir==1){
        rotateCW();
    }else if(dir==0){
        rotateCCW();
    }

    TB0CCTL0 &= ~CCIFG;                 // clear ifg
}
//--------------- End EUSCI_A1 ----------------------------

//--------------- End ISRs ----------------------------
