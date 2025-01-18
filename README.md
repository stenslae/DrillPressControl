# Drill Press Control

## Project Description
Drill Press Control is a project designed to interface a stepper motor with a microcontroller. It provides functionality for motor actuation, pressure monitoring, real-time clock (RTC) reading, and user interface reporting. Additional signal conditioning and other features ensure precise control and monitoring.

## Development
### Part 1: Planning and Design
- A programming flowchart and an initial pinout diagram were developed to understand the program functionality and correct initialization for the project.
   ![MSP430 Connections to External Hardware](Part9_Flowchart.pdf)

### Part 2: ADC Sensor Code
- Developed code to read simulated pressure sensor data using the ADC module of the MSP430.
- Implemented threshold comparison logic to trigger events based on pressure readings.

### Part 3: I2C RTC Code
- Wrote code to communicate with a real-time clock (RTC) module via the I2C protocol.
- Ensured accurate timekeeping and synchronized operations based on time-based events.

### Part 4: UART Code
- Implemented UART communication for user interface reporting.
- Designed output messages to convey sensor status, motor activity, and time data.

### Part 5: LED Timing Code
- Programmed LED indicators to reflect the timing and status of various operations.
- Used timers to synchronize LED behavior with motor and sensor activities.

### Part 6: Full Integration with Stepper Motor
- Combined all modules to control the stepper motor based on sensor input and RTC timing.
- Fine-tuned motor actuation for smooth and precise operation.

### Part 7: Additional Functionality
- Added error-handling routines to improve system robustness.
- Enhanced user interface with additional status messages and controls.

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
