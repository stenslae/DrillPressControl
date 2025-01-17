//--------------------------------------------------------------------
// MSP430 C Code Template for use with TI Code Composer Studio
// Emma Stensland, EELE371, Final Project FINAL VERSION
// December 6, 2024
//--------------------------------------------------------------------
// PROJECT DESCRIPTION:
// Moves the motor forward/reversed and sends a message to the serial terminal.
// Collects and Records an ADC reading from the AD2/sensor and illuminates the appropriate LED.
// If the system is within the unsafe operation zone, collect an RTC timestamp and sends to the serial terminal.
// ADC has rolling average implemented, and if system hits maximum pressure, the drill can't apply more pressure.
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
// ALARM: 3.4
// UART: 4.3
//--------------------------------------------------------------------

#include <msp430.h> 

// -- Update with current time:
// addr, seconds, minutes, hours, days, weekday, month, year
char Start_Packet[] = {0x03, 0x00, 0x14, 0x12, 0x06, 0x03, 0x12, 0x24};

// -- Update with rotation size: (513 = 360 degrees, 51 = 36 degrees)
int rspin = 513;
int fspin = 51;
char message1[] = "\n\r Motor moved forward 36 degrees. \r\n";
char message2[] = "\n\r Motor reversed 1 rotation. \r\n";

// -- Update with rotation speed:
// 50 rpm for 51 steps, width of 25.53 ms
int fspeed = 24500;
// 25 rpm for 513 steps, width of 4.68 ms
int rspeed = 4900;

// Declare Variables/Subroutines:

// I/O Variables
unsigned int ADC_Value;
unsigned int AVE_Value;
unsigned int ADC_Values[20];
char Status_Packet[] = {0, 0, 0, 0, 0, 0, 0};
char message3[] = "\n\r Drill pressed into unsafe conditions at ";
char message4[] = "\n\r Alert! Alert! Pressure too high, drill is disabled. \n\r";

// Subroutines
int init(void);
int rotateCW(void);
int rotateCCW(void);
int transmit(void);
int uartWarning(void);
int adcStatus(void);
int switch1Pressed(void);
int switch2Pressed(void);
int adcAverage(void);

// Loop Variables
int i, count;
unsigned int endof, position;
unsigned int Data_Cnt = 0;
unsigned long int total=0;
int index=0;
int width=0;

//Flags
volatile int printWarning = 0;
volatile int saveTime = 0;
volatile int trigger = 1;
volatile int trigger2 = 1;
volatile int timeSet = 0;
volatile int timeReady = 0;
volatile int adcReady = 0;
volatile int switch1 = 0;
volatile int switch2 = 0;
volatile int dir=3;


//--------------- MAIN -------------------------------------------
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    // Initialize Pins:
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

        // if done collecting the timestamp of warning, will send the timestamp.
        if(adcReady == 1){
            adcAverage();
            adcStatus();
        }

        // if warning triggered, will read the time
        if(saveTime==1){
            transmit();
        }
        // if done collecting the timestamp of warning, will send the timestamp.
        if(printWarning == 1){
            uartWarning();
        }

        // if the switch 1 pressed signal motor needs to be moved
        if(switch1==1){
            switch1Pressed();
        }
        // if the switch 2 pressed signal motor needs to be moved
        if(switch2==1){
            switch2Pressed();
        }
        // if the clock is high, checks if motor needs to be moved
        if(timeReady == 1){
            timeReady = 0;
            if(dir==0){
                rotateCW();
            }else if(dir==1){
                rotateCCW();
            }
        }
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

    // MOTOR CONTROL & ALARM
    // P3.0-3
    // set p3 to dig i/o (sel0&1 = 0)
    P3SEL0 = 0;
    P3SEL1 = 0;
    // set p1 to an output at bit 0
    P3DIR |= BIT0;
    P3DIR |= BIT1;
    P3DIR |= BIT2;
    P3DIR |= BIT3;
    P3DIR |= BIT4;
    // set/clear outputs
    P3OUT |= BIT0;
    P3OUT &= ~BIT1;
    P3OUT &= ~BIT2;
    P3OUT &= ~BIT3;
    P3OUT &= ~BIT4;

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
    ADCCTL2 |=

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
    TB0CCR0 = fspeed;
    TB0CCTL0 |= CCIFG;           // CCIFG=0 clears interrupt flag
    TB0CCTL0 |= CCIE;            // CCIE=1 enables compare interrupt
    // TB1: (for ADC)
    TB0CCR1 = 300;
    TB0CCTL1 |= CCIFG;           // CCIFG=0 clears interrupt flag
    TB0CCTL1 |= CCIE;            // CCIE=1 enables compare interrupt

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
    // reset flag
    saveTime==0;
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

//--------------- uartWarning ----------------------------------------
// Outputs message to serial terminal with timestamp of drill press.
//--------------------------------------------------------------------

