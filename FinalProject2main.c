// MSP430 C Code Template for use with TI Code Composer Studio
// Emma Stensland, EELE371, 15.1Lab
// October 28, 2024

#include <msp430.h> 

int Data_Cnt = 0;
char Start_Packet[] = {0x03, 0x00, 0x00, 0x12, 0x08, 0x05, 0x11, 0x24};
char Status_Packet[] = {0, 0, 0, 0, 0, 0, 0};
char seconds = 0;
char minutes = 0;
int i;
int initalized = 0;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    // 1. put eUSCI_B0 into sw reset
    UCB1CTLW0 |= UCSWRST;       // ucswrst=1 for eUSCI_B0 in sw reset

    // 2. configure eUSCI_B0 (master clock/scl, then the mode)
    UCB1CTLW0 |= UCSSEL__SMCLK;      // choose brclk=smclk=1MHz
    UCB1BRW = 10;               // divide brclk by 10 so scl=100kHz

    // eUSCI_B0 is i2c master that goes to slave with addr 0x68
    UCB1CTLW0 |= UCMODE_3;      // put into I2C mode
    UCB1CTLW0 |= UCMST;         // master mode
    UCB1I2CSA = 0x0068;          // slave address = 0x68

    UCB1CTLW1 |= UCASTP_2;                    // auto stop when ucb0
    UCB1TBCNT = sizeof(Start_Packet);         // sends number of bytes in packet

    // 3. configure ports
    P4SEL1 &= ~BIT7;            // we want p4.7 = scl
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;            // we want p4.6 = sda
    P4SEL0 |= BIT6;

    // S1: P4.1 Input
    P4DIR &= ~BIT1;             // set as input
    P4REN |= BIT1;              // enable resitor
    P4OUT |= BIT1;              // pull up resistor

    PM5CTL0 &= ~LOCKLPM5;

    // 4. take eUSCI_B0 out of sw reset
    UCB1CTLW0 &= ~UCSWRST;      // UCSWRST=1 for eUSCI_B0 in SW reset

    // 5. enable interrupts
    // SW1:
    P4IFG &= ~BIT1;             // clear interrupt flag
    P4IES |= BIT1;             // sets IRQ to high to low
    P4IE |= BIT1;               // asserts local enable
    // I2C:
    UCB1IE |= UCTXIE0;          // enable I2C Rx0 IRQ
    UCB1IE |= UCRXIE0;          // enable I2C Tx0 IRQ

    __enable_interrupt();       // enable maskable IRQs

    // Initialize First Value:
    UCB1CTLW0 |= UCTR;          // put into Tx mode
    UCB1CTLW0 |= UCTXSTT;       // start condition

    while((UCB1IFG & UCSTPIFG) ==0);    // wait for STOP condition
        UCB1IFG &= ~UCSTPIFG;               // clear stop flag

    // signal that we initialized are reset counter
    initalized = 1;
    Data_Cnt=0;

    while(1){
        // Transmit Register addr with write message

        UCB1TBCNT = 0x01;         // sends number of bytes in packet
        UCB1CTLW0 |= UCTR;                  // put into Tx mode
        UCB1CTLW0 |= UCTXSTT;               // generate start condition

        while((UCB1IFG & UCSTPIFG) ==0);    // wait for STOP condition
            UCB1IFG &= ~UCSTPIFG;               // clear stop flag

        UCB1TBCNT = sizeof(Status_Packet);         // sends number of bytes in packet

        // recieve data from Rx
        UCB1CTLW0 &= ~UCTR;                 // put into Rx mode
        UCB1CTLW0 |= UCTXSTT;               // generate start conditions

        while((UCB1IFG & UCSTPIFG)==0);     // wait for stop
            UCB1IFG &= ~UCSTPIFG;               //clear stop flag
    }

    return 0;
}
//--------------- End MAIN ----------------------------------
//--------------------------------------------------------------------
// Transmit Subroutine
// Initializes values to transmit the read start and
//--------------------------------------------------------------------

//TODO -- fix the weird halting that happens
int transmit(void){
    // Transmit Register addr with write message

    UCB1TBCNT = 0x01;         // sends number of bytes in packet
    UCB1CTLW0 |= UCTR;                  // put into Tx mode
    UCB1CTLW0 |= UCTXSTT;               // generate start condition

    while((UCB1IFG & UCSTPIFG) ==0);    // wait for STOP condition
        UCB1IFG &= ~UCSTPIFG;               // clear stop flag

    UCB1TBCNT = sizeof(Status_Packet);         // sends number of bytes in packet

    // recieve data from Rx
    UCB1CTLW0 &= ~UCTR;                 // put into Rx mode
    UCB1CTLW0 |= UCTXSTT;               // generate start conditions

    while((UCB1IFG & UCSTPIFG)==0);     // wait for stop
        UCB1IFG &= ~UCSTPIFG;               //clear stop flag

    return 0;
}
//--------------- End Transmit
//--------------------------------------------------------------------
// Interrupt Service Routines
//--------------------------------------------------------------------

//--------------- Port4_S1 ----------------------------
// s1 isr... takes 1 reading of the current time by tx where to start reading then rx to recieve time
#pragma vector=PORT4_VECTOR
__interrupt void ISR_Port4_S1(void){
    transmit();

    P4IFG &= ~BIT1;                 // clear ifg
    for(i=0; i<10000; i=i+1){
         //ignore switch bounce
     }
}
//--------------- End Port4_S1 ----------------------------


//--------------- EUSCI_A1 ----------------------------

// when rx buffer is ready for data, this is triggered
#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    // switch case determines which flag was triggered
    switch(UCB1IV){
    case 0x18:                      // id 16: TXIFG0
        if(initalized==1){
            UCB1TXBUF = 0x03;            // start reading at 0x03 register
        }else{
            // feeds next part of packet like normal, which puts date in RTC
            if(Data_Cnt == (sizeof(Start_Packet) -1)){
                UCB1TXBUF = Start_Packet[Data_Cnt];
                Data_Cnt = 0;
            }else{
                UCB1TXBUF = Start_Packet[Data_Cnt];
                Data_Cnt++;
            }
        }
        break;
    case 0x16:                      // id 18: RXIFG0
        // recieves next part of RTC, which is put into packet
        // if it is the seconds or minutes value, it is put into additonal vars
        if(Data_Cnt == (sizeof(Status_Packet) -1)){
            Status_Packet[Data_Cnt] = UCB1RXBUF;
            Data_Cnt = 0;
        }else{
            Status_Packet[Data_Cnt] = UCB1RXBUF;
            if(Data_Cnt==0){
                seconds = Status_Packet[Data_Cnt];
            }else if(Data_Cnt==1){
                minutes = Status_Packet[Data_Cnt];
            }
            Data_Cnt++;
        }
        break;
    default:
        break;
    }
}

//--------------- End EUSCI_A1 ----------------------------

//--------------- End ISRs ----------------------------
