# Drill Press Control

## Project Description
The project: ![Final Code](FinalProject9main.c)

Drill Press Control is a project designed to interface a stepper motor with a microcontroller. It provides functionality for motor actuation, pressure monitoring, real-time clock (RTC) reading, and user interface reporting. Additional signal conditioning and other features ensure precise control and monitoring.

## Development
### Part 1: Planning and Design
- A programming flowchart and an initial pinout diagram were developed to understand the program functionality and correct initialization for the project.
   ![MSP430 Connections to External Hardware](Part9_Flowchart.pdf)

### Part 2: ADC Sensor Code
- Developed code to read simulated pressure sensor data using the ADC module of the MSP430.
- Implemented threshold comparison logic to trigger events based on pressure readings.
   ![ADC Sensor Code](FinalProject1.c)

### Part 3: I2C RTC Code
- Wrote code to communicate with a real-time clock (RTC) module via the I2C protocol.
- Was used to save a time stamp when a button was pressed.
   ![I2C RTC Code](FinalProject2.c)

### Part 4: UART Code
- Implemented UART communication for user interface reporting.
- Designed output messages to convey sensor status, motor activity, and time data.
   ![UART Code](FinalProject3.c)

### Part 5: LED Timing Code
- Programmed LEDs to emulate the pulses of the motor.
   ![LED Timing Code](FinalProject4.c)

### Part 6: Full Integration with Stepper Motor
- Combined the code from part 2-5 into one cohesive project, and added the stepper motor functionality.
   ![Full Integration Code](FinalProject8main3.c)

### Part 7: Additional Functionality
- The additional functionality was developed and but into the final code:
   ![Final Code](FinalProjet9main.c)
- Optimized the code to ISRs were short and instead used to trigger flags that were handled in main() and 
- Implemented a rolling average for the ADC input. I first tried to do the averaging by iterating through an array, which had an O(N) runtime and negatively impacted the timing to the point of making the code unrunnable. This was optimized by using an index pointer to shift through the array, reducing the complexity to O(1) and significantly improving performance.  
- Introduced a pressure threshold to enhance system safety. When the drill press pressure exceeds 50 lbs (a critical level that could damage the pressure sensor), the forward motor control is disabled, a buzzer alarm is activated, and a UART warning is sent to notify the user.  
- Enhanced signal conditioning for the ADC input:  
  - Added a zener diode clipping circuit to prevent the ADC input from exceeding 3V.  
  - Used a difference amplifier to eliminate a 120mV offset from the pressure sensor signal.  
  - Incorporated a noninverting amplifier with a gain of 1.5 to scale the sensor's voltage range, ensuring the full utilization of the ADC's input range for improved accuracy.  

## Getting Started
### Prerequisites
- MSP-EXP430FR2355 Development Board
- Variable Power Supply
- Analog Discovery 2 (optional, for simulation)
- Stepper Motor (Different speeds may need to be used depending on the stepper motor's gearing)
- IDE supporting MSP430 (e.g., Code Composer Studio)

### Installation and Usage
1. Download FinalProject9main.c
2. Set up the hardware according to the pinout diagram:
   ![MSP430 Connections to External Hardware](Pinout_Diagram_Pt9.png)
5. Open the project in your IDE and adjust the code's values.
6. Build the code and flash the compiled program
8. Watch the UART interface for communcations as you use buttons and ADC input to interact with the system.

## Acknowledgments
- This work was based on course materials provided by Cory Mettler, Microprocesses HW and SW, Montana State Univeristy - Bozeman.
- Libraries/Tools: Uses the MSP-EXP430FR2355, and all code is written in C and compiled with CSS using standard libraries. Diagrams developed in Mirosoft Visio
- Resources: "Embedded Systems Design" by Brock LaMeres.
