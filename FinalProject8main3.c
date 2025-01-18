//--------------------------------------------------------------------
// MSP430 C Code Template for use with TI Code Composer Studio
// Emma Stensland, EELE371, Final Project 7
// December 6, 2024
//--------------------------------------------------------------------
// PROJECT DESCRIPTION:
// Switches serve as input to decide to move motor forward or reverse,
// and UART announces the movement of the motor.
//--------------------------------------------------------------------
// PINOUT on MSP430:
// ADC Input Pin: 1.4
// I2C SCL Pin: 4.6
// I2C SDA Pin: 4.7
// LED 1 Pin: 1.0
// LED 2 Pin: 6.6
// Switch 1: 4.1
// Switch 2: 2.3
// LEDS: 3.0-3
// UART: 4.3
//--------------------------------------------------------------------

#include <msp430.h> 

// TODO -- Update with current time:
// addr, seconds, minutes, hours, days, weekday, month, year
char Start_Packet[] = {0x03, 0x00, 0x00, 0x12, 0x08, 0x05, 0x11, 0x24};

// TODO -- Update with rotation size: (513 = 360 degrees)
int rspin = 513;
int fspin = 51;
char message1[] = "\n\r Motor moved forward 36 degrees. \r\n";
char message2[] = "\n\r Motor reversed 1 rotation. \r\n";

// Declare Variables/Subroutines:

// I/O Variables
unsigned int ADC_Value;
char Status_Packet[] = {0, 0, 0, 0, 0, 0, 0};

// Subroutines
int init(void);
int rotateCW(void);
int rotateCCW(void);
int transmit(void);

// Loop/Status Variables
int i, count;
int dir=3;
unsigned int endof, position;
unsigned int Data_Cnt = 0;
volatile int timeSet = 0;
volatile int saveTime = 0;
volatile int trigger = 1;


//--------------- MAIN -------------------------------------------
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    // Initialize Pinouts:
    init();

    // Set RTC with Current Time:
    UCB1CTLW0 |= UCTR;               // put into Tx mode
    UCB1CTLW0 |= UCTXSTT;            // start condition

    while((UCB1IFG & UCSTPIFG) ==0); // wait for STOP condition
        UCB1IFG &= ~UCSTPIFG;        // clear stop flag

    timeSet = 1;
    Data_Cnt=0;                      // signal that we initialized and reset counter

    // Infinite loop
    while(1){
        transmit();                     // reads current time
    }

    return 0;
}
//--------------- End MAIN -------------------------------------------

//--------------------------------------------------------------------
// SUBROUTINES
//--------------------------------------------------------------------

//--------------- Init -----------------------------------------------
// Intialize ADC, UART, motor control, and I2C on the MSP
//--------------------------------------------------------------------

