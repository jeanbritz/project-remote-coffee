// External header files
#include <i2cmaster.h>

#define DEBUG_MODE (false)

// Special characters
#define CR (13)
#define HORIZONTAL_LINE ("================================")

// Custom baud rates
#define MEGA_BAUD_RATE (9600)
#define GSM_BAUD_RATE (9600)

// Custom pin definitions
#define baseTempADCPin (A0)
#define steamTempADCPin (A1) 


#define DELAY_BETWEEN_SAMPLES (0)  // milliseconds

// MCP9700 Temp offsets
#define BASE_TEMP_OFFSET (14)
#define STEAM_TEMP_OFFSET (14)

// MLX90614 constants
#define MLX90614_ADDR (0x5A)
#define MLX_TEMP_FACTOR (0.02) // 0.02 degrees per LSB (measurement resolution of the MLX90614)
#define KELVIN_CELSIUS_FACTOR (273.15)

// AT command templates
#define AT_UPDATE_TEMP ("AT+AWTDA=d,\"Coffee.stat\",3,\"tempBase,INT32,%d\",tempSteam,INT32,"
// Test variables
double timeElapsed = 0;
double startTime = 0;
boolean runTest = false;
int8_t initCount = 0;
int8_t answer = 0;

// MLX90614 variables
int dev = (MLX90614_ADDR << 1);
int data_low = 0;
int data_high = 0;
int pec = 0;
double tempData = 0x0000; // zero out the data
int frac; // data past the decimal point

// Sensor variables
int baseTemp = 0;
int steamTemp = 0;
int coffeeTemp = 0;
int rawADCBase = 0;
int rawADCSteam = 0;

void setup()
{
  Serial.begin(MEGA_BAUD_RATE);
  Serial1.begin(GSM_BAUD_RATE);
  Serial.println(F("Setting up..."));
  delay(200);
  initCount += sendATcommand("AT", "OK", 1000);
    
  i2c_init(); //Initialise the i2c bus
  // Enable pullups for I2C bus
  PORTC = (1 << PORTC4) | (1 << PORTC5);
  delay(200);
  
  if(init > 0)
  {
    Serial.print(HORIZONTAL_LINE);
    Serial.println(HORIZONTAL_LINE);
    Serial.println(F("| Type \'r\' to run the test or \'s\' to stop it                   |"));
    Serial.print(HORIZONTAL_LINE);
    Serial.println(HORIZONTAL_LINE);
    Serial.println(F(" CSV Format: "));
    Serial.println(F(" Time Elapsed (seconds), Base Temp (Celsius), Steam Temp (Celsius), Coffee Temp (Celcius)"));
    Serial.print(HORIZONTAL_LINE);
    Serial.println(HORIZONTAL_LINE);
  }
  else
  {
    Serial.println(F("Initialisation errors has occurred"));
  }
}
void loop()
{
  if(Serial.read() == 'r')  {  
    runTest = true;
    startTime = millis(); 
  }

  while(runTest)
  {
    timeElapsed = millis() - startTime;   

    // Read the raw voltage
    rawADCBase = analogRead(A0);
    rawADCSteam = analogRead(A1);

    // request data at ram address 0x07
    i2c_start_wait(dev+I2C_WRITE);
    i2c_write(0x07);

    // read value sent back
    i2c_rep_start(dev+I2C_READ);
    data_low = i2c_readAck(); //Read 1 byte and then send ack
    data_high = i2c_readAck(); //Read 1 byte and then send ack
    pec = i2c_readNak();
    i2c_stop();

    // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
    tempData = (double)(((data_high & 0x007F) << 8) + data_low);
    tempData = (tempData * MLX_TEMP_FACTOR) - 0.01;

    // Convert the voltage to a temperature	
    baseTemp = rawADCBase/4 - BASE_TEMP_OFFSET;
    steamTemp = rawADCSteam/4 - STEAM_TEMP_OFFSET;
    coffeeTemp = (int)round(tempData - KELVIN_CELSIUS_FACTOR);

    // Send updated variables to the SMART platform
    while(trintelMetricUpdateTemp() == 0);
    //if(answer == 0)
    //{
    //  Serial.print(F("Error occured while updating the temperature metrics")); 
    //}

    // Print the sensor values on the debug terminal in CSV-format
    Serial.print(timeElapsed/1000, 2);
    Serial.print(F(",")); 
    Serial.print(baseTemp);
    Serial.print(F(",")); 
    Serial.print(steamTemp);
    Serial.print(F(",")); 
    Serial.println(coffeeTemp);

    delay(DELAY_BETWEEN_SAMPLES);

    // Check if the user has requested to stop the test
    if(Serial.read() == 's')
    {
      runTest = false;
      Serial.println(F("========================="));
      timeElapsed = 0;
    } 
  }

}        			

int8_t trintelMetricUpdateTemp()
{
  int8_t answer = 0;
  Serial1.print(F("AT+AWTDA=d,\"Coffee.stat\",3,\"tempBase,INT32,"));Serial1.print(baseTemp);
  Serial1.print(F("\",\"tempSteam,INT32,"));Serial1.print(steamTemp);
  Serial1.print(F("\",\"tempCoffee,INT32,"));Serial1.print(coffeeTemp);
  answer = sendATcommand("\"", "OK", 1000); 
  return answer;
}

int8_t sendATcommand(char* ATcommand, char* expected_answer1,
unsigned int timeout)
{

  uint8_t x=0,  answer=0;
  char response[100];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialize the string

  delay(100);

  while( Serial1.available() > 0) Serial1.read();    // Clean the input buffer
  if(DEBUG_MODE)
  {
    Serial.print(ATcommand);
    Serial.print(CR);
  }
  Serial1.print(ATcommand);    // Send the AT command
  Serial1.write(CR);

  x = 0;
  previous = millis();

  // this loop waits for the answer
  do{

    if(Serial1.available() != 0){
      response[x] = Serial1.read();
      x++;
      // check if the desired answer is in the response of the module
      if (strstr(response, expected_answer1) != NULL)
      {
        answer = 1;
      }
    }
    // Waits for the answer with time out
  }
  while((answer == 0) && ((millis() - previous) < timeout));
  
  return answer;
}





