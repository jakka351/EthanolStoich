////////////////////////////////////////////////////////////////////////
// Tony Donn's Flex Fuel Ethanol System for FG Falcon
// Tester Present Specialist Automotive Solutions
// Jack Leighton Copyright 2025
////////////////////////////////////////////////////////////////////////
#include <SPI.h>
#include <mcp_can.h>
////////////////////////////////////////////////////////////////////////
#define CAN_CS_PIN 10   // Chip Select pin for MCP2515
MCP_CAN CAN(CAN_CS_PIN);  // Initialize MCP2515 CAN controller
////////////////////////////////////////////////////////////////////////
#define ETHANOL_SENSOR_PIN A0  // Analog input pin for ethanol percentage sensor
#define VOLTAGE_REGULATOR_PIN 9  // PWM output pin for voltage regulation
////////////////////////////////////////////////////////////////////////
// Stoichiometric Air-Fuel Ratios for different fuels
const float AFR_PETROL_98 = 14.7;  // Premium 98 Stoich AFR
const float AFR_ETHANOL_75 = 9.8;  // 75% Ethanol Stoich AFR
const float AFR_ETHANOL_85 = 9.5;  // 85% Ethanol Stoich AFR
float AFR_CURRENT;
float AFR_DESIRED;
////////////////////////////////////////////////////////////////////////
float ethanolVoltage = 0.0;
////////////////////////////////////////////////////////////////////////
// CAN IDs for PCM communication
const unsigned long CAN_ID_RX = 0x7E0;  // Receive from PCM
const unsigned long CAN_ID_TX = 0x7E8;  // Transmit to PCM
////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  ////////////////////////////////////////////////////////////////////////
  // Initialize CAN bus at 500kbps
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("CAN Bus Initialized Successfully.");
  } else {
    Serial.println("CAN Bus Initialization Failed!");
    while (1);
  }
  CAN.setMode(MCP_NORMAL);  // Set CAN controller to normal mode
  ////////////////////////////////////////////////////////////////////////
  // Set up ethanol sensor
  pinMode(ETHANOL_SENSOR_PIN, INPUT);
  ////////////////////////////////////////////////////////////////////////
  // Set up voltage regulator
  pinMode(VOLTAGE_REGULATOR_PIN, OUTPUT);
}
////////////////////////////////////////////////////////////////////////
unsigned long rxId;
byte len = 0;
byte rxBuf[8];
////////////////////////////////////////////////////////////////////////
/////////////////// Air Fuel Ratio Calculations/////////////////////////
//Let's check with Stored Value = 51135:
//
//AFR =( 0.0002944 × 51135) − 0.4115
//AFR = 15.05 − 0.4115
//AFR = 14.64✅
//Now check with Stored Value = 1398:
//AFR = (0.0002944 × 1398) − 0.4115
//AFR = (0.0002944×1398) − 0.4115
//AFR = 0.4115 − 0.4115
//AFR = 0.00✅
////////////////////////////////////////////////////////////////////////