int uartWarning(void){
    // reset flag
    printWarning = 0;
    // print first part of message
    dir = 4;
    endof = sizeof(message3)-1;
    position = 0;
    UCA1IE |= UCTXCPTIE;
    UCA1IFG &= ~UCTXCPTIFG;
    UCA1TXBUF = message3[position];
    for (i = 0; i < 1000; i++){
    }
    // print hours
    UCA1TXBUF = ((Status_Packet[2] & 0xF0)>>4) + '0'; // Prints the 10s digit
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = (Status_Packet[2] & 0x0F) + '0'; // Prints the 1s digit
    for (i = 0; i < 500; i++){
    }
    // print spacer
    UCA1TXBUF = ':';
    for (i = 0; i < 500; i++){
    }
    // print minutes
    UCA1TXBUF = ((Status_Packet[1] & 0xF0)>>4) + '0'; // Prints the 10s digit
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = (Status_Packet[1] & 0x0F) + '0'; // Prints the 1s digit
    for (i = 0; i < 500; i++){
    };
    // print spacer
    UCA1TXBUF = ':';
    for (i = 0; i < 500; i++){
    }
    // print seconds
    UCA1TXBUF = ((Status_Packet[0] & 0xF0)>>4) + '0'; // Prints the 10s digit
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = (Status_Packet[0] & 0x0F) + '0'; // Prints the 1s digit
    for (i = 0; i < 500; i++){
    }

    // print spacer
    UCA1TXBUF = ' ';
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = 'o';
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = 'n';
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = ' ';
    for (i = 0; i < 500; i++){
    }

    // print month
    UCA1TXBUF = ((Status_Packet[5] & 0xF0)>>4) + '0'; // Prints the 10s digit
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = (Status_Packet[5] & 0x0F) + '0'; // Prints the 1s digit
    for (i = 0; i < 500; i++){
    }
    // print spacer
    UCA1TXBUF = '/';
    for (i = 0; i < 500; i++){
    }
    // print day
    UCA1TXBUF = ((Status_Packet[3] & 0xF0)>>4) + '0'; // Prints the 10s digit
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = (Status_Packet[3] & 0x0F) + '0'; // Prints the 1s digit
    for (i = 0; i < 500; i++){
    }
    // print spacer
    UCA1TXBUF = '/';
    for (i = 0; i < 500; i++){
    }
    // print year
    UCA1TXBUF = ((Status_Packet[6] & 0xF0)>>4) + '0'; // Prints the 10s digit
    for (i = 0; i < 500; i++){
    }
    UCA1TXBUF = (Status_Packet[6] & 0x0F) + '0'; // Prints the 1s digit
    for (i = 0; i < 500; i++){
    }

    // print end
    UCA1TXBUF = '\n'; // Newline character
    for (i=0; i<100; i++){
    }
    UCA1TXBUF = '\r'; // Carriage return (align-L)


    return 0;
}

//--------------- End uartWarning --------------------------------------

//--------------- adcAverage ----------------------------------------
// Implements a rolling average of the past 20 values to reduce adc noise
//--------------------------------------------------------------------

int adcAverage(void){
    // reset flag
    adcReady = 0;

    // remove old value from array
    total -= ADC_Values[index];

    // put new value into array
    ADC_Values[index] = ADC_Value;
    total += ADC_Values[index];

    // update the index to go to oldest value
    index = (index+1)%20;

    // increase the width to the max size of the array
    if(width<20){
        width++;
    }

    // calculate the average
    AVE_Value = total/width;

    return 0;
}

//--------------- end adcAverage ----------------------------------------

//--------------- adcStatus ----------------------------------------
//When the reading is below 1200mV, the Green LED indicates a “safe” operating condition.
//When the reading surpasses 1200mV, the LEDs turn off to indicate “warning” state.
//When the reading surpasses 1600mV, the Red LED indicates an “unsafe” operating condition.
//--------------------------------------------------------------------

int adcStatus(void){
    adcReady = 0;
    if(AVE_Value>=2560){                      // if over 50lbs, emergency shuoff
        if(trigger2==1){
            trigger2=0;
            dir = 5;
            position = 0;
            endof = sizeof(message4)-1;
            P4IE &= ~BIT1;               // asserts local enable
            UCA1IE |= UCTXCPTIE;
            UCA1IFG &= ~UCTXCPTIFG;
            UCA1TXBUF = message4[position];
        }
        P3OUT |= BIT4;
    }else if(AVE_Value<=2420 && AVE_Value>=2030){            // a4 > 1600mV, the red led turns on
        // save the time if this is the first unsafe read
        if(trigger==1){
            saveTime=1;
            trigger=0;
        }
        P4IE |= BIT1;               // asserts local enable
        trigger2=1;
        P1OUT |= BIT0;
        P6OUT &= ~BIT6;
        P3OUT &= ~BIT4;
    }else if(AVE_Value<=1960 && AVE_Value>=1500){           // a2> 1200mV, the led turns off
        trigger=1;
        trigger2=1;
        P4IE |= BIT1;               // asserts local enable
        P1OUT &= ~BIT0;
        P6OUT &= ~BIT6;
        P3OUT &= ~BIT4;
    }else if(AVE_Value<1450){                  // a2 <= 1200mV, the green led is on
        trigger=1;
        trigger2=1;
        P4IE |= BIT1;               // asserts local enable
        P1OUT &= ~BIT0;
        P6OUT |= BIT6;
        P3OUT &= ~BIT4;
    }
    return 0;
}

