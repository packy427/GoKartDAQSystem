//-----------------------------------------------------------------------------
//  Filename   : Main.c
//  Title      : Main function for GKDAQ slave nodes
//  Author     : Patrick Kennedy (PK3)
//  Created    : 08/02/2016
//  Modified   : 02/26/2018
//  Version    : 0.2
//  Description:
//     Contains all functionalty for slave node in GKDAQ system
//-----------------------------------------------------------------------------

/*== PREPROCESSOR STATEMENTS ==*/

// Import AVR Libraries
#include <avr/io.h>       // Used to allow I/O
#include <util/delay.h>   // Used to set call wait() function
#include <avr/eeprom.h>   // Used to read/write to AVR EEPROM
#include <avr/interrupt.h>  // Used to process hardware interrupts

// Import Core Functionality Code
#include "lib/PinDefinitions.h"   // AVR pin definitions
#include "lib/MCP2515.c"  // MCP2515 SPI commands
#include "lib/USART.c"    // UART register controls
#include "lib/SPI.c"      // SPI register controls
#include "lib/Analog.c"   // Analog register controls
#include "lib/Sensors.c"  // Supported sensors

// Set macros for MCP2515 SPI chip select
#define CAN_SELECT    SPI_PORT &= ~(1 << SPI_CAN_SS)
#define CAN_DESELECT  SPI_PORT |= (1 << SPI_CAN_SS)

/*== GLOBAL VARIABLES ==*/
uint8_t NodeAddress;
bool volatile CALIBRATION_MODE; // Calib mode flag
bool volatile CAN_RCV0_FLAG;    // Set on CAN msg rcv
bool volatile CAN_RCV1_FLAG;    // Set on can msg rcv

// Enumerations
enum IOPORTS{
    IO_A0 = 0,
    IO_A1 = 1,
    IO_A2 = 2,
    IO_A3 = 3,
    IO_D0 = 4,
    IO_I2C = 5,
    IO_UART = 6,
    IO_SPI = 7
};
enum MEASUREMENTS{
    M_NOMEASUREMENT     = 0,  // No measurement
    M_ENGINETEMP        = 1,  // AD8495
    M_EXHAUSTTEMP       = 2,  // AD8495
    M_ENGINESPEED       = 3,  // PK0010
    M_AXLESPEED         = 4,  // PK0020
    M_THROTTLEPOSITION  = 5,  // PK0030
    M_BRAKEPOSITION     = 6,  // PK0040
    M_STEERINGANGLE     = 7,  // PK0050
    M_AMBIENTTEMP       = 8,  // TMP36
    M_ACCGYRO           = 9,  // MPU6050
    M_ACCGYROMAG        = 10, // MPU9250
    M_GPS               = 11, // MTK3339
    M_TEMPHUMIDITY      = 12  // Si7021
};
enum CANIDs{
    ID_CAL_START        = 0x01,
    ID_CAL_CHANGEADDR   = 0x02,
    ID_CAL_CHANGEIO     = 0x03,
    ID_CAL_RESET        = 0x04,
    ID_CAL_EXIT         = 0x0F,
    ID_ENGINETEMP       = 0x10,
    ID_EXHAUSTTEMP      = 0x11,
    ID_ENGINESPEED      = 0x12,
    ID_GPS              = 0x20,
    ID_ACCELERATION     = 0x21,
    ID_HEADING          = 0x22,
    ID_AXLESPEED        = 0x23,
    ID_GYRATION         = 0x24,
    ID_THROTTLEPOSITION = 0x30,
    ID_BRAKEPOSITION    = 0x31,
    ID_STEERINGANGLE    = 0x32,
    ID_AMBIENTTEMP      = 0x40,
    ID_HUMIDITY         = 0x41
};


/* INTERRUPT ROUTINES TO BE IMPLEMENTED
ISR(){
  // Interrupt for command mode, pin interrupt on
  // 1) Read address
  // 2) If ID matches calib msg then read data and match to node address
  // 3) If node address matches, enter calib mode
  CalibrationMode = true;
}

ISR(UART_RX){
  // PARSE GPS
  // Only identify the one message we want
}
*/