int init(void){
    // 1. SW RESET
    UCA1CTLW0 |= UCSWRST;       // uart sw reset
    UCB1CTLW0 |= UCSWRST;       // i2c sw reset

    // 2. CONFIGURE CLOCKS

    // UART
    UCA1CTLW0 |= UCSSEL__SMCLK;

    UCA1BRW = 17;
    UCA1MCTLW |= 0x4A00;            // baud 57600

    // I2C
    UCB1CTLW0 |= UCSSEL__SMCLK;      // choose brclk=smclk=1MHz
    UCB1BRW = 10;               // divide brclk by 10 so scl=100kHz

    // eUSCI_B1 is i2c master that goes to slave with addr 0x68
    UCB1CTLW0 |= UCMODE_3;      // put into I2C mode
    UCB1CTLW0 |= UCMST;         // master mode
    UCB1I2CSA = 0x0068;          // slave address = 0x68

    UCB1CTLW1 |= UCASTP_2;                    // auto stop
    UCB1TBCNT = sizeof(Start_Packet);         // sends number of bytes in packet

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
    // Red LED 1: P1.0
    // set p1 to dig i/o (sel0&1 = 0)
    P1SEL0 = 0;
    P1SEL1 = 0;
    // set p1 to an output at bit 0
    P1DIR |= BIT0;
    // set output at bit 0
    P1OUT &= ~BIT0;

    // Green LED 2: P6.6
    // set p6 to dig i/o (sel0&1 = 0)
    P6SEL0 = 0;
    P6SEL1 = 0;
    // set p6 to an output at bit 6
    P6DIR |= BIT6;
    // clear output at bit 6
    P6OUT &= ~BIT6;

    // MOTOR CONTROL
    // P3.0-3
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

    // CONFIGURE ADC:
    P1SEL1 |= BIT4;                     // configure p1.4 pin for a4
    P1SEL0 |= BIT4;

    ADCCTL0 &= ~ADCSHT;                 // clear adcsht from def
    ADCCTL0 |= ADCSHT_2;                // conversion cycles = 16
    ADCCTL0 |= ADCON;                   // turn adc on

    ADCCTL1 |= ADCSSEL_2;               // adc clock source = smclk
    ADCCTL1 |= ADCSHP;                  // sample signal source= sampling timer

    ADCCTL2 &= ~ADCRES;                 // clear adcres from def of adcres=01
    ADCCTL2 |= ADCRES_2;                // resolution = 12bit (ADCRES=10)

    ADCMCTL0 |= ADCINCH_4;              // adc input channel = A4 (P1.4)

    // I2C PINS SETUP
    P4SEL1 &= ~BIT7;            // we want p4.7 = scl
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;            // we want p4.6 = sda
    P4SEL0 |= BIT6;

    //4. SW RESET
    UCA1CTLW0 &= ~UCSWRST;      // uart sw reset
    UCB1CTLW0 &= ~UCSWRST;      // i2c sw reset

    // 5. IRQs
    // ADC:
    ADCIE |= ADCIE0;                    // enable adc irq

    // I2C:
    UCB1IE |= UCTXIE0;          // enable I2C Rx0 IRQ
    UCB1IE |= UCRXIE0;          // enable I2C Tx0 IRQ

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

//--------------- Transmit ---------------------------------------
// Start the reading of the time on the RTC by using I2C
//----------------------------------------------------------------

int transmit(void){
    // Transmit Register addr with write message
    timeSet=1;
    UCB1TBCNT = 0x01;         // sends number of bytes in packet
    UCB1CTLW0 |= UCTR;               // put into Tx mode
    UCB1CTLW0 |= UCTXSTT;            // start condition

    while((UCB1IFG & UCSTPIFG) ==0); // wait for STOP condition
        UCB1IFG &= ~UCSTPIFG;        // clear stop flag

    UCB1TBCNT = sizeof(Status_Packet);         // sends number of bytes in packet

    // recieve data from Rx
    UCB1CTLW0 &= ~UCTR;                 // put into Rx mode
    UCB1CTLW0 |= UCTXSTT;               // generate start conditions

    while((UCB1IFG & UCSTPIFG)==0);     // wait for stop
        UCB1IFG &= ~UCSTPIFG;               //clear stop flag

    return 0;
}

//--------------- End Transmit ---------------------------------------

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

//------- ADC_ISR ----------------------------------------------------

//A voltage reading is found from pin 1.4
//When the reading is below 1200mV, the Green LED indicates a “safe” operating condition.
//When the reading surpasses 1200mV, the LEDs turn off to indicate “warning” state.
//When the reading surpasses 1600mV, the Red LED indicates an “unsafe” operating condition.

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void){
    ADC_Value = ADCMEM0;                // read adc value

    if(ADC_Value>2039){           // a4 > 1600mV, the red led turns on
        // save the time if this is the first unsafe read
        if(trigger==1){
            saveTime=1;
        }
        P1OUT |= BIT0;
        P6OUT &= ~BIT6;
    }else if(ADC_Value>=1530){           // a2> 1200mV, the led turns off
        trigger=1;
        P1OUT &= ~BIT0;
        P6OUT &= ~BIT6;
    }else{                              // a2 <= 1200mV, the green led is on
        trigger=1;
        P1OUT &= ~BIT0;
        P6OUT |= BIT6;
    }
}
//------- End ADC_ISR ---------------------------

//--------------- Port4_S1 ----------------------------
// s1 isr... starts moving forward with half time
#pragma vector=PORT4_VECTOR
__interrupt void ISR_Port4_S1(void){
    dir = 0;
    count = 1;
    position = 0;
    endof = sizeof(message1)-1;
    TB0CCR0 = 4280;             // move slower forward

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
    TB0CCR0 = 2700;              //motor reverse at ~25RPM

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

//--------------- EUSCI_B1 ----------------------------

// When buffer is ready, this is triggered to read or transmit to RTC

#pragma vector=EUSCI_B1_VECTOR
__interrupt void EUSCI_B1_I2C_ISR(void){
    // switch case determines which flag was triggered
    switch(UCB1IV){
    case 0x18:                      // id 18: TXIFG0
        // checks to see if the current date is needed to be put in
        if(timeSet==1){
            UCB1TXBUF = 0x03;            // start reading at 0x03 register
            break;
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
    case 0x16:                      // id 16: RXIFG0
        // only happens when pressure is triggered high
        // recieves current timestamp and saves into Status Packet
        if(Data_Cnt == (sizeof(Status_Packet) -1)){
            if(saveTime==1){
                Status_Packet[Data_Cnt] = UCB1RXBUF;
                saveTime=0;
                trigger=0;
            }else{
                UCB1RXBUF;
            }
            Data_Cnt = 0;
        }else{
            if(saveTime==1){
                Status_Packet[Data_Cnt] = UCB1RXBUF;
            }else{
                UCB1RXBUF;
            }
            Data_Cnt++;
        }
        break;
    default:
        break;
    }
}

//--------------- End EUSCI_B1 ----------------------------

//--------------- ISR_TBO_CCR0 ----------------------------
// will step the motor
#pragma vector=TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void){

    ADCCTL0 |= ADCENC | ADCSC;      // enable and start conversion
    while((ADCIFG & ADCIFG0)==0);   // wait until conv. is complete

    if(dir==0){
        rotateCW();
    }else if(dir==1){
        rotateCCW();
    }

    TB0CCTL0 &= ~CCIFG;                 // clear ifg
}
//------------- End ISR_TBO_CCR0 --------------------------

//--------------- End ISRs --------------------------------