//--------------- end adcStatus ----------------------------------------

//--------------- switch1Pressed -------------------------------------
// Rotates the motor CW or CCW by powering one output at a time.
//--------------------------------------------------------------------

int switch1Pressed(void){
    switch1=0;
    dir = 0;
    count = 1;
    position = 0;
    endof = sizeof(message1)-1;
    TB0CCR0 = fspeed;             // move slower forward

    UCA1IE |= UCTXCPTIE;
    UCA1IFG &= ~UCTXCPTIFG;
    UCA1TXBUF = message1[position];

    for(i=0; i<10000; i=i+1){
         //prevent overwrite on buffer with delay
     }
    return 0;
}

//--------------- End switch1Pressed ----------------------------------

//--------------- switch2 --------------------------------------------
// Rotates the motor CW or CCW by powering one output at a time.
//--------------------------------------------------------------------

int switch2Pressed(void){
    switch2 = 0;
    dir = 1;
    count = 1;
    position = 0;
    endof = sizeof(message2)-1;
    TB0CCR0 = rspeed;              //motor reverse at ~25RPM

    UCA1IE |= UCTXCPTIE;
    UCA1IFG &= ~UCTXCPTIFG;
    UCA1TXBUF = message2[position];

    for(i=0; i<10000; i=i+1){
         //prevent overwrite on buffer with delay
     }
    return 0;
}

//--------------- End switch2Pressed ---------------------------------

//--------------- End SUBROUTINES ------------------------------------

//--------------------------------------------------------------------
// INTERRUPT SERVICE ROUTINES
//--------------------------------------------------------------------

//--------------- ISR_TBO_CCR0 ----------------------------
// will step the motor
#pragma vector=TIMER0_B0_VECTOR
__interrupt void ISR_TB0_CCR0(void){
    timeReady = 1;

    TB0CCTL0 &= ~CCIFG;                 // clear ifg
}
//------------- End ISR_TBO_CCR0 --------------------------

//--------------- ISR_TBO_CCR0 ----------------------------
// read the ADC value
#pragma vector=TIMER0_B1_VECTOR
__interrupt void ISR_TB0_CCR1(void){
    // Take ADC reading
    ADCCTL0 |= ADCENC | ADCSC;      // enable and start conversion
    while((ADCIFG & ADCIFG0)==0);   // wait until conv. is complete

    TB0CCTL1 &= ~CCIFG;                 // clear ifg
}
//------------- End ISR_TBO_CCR0 --------------------------

//------- ADC_ISR ----------------------------------------------------

//A voltage reading is found from pin 1.4

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void){
    ADC_Value = ADCMEM0;                // read adc value
    adcReady = 1;
}
//------- End ADC_ISR ---------------------------

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
        }else if(dir==4){
            UCA1TXBUF = message3[position];
        }else if(dir==1){
            UCA1TXBUF = message2[position];
        }else if(dir==5){
            UCA1TXBUF = message4[position];
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
            Status_Packet[Data_Cnt] = UCB1RXBUF;
            saveTime=0;
            printWarning=1;
            Data_Cnt = 0;
        }else{
            Status_Packet[Data_Cnt] = UCB1RXBUF;
            Data_Cnt++;
        }
        break;
    default:
        break;
    }
}

//--------------- End EUSCI_B1 ----------------------------

//--------------- Port4_S1 ----------------------------
// s1 isr... starts moving forward with half time
#pragma vector=PORT4_VECTOR
__interrupt void ISR_Port4_S1(void){
    P4IFG &= ~BIT1;
    switch1 = 1;
    for(i=0; i<10000; i=i+1){
         //prevent overwrite on buffer with delay
     }
}
//--------------- End Port4_S1 ----------------------------

//--------------- Port2_S2 ----------------------------
// s2 isr... starts moving backward with quarter time
#pragma vector=PORT2_VECTOR
__interrupt void ISR_Port2_S2(void){
    P2IFG &= ~BIT3;
    switch2 = 1;
    for(i=0; i<1000; i=i+1){
         //prevent overwrite on buffer with delay
     }
}
//--------------- End Port2_S2 ----------------------------

//--------------- End ISRs --------------------------------