int main(void){
  /*== VARIABLE DECLARATIONS ==*/
  uint8_t measurements[8];

  /*== INITIALIZATION ROUTINE ==*/

  _delay_ms(500); // Delay for 500ms, allow all devices to wake up

  // Read EEPROM Values
  /* NodeAddress = eeprom_read_byte(0x00);   // Node address
  sensors[0] = eeprom_read_byte(0x01);    // Analog 0  (IO Port 0)
  sensors[1] = eeprom_read_byte(0x02);    // Analog 1  (IO Port 1)
  sensors[2] = eeprom_read_byte(0x03);    // Analog 2  (IO Port 2)
  sensors[3] = eeprom_read_byte(0x04);    // Analog 3  (IO Port 3)
  sensors[4] = eeprom_read_byte(0x05);    // Digital 0 (IO Port 4)
  sensors[5] = eeprom_read_byte(0x06);    // I2C (TWI) (IO Port 5)
  sensors[6] = eeprom_read_byte(0x07);    // UART      (IO Port 6)
  sensors[7] = eeprom_read_byte(0x08);    // SPI       (IO Port 7)

  // Make Decisions Based On EEPROM Values
  */

  // Node specific information (change for each node)
  NodeAddress = 0x01;

  // Define measurements taken on each IO channel
  measurements[IO_A0]    = M_NOMEASUREMENT;   // A0
  measurements[IO_A1]    = M_NOMEASUREMENT;   // A1
  measurements[IO_A2]    = M_NOMEASUREMENT;   // A2
  measurements[IO_A3]    = M_NOMEASUREMENT;   // A3
  measurements[IO_D0]    = M_NOMEASUREMENT;   // D0
  measurements[IO_I2C]   = M_NOMEASUREMENT;   // I2C
  measurements[IO_UART]  = M_NOMEASUREMENT;   // Serial
  measurements[IO_SPI]   = M_NOMEASUREMENT;   // SPI

  // Configure Hardware
  USART_Init();     // Configure serial
  SPI_Init();       // Configure SPI
  MCP2515_Init();   // Configure MCP2515    TODO: MAKE SURE INTERRUPT IS GENERATED ON MSG RCV, FILTER out all msgs >0x0F
  Analog_Init();    // Configure analog inputs


  // Loop
  while(1){



  }

  // Never should reach here
  return 0;
}



void CalibrationRoutine(){
  uint8_t rxBuffer;
  uint32_t msgID;
  uint64_t msgData;

  while(CALIBRATION_MODE != 0){
    if(CAN_RCV0_FLAG != 0 || CAN_RCV1_FLAG != 0){
      if(CAN_RCV0_FLAG != 0){
        rxBuffer = 0;
      }
      else{
        rxBuffer = 1;
      }

      msgID = MCP2515_ReadID(rxBuffer);
      msgData = MCP2515_ReadData(rxBuffer);

      if(msgID == ID_CAL_CHANGEADDR){
        // Change node addr based on msgdata
      }
      else if(msgID == ID_CAL_CHANGEIO){
        // change io port measurement based on msgdata
      }
      else if(msgID == ID_CAL_EXIT){
        CALIBRATION_MODE = 0;
      }
    }
  }
  return;
}

void UpdateRoutine(uint8_t meas, uint8_t ioPort){
  uint64_t data;
  if(meas == M_NOMEASUREMENT){
    return;
  }
  switch(meas){
    // Analog Engine Head Temperature Sensor
    case M_ENGINETEMP:
      data = GetAnalogInput(ioPort);
      MCP2515_SendCANMessage(MSG_STD, ID_ENGINETEMP, data, 2);
      break;

    // Analog Exhaust Gas Temperature Sensor
    case M_EXHAUSTTEMP:
      data = GetAnalogInput(ioPort);
      MCP2515_SendCANMessage(MSG_STD, ID_EXHAUSTTEMP, data, 2);
      break;

    // Analog Tachometer (Engine Speed) Sensor
    case M_ENGINESPEED:
      data = GetAnalogInput(ioPort);
      MCP2515_SendCANMessage(MSG_STD, ID_ENGINESPEED, data, 2);
      break;

    // Analog Tachometer (Axle Speed) Sensor
    case M_AXLESPEED:
      data = GetAnalogInput(ioPort);
      MCP2515_SendCANMessage(MSG_STD, ID_AXLESPEED, data, 2);
      break;

    // Analog Throttle Position Sensor
    case M_THROTTLEPOSITION:
      data = GetAnalogInput(ioPort);
      MCP2515_SendCANMessage(MSG_STD, ID_THROTTLEPOSITION, data, 2);
      break;

    // Analog Brake Position Sensor
    case M_BRAKEPOSITION:
      data = GetAnalogInput(ioPort);
      MCP2515_SendCANMessage(MSG_STD, ID_BRAKEPOSITION, data, 2);
      break;

    // Analog Steering Angle Sensor
    case M_STEERINGANGLE:
      data = GetAnalogInput(ioPort);
      MCP2515_SendCANMessage(MSG_STD, ID_STEERINGANGLE, data, 2);
      break;

    // Analog Ambient Temperature Sensor
    case M_AMBIENTTEMP:
      data = GetAnalogInput(ioPort);
      MCP2515_SendCANMessage(MSG_STD, ID_AMBIENTTEMP, data, 2);
      break;

    // MPU6050 (I2C)
    case M_ACCGYRO:
      //data = MPU6050_GetAcceleration();
      MCP2515_SendCANMessage(MSG_STD, ID_ACCELERATION, data, 6);
      //data = MPU6050_GetGyration();
      MCP2515_SendCANMessage(MSG_STD, ID_GYRATION, data, 6);
      break;

    // MPU9250
    case M_ACCGYROMAG:
      //data = mpu9205_GetAcceleration();
      MCP2515_SendCANMessage(MSG_STD, ID_ACCELERATION, data, 6);
      //data = MPU9250_GetGyration();
      MCP2515_SendCANMessage(MSG_STD, ID_GYRATION, data, 6);
      //data = MPU9250_GetGyration();
      MCP2515_SendCANMessage(MSG_STD, ID_HEADING, data, 6);
      break;

    // MTK3339 GPS Module
    case M_GPS:
      // TODO: Need code w/ interrupts, ignore? Handled w/ cgps on RPi
      //data = MTK3339_GetPosition();
      MCP2515_SendCANMessage(MSG_STD, ID_GPS, data, 6);
      break;

    // Si7021 Temperature/Humidity Sensor
    case M_TEMPHUMIDITY:
      // TODO: Need code, low priority
      // data = Si7021_GetTemperature()
      MCP2515_SendCANMessage(MSG_STD, ID_AMBIENTTEMP, data, 2);
      // data = Si7021_GetHumidity()
      MCP2515_SendCANMessage(MSG_STD, ID_HUMIDITY, data, 2);
      break;
  }
}

