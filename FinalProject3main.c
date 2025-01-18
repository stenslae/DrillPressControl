// MSP430 C Code Template for use with TI Code Composer Studio
// Emma Stensland, EELE371, Final Project 3
// November 13, 2024

#include <msp430.h> 

char message1[] = "\n\r forward \r\n";
char message2[] = "\n\r reverse \r\n";
unsigned int endof;
unsigned int position;
int i;
int det;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    //1. Put eUSCI_A1 into SW reset
    UCA1CTLW0 |= UCSWRST;

    //2. Configure eUSCI_A1, baud rate set for 57600
    UCA1CTLW0 |= UCSSEL__SMCLK;

    UCA1BRW = 17;
    UCA1MCTLW |= 0x4A00;

    //3. Configure ports
    P4DIR &= ~BIT1;
    P4REN |= BIT1;
    P4OUT |= BIT1;
    P4IES |= BIT1;

    P2DIR &= ~BIT3;
    P2REN |= BIT3;
    P2OUT |= BIT3;
    P2IES |= BIT3;

    // Red LED 1: P1.0
    // set p1 to dig i/o (sel0&1 = 0)
    P1SEL0 = 0;
    P1SEL1 = 0;
    // set p1 to an output at bit 0
    P1DIR |= BIT0;
    // clear output at bit 0
    P1OUT &= ~BIT0;

    // Green LED 2: P6.6
    // set p6 to dig i/o (sel0&1 = 0)
    P6SEL0 = 0;
    P6SEL1 = 0;
    // set p6 to an output at bit 6
    P6DIR |= BIT6;
    // clear output at bit 6
    P6OUT &= ~BIT6;

    P4SEL1 &= ~BIT3;
    P4SEL0 |= BIT3;

    //4. take eUSCI_A1 out of software reset
    UCA1CTLW0 &= ~UCSWRST;

    //5. enable IRQs
    P4IFG &= ~BIT1;
    P4IE |= BIT1;
    P2IFG &= ~BIT3;
    P2IE |= BIT3;

    __enable_interrupt();
    PM5CTL0 &= ~LOCKLPM5;               // clear high z

    while(1){
    }

    return 0;
}
//--------------- End MAIN ----------------------------------
//--------------------------------------------------------------------
// Interrupt Service Routines
//--------------------------------------------------------------------

//--------------- Port4_S1 ----------------------------
// s1 isr... sets position to zero and puts first char
// initializes uca1 ifg, and lights led1 on
#pragma vector=PORT4_VECTOR
__interrupt void ISR_Port4_S1(void){
    det = 1;
    position = 0;
    endof = sizeof(message1)-1;

    UCA1IE |= UCTXCPTIE;
    UCA1IFG &= ~UCTXCPTIFG;
    UCA1TXBUF = message1[position];

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
    det = 0;
    position = 0;
    endof = sizeof(message2)-1;

    UCA1IE |= UCTXCPTIE;
    UCA1IFG &= ~UCTXCPTIFG;
    UCA1TXBUF = message2[position];

    P2IFG &= ~BIT3;
    for(i=0; i<100; i=i+1){
         //prevent overwrite on buffer with delay
     }
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
        if(det==1){
            UCA1TXBUF = message1[position];
        }else{
            UCA1TXBUF = message2[position];
        }
    }

    UCA1IFG &= ~UCTXCPTIFG;
}
//--------------- End EUSCI_A1 ----------------------------

//--------------- End ISRs ----------------------------