void loop() {
  ////////////////////////////////////////////////////////////////////////
  // Example CAN message to initiate PCM flash sequence
  byte flashMessage[] = {0x02, 0x10, 0x85};  // Example UDS message (Session Control: Programming Session)
  if (sendCANMessage(CAN_ID_RX, flashMessage, sizeof(flashMessage))) {
    Serial.println("Flash command sent to PCM.");
  } else {
    Serial.println("Failed to send flash command.");
  }
  ////////////////////////////////////////////////////////////////////////
  // UDS Flashing Routine (rough implementation)
  //udsFlashECU();
  ////////////////////////////////////////////////////////////////////////
  // Check for CAN responses from the PCM
  if (CAN.checkReceive() == CAN_MSGAVAIL) {
    CAN.readMsgBuf(&rxId, &len, rxBuf);
    Serial.print("Received CAN message from ID: 0x");
    Serial.println(rxId, HEX);

    Serial.print("Data: ");
    for (int i = 0; i < len; i++) {
      Serial.print(rxBuf[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  delay(1000);  // Delay for readability
}
////////////////////////////////////////////////////////////////////////
// Function to send a CAN message
bool sendCANMessage(unsigned long id, byte* data, byte len) {
  byte sendStatus = CAN.sendMsgBuf(id, 0, len, data);
  if (sendStatus == CAN_OK) {
    return true;
  } else {
    Serial.println("Error sending CAN message.");
    return false;
  }
}
////////////////////////////////////////////////////////////////////////
// Function to set voltage regulator output
void setVoltageRegulator(float voltage) {
  if (voltage < 12.0) voltage = 12.0;
  if (voltage > 18.0) voltage = 18.0;
  
  int pwmValue = map(voltage, 12, 18, 128, 255);  // Convert to PWM duty cycle
  analogWrite(VOLTAGE_REGULATOR_PIN, pwmValue);
  Serial.print("Regulator set to output: ");
  Serial.print(voltage);
  Serial.println("V");
}
////////////////////////////////////////////////////////////////////////
// UDS Flashing Routine (simplified)
//byte[] flashMemory = { 0x00, 0x00 }; 
void readPcmMemory() 
{
    requestSecurityAccess();
    // rEAD mEMORY bY aDDRESS TO READ sotich valUE.
    // take stoicj value and modify accordinvg to ETHANOL PERCENTAGE.
    // fLASH BACK TO PCM USING 34 36 37
    // Step 5: Read Memory by Address (0x23) - Verify Flashing
    Serial.println("Reading Memory Address @ 0x10222-0x10224...");
    byte[] flashMemory; int blocksize = 0x04
    for (uint i = 0x10222; i <= 0x10224; i+= blockSize) // Need to replace these with the actual memory locations of STOICH
    {
        flashMemory = readMemoryByAddress(i, blockSize); fileStream.Write(flashMemory, 0, flashMemory.Length);
    }
    try
    {
        byte highByte = flashMemory[0];
        byte lowByte = flashMemory[1];
        ushort combined = (ushort)((highByte << 8) | lowByte);
        Half halfFloat = BitConverter.HalfToHalf(single: combined);
        float binaryByytes = (float)halfFloat;
        AFR_CURRENT = (0.0002944 × binaryBytes) − 0.4115 // HERES OUR AFR CALCULATION
        Serial.println("Current Air Fuel Ratio Setting: " + AFR_CURRENT.ToString());
    }
    catch(Exception Ex) { return; }
    //Now Check Current Ethanol Percentage
    // ETHANOL SENSOR READING CODE GOES HERE
    // Read voltage from ethanol percentage sensor (0-5V range)
    ethanolVoltage = analogRead(ETHANOL_SENSOR_PIN) * (5.0 / 1023.0);
    Serial.print("Ethanol Sensor Voltage: ");
    Serial.println(ethanolVoltage, 2);
    Serial.println("Current Ethanol Content Percentage: ");
    //NOW set oue desired Air Fuel Ratio Target
    int scaledVoltage = ethanolVoltage * 100
    switch(scaledVoltage)
    {
          case int n when (n < 0):
              AFR_DESIRED = AFR_PETROL_98; 
              break;
          case int n when (n > 500):
              AFR_DESIRED = AFR_PETROL_98;
              break;
          case int n when (n < 100):
              AFR_DESIRED = AFR_ETHANOL_75;
              break;
          case int n when (n < 200):
              AFR_DESIRED = AFR_ETHANOL_75;
              break;
          case int n when (n < 300):
              AFR_DESIRED = AFR_ETHANOL_85;
              break;
          case int n when (n < 400):
            AFR_DESIRED = AFR_ETHANOL_85;
            break;
          case int n when (n <= 500):
            AFR_DESIRED = AFR_ETHANOL_85;
            break;
          default:
              return "Unknown voltage";
        
    }
    Serial.println("Target Air Fuel Ratio Setting: " + AFR_DESIRED.ToString());
    // Now we need to convert from our Target Air Fuel Ratio into correctly formatted bytes to be flashed to the PCM
    //const float AFR_PETROL_98 = 14.7;  // Premium 98 Stoich AFR
    //const float AFR_ETHANOL_75 = 9.8;  // 75% Ethanol Stoich AFR
    //const float AFR_ETHANOL_85 = 9.5;  // 85% Ethanol Stoich 
    // AFR=(0.0002944×A)−0.4115
    byte[] writeMemory;

    // Stoichiometric Air-Fuel Ratios for different fuels
    //float AFR_CURRENT;
    //float AFR_DESIRED;
    //Request Security Access Again.
    requestSecurityAccess();
    /*Request Download
    byte[] spanishOak1= { 0x10, 0x09, 0x34, 0x00, 0x01, 0x00, 0x00, 0x00  };// PCM Request Download Message Bytes Spanish Oak //
    byte[] spanishOak1= { 0x21, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };// PCM Request Download Message Bytes Spanish Oak //
    if (sendCANMessage(CAN_ID_RX, spanishOak1, sizeof(spanishOak1))) 
    {
        if (sendCANMessage(CAN_ID_RX, spanishOak2, sizeof(spanishOak2))) 
        {
            
        }    
    
    }*/
    // Transfer Data OR Try to use writeDataByAddress (0x3D) if possible
    // Request Transfer Exit
    for (uint i = 0x10222; i <= 0x10224; i+= blockSize) // Need to replace these with the actual memory locations of STOICH
    {
        writeMemory = writeMemoryByAddress(i, blockSize); fileStream.Write(writeMemory, 0, writeMemory.Length);
    }



    // Car is ready to start with new STOICH value for current ethanol fuel percentagfe
}
void requestSecurityAccess()
    Serial.println("Requesting Security Access...");
    // ACVITATE FEPS 18 volts here
    //BUCK CONVERTER CONTROL CODE NEEDS TO GO HERE
    setVoltageRegulator(18.0);  // Set default output to 18V
    // Step 1: Security Access (0x27)
    byte securityAccess[] = {0x02, 0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};  
    if (sendCANMessage(CAN_ID_RX, securityAccess, sizeof(securityAccess))) 
    {
        Serial.println("Sent Security Access Request.");
        if (CAN.checkReceive() == CAN_MSGAVAIL) 
        {
            string seedResponse = "";
            CAN.readMsgBuf(&rxId, &len, rxBuf);
            //Serial.println(rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4], rxBuf[5], rxBuf[6], rxBuf[7]);
            int num22 = 0;
            num22 += (int)rxBuf[3] << 0x10;
            num22 += (int)rxBuf[4] << 8;
            num22 += (int)rxBuf[5];
            seedResponse =  KeyGenMkI(num22, 0x08, 0x30, 0x61, 0xA4, 0xC5).ToString("X6");
            string response1 = responseKey.Substring(0, 2);
            string response2 = responseKey.Substring(2, 2);
            string response3 = responseKey.Substring(4, 2);
            byte responseByte1 = Convert.ToByte(response1, 16);
            byte responseByte2 = Convert.ToByte(response2, 16);
            byte responseByte3 = Convert.ToByte(response3, 16);
            byte[] sendSecurityKey = {0x05, 0x27, 0x02, responseByte1, responseByte2, responseByte3, 0x00, 0x00}      
            if(sendCANMessage(CAN_ID_RX, sendSecurityKey sizeof(sendSecurityKey))) {
                if (CAN.checkReceive() == CAN_MSGAVAIL) 
                {
                  CAN.readMsgBuf(&rxId, &len, rxBuf);
                  Serial.println(rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4], rxBuf[5], rxBuf[6], rxBuf[7]);
                  if((int)rxBuf[2] == 0x67) {
                      Serial.println("Security Access Granted");
                  }
                }
        }
    }
////////////////////////////////////////////////////////////////////////
// SECURITY ACCESS KEY GEN ALGORITHM
int KeyGenMkI(int s, int sknum, int sknum2, int sknum3, int sknum4, int sknum5)
{  
    var sknum13 = (int)((byte)(s >> 0x10 & 0xFF));
    var b2 = (byte)(s >> 8 & 0xFF);
    var b3 = (byte)(s & 0xFF);
    var sknum6 = (sknum13 << 0x10) + ((int)b2 << 8) + (int)b3;
    var sknum7 = (sknum6 & 0xFF0000) >> 0x10 | (sknum6 & 0xFF00) | sknum << 0x18 | (sknum6 & 0xFF) << 0x10;
    var sknum8 = 0xC541A9;
    for (int i = 0; i < 0x20; i++)
    {
      int sknum10;
      int sknum9;
      sknum8 = (((sknum9 = (sknum10 = (((sknum7 >> i & 1) ^ (sknum8 & 1)) << 0x17 | sknum8 >> 1))) & 0xEF6FD7) | ((sknum9 & 0x100000) >> 0x14 ^ (sknum10 & 0x800000) >> 0x17) << 0x14 | ((sknum8 >> 1 & 0x8000) >> 0xF ^ (sknum10 & 0x800000) >> 0x17) << 0xF | ((sknum8 >> 1 & 0x1000) >> 0xC ^ (sknum10 & 0x800000) >> 0x17) << 0xC | 0x20 * ((sknum8 >> 1 & 0x20) >> 5 ^ (sknum10 & 0x800000) >> 0x17) | 8 * ((sknum8 >> 1 & 8) >> 3 ^ (sknum10 & 0x800000) >> 0x17));
    }
    for (int j = 0; j < 0x20; j++)
    {
      int sknum12;
      int sknum11;
      sknum8 = (((sknum11 = (sknum12 = ((((sknum5 << 0x18 | sknum4 << 0x10 | sknum2 | sknum3 << 8) >> j & 1) ^ (sknum8 & 1)) << 0x17 | sknum8 >> 1))) & 0xEF6FD7) | ((sknum11 & 0x100000) >> 0x14 ^ (sknum12 & 0x800000) >> 0x17) << 0x14 | ((sknum8 >> 1 & 0x8000) >> 0xF ^ (sknum12 & 0x800000) >> 0x17) << 0xF | ((sknum8 >> 1 & 0x1000) >> 0xC ^ (sknum12 & 0x800000) >> 0x17) << 0xC | 0x20 * ((sknum8 >> 1 & 0x20) >> 5 ^ (sknum12 & 0x800000) >> 0x17) | 8 * ((sknum8 >> 1 & 8) >> 3 ^ (sknum12 & 0x800000) >> 0x17));
    }
    return (sknum8 & 0xF0000) >> 0x10 | 0x10 * (sknum8 & 0xF) | ((sknum8 & 0xF00000) >> 0x14 | (sknum8 & 0xF000) >> 8) << 8 | (sknum8 & 0xFF0) >> 4 << 0x10;
}

public byte[] readMemoryByAddress(uint address, uint blockSize)
{
    //Send the read memory request
    byte blockSizeUpper = (byte)((blockSize >> 8) & 0xFF);
    byte blockSizeLower = (byte)(blockSize & 0xFF);
    byte readMemoryAddress[] = {0x07, 0x23, 0x00, ((address >> 16) & 0xFF), ((address >> 8) & 0xFF), ((address) & 0xFF), blockSizeUpper, blockSizeLower };
    sendCANMessage(CAN_ID_TX, readMemoryAddress, sizeof(readMemoryAddress))        
    if (CAN.checkReceive() == CAN_MSGAVAIL) {
      CAN.readMsgBuf(&rxId, &len, rxBuf);
      Serial.print(rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4], rxBuf[5], rxBuf[6], rxBuf[7]);
    }
    byte[] flashBytes = { rxBuf[3], exBuf[4] };
    return flashBytes
}
// writeMemoryByAddress (ref. KWP-GRP-1.5, 7.7) 
// This service is mainly intended for development purposes only. Most service and end-of-line 
// equipment do not support the use of this service. 
// Every instance of this message shall include all four address bytes as specified in Table 7. If the ECU 
// supports less than four address bytes then every byte that is not used shall be set to zero in the request 
// message with data byte #5 always corresponding to the LSB of the ECU's address. 
The MemorySize parameter shall be limited to a maximum of 4088 bytes.
void writeMemoryByAddress(uint address, uint blocksize)
{
    byte blockSizeUpper = (byte)((blockSize >> 8) & 0xFF);
    byte blockSizeLower = (byte)(blockSize & 0xFF);
    byte writeMemoryAddress[] = {0x07, 0x23, 0x00, ((address >> 16) & 0xFF), ((address >> 8) & 0xFF), ((address) & 0xFF), blockSizeUpper, blockSizeLower };
    sendCANMessage(CAN_ID_TX, writeMemoryAddress, sizeof(writeMemoryAddress))        
    if (CAN.checkReceive() == CAN_MSGAVAIL) {
      CAN.readMsgBuf(&rxId, &len, rxBuf);
      Serial.print(rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4], rxBuf[5], rxBuf[6], rxBuf[7]);
    }

}