void LoopbackTest(){
  uint8_t msgType;
  uint32_t msgID;
  uint64_t data;
  uint8_t c;
  uint64_t sendData;
  uint8_t ctrlCAN;
  uint8_t ctrlTx0;
  uint8_t ctrlTx1;
  uint8_t timeout;
  uint8_t att;
  uint8_t x;

  if(MCP2515_SetCANMode(MODE_LOOPBACK)){
    PrintString("Loopback Mode Fail");
  }
  else{
    PrintString("Loopback Mode OK\r\n");
  }

  sendData = 0;
  MCP2515_Write(0x6D, 0);
  MCP2515_Write(0x6C, 0);
  MCP2515_Write(0x6B, 0);
  MCP2515_Write(0x6A, 0);
  MCP2515_Write(0x69, 0);
  MCP2515_Write(0x68, 0);
  MCP2515_Write(0x67, 0);
  MCP2515_Write(0x66, 0);

  MCP2515_LoadID(0, MSG_STD, 0xBAE, 8);
  MCP2515_LoadData(0, 0xABC);
  PrintString("TX0DLC: ");
  PrintBinaryByte(MCP2515_Read(0x35));
  PrintString("\r\n");
  MCP2515_Write(0x35, 0b00001000);
  _delay_ms(100);
  MCP2515_RTS(0);
  ctrlTx0 = MCP2515_Read(TXB0CTRL) & (1 << TXREQ);
  while(ctrlTx0 == 1 && att < timeout){
    ctrlTx0 = MCP2515_Read(TXB0CTRL) & (1 << TXREQ);
    TransmitByte(ctrlTx0);
    att = att + 1;
  }
  PrintString("Rx Status : ");
  PrintBinaryByte(MCP2515_ReadRxStatus());
  PrintString(" \r\n");
  data = MCP2515_ReadData(0);
  PrintString("Read Data: ");
  PrintHexByte((data >> 56) & 0xFF);
  PrintHexByte((data >> 48) & 0xFF);
  PrintHexByte((data >> 40) & 0xFF);
  PrintHexByte((data >> 32) & 0xFF);
  PrintHexByte((data >> 24) & 0xFF);
  PrintHexByte((data >> 16) & 0xFF);
  PrintHexByte((data >> 8) & 0xFF);
  PrintHexByte((data >> 0) & 0xFF);
  PrintString("\r\n");
  PrintString("Receive ID Buffer");
  PrintHexByte(MCP2515_Read(0x31));
  PrintHexByte(MCP2515_Read(0x32));
  PrintString("\r\n");
  msgID = MCP2515_ReadID(0);
  PrintString("Read ID: ");
  PrintHexByte((msgID >> 24) & 0xFF);
  PrintHexByte((msgID >> 16) & 0xFF);
  PrintHexByte((msgID >> 8) & 0xFF);
  PrintHexByte(msgID & 0xFF);
  PrintString(" \r\n");

   _delay_ms(5000);
   x = 1;
}

// EOF