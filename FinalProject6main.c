//--------------------------------------------------------------------
// MSP430 C Code Template for use with TI Code Composer Studio
// Emma Stensland, EELE371, Final Project 6
// December 6, 2024
//--------------------------------------------------------------------
// PROJECT DESCRIPTION:
// Pressure sensor input is converted with ADC and triggers a warning
// when reaching a threshold. The time the input passes the high threshold
// is saved using I2C.
//--------------------------------------------------------------------
// PINOUT on MSP430:
// ADC Input Pin: 1.3
// I2C SCL Pin: 4.6
// I2C SDA Pin: 4.7
// LED 1 Pin: 1.0
// LED 2 Pin: 6.6

//--------------------------------------------------------------------
// CALCULATIONS:
// High Threshold: (40lbs*(40mV/lb))=1600mV --> ADC Value adjusted and set at 2030
// Low Threshold: (30lbs*(40mV/lb))=1200mV --> ADC Value adjusted and set at 1510
// High Threshold set at 40lbs, as this is above typical value but not at the breaking point yet
// Low Threshold set at 30lbs, because going above this is atypical and gives sufficient warning to user to stop
//--------------------------------------------------------------------

#include <msp430.h> 

// TODO -- Update with current time:
// addr, seconds, minutes, hours, days, weekday, month, year
char Start_Packet[] = {0x03, 0x00, 0x00, 0x12, 0x08, 0x05, 0x11, 0x24};

// Declare Variables/Subroutines:

// I/O Variables
unsigned int ADC_Value;
char Status_Packet[] = {0, 0, 0, 0, 0, 0, 0};

// Subroutines
int init(void);
int transmit(void);

// Loop/Status Variables
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
        ADCCTL0 |= ADCENC | ADCSC;      // enable and start conversion
        while((ADCIFG & ADCIFG0)==0);   // wait until conv. is complete

        transmit();                     // reads current time
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
    UCB1CTLW0 |= UCSWRST;       // ucswrst=1 for eUSCI_B0 in sw reset

    // 2. CONFIGURE CLOCK
    UCB1CTLW0 |= UCSSEL__SMCLK;      // choose brclk=smclk=1MHz
    UCB1BRW = 10;               // divide brclk by 10 so scl=100kHz

    // eUSCI_B0 is i2c master that goes to slave with addr 0x68
    UCB1CTLW0 |= UCMODE_3;      // put into I2C mode
    UCB1CTLW0 |= UCMST;         // master mode
    UCB1I2CSA = 0x0068;          // slave address = 0x68

    UCB1CTLW1 |= UCASTP_2;                    // auto stop when ucb0
    UCB1TBCNT = sizeof(Start_Packet);         // sends number of bytes in packet

    // 3. CONFIGURE PORTS
    // ----- LEDS
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

    // ----- CONFIGURE ADC:
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

    // ---- I2C PINS SETUP
    P4SEL1 &= ~BIT7;            // we want p4.7 = scl
    P4SEL0 |= BIT7;

    P4SEL1 &= ~BIT6;            // we want p4.6 = sda
    P4SEL0 |= BIT6;

    // ---- CLEAR HIGH Z
    PM5CTL0 &= ~LOCKLPM5;

    // 4. SW RESET
    UCB1CTLW0 &= ~UCSWRST;      // UCSWRST=1 for eUSCI_B0 in SW reset

    // ------ CONFIGURE INTERRUPTS:
    // ADC:
    ADCIE |= ADCIE0;                    // enable adc irq
    // I2C:
    UCB1IE |= UCTXIE0;          // enable I2C Rx0 IRQ
    UCB1IE |= UCRXIE0;          // enable I2C Tx0 IRQ

    __enable_interrupt();       // enable maskable IRQs
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

//--------------- End SUBROUTINES --------------------------------

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

    // TODO -- update the adc values for this right pressure range
    if(ADC_Value>2023){           // a4 > 1600mV, the red led turns on
        // save the time if this is the first unsafe read
        if(trigger==1){
            saveTime=1;
        }
        P1OUT |= BIT0;
        P6OUT &= ~BIT6;
    }else if(ADC_Value>=1510){           // a2> 1200mV, the led turns off
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

//--------------- EUSCI_A1 ----------------------------

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

//--------------- End EUSCI_A1 ----------------------------

//--------------- End ISRs ----------------------------
