#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <i2cmaster.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define DEBUG_MODE (true)

// Industrial standard constansts
#define MAINS_VOLTAGE_FREQ_HZ (50)
#define MAINS_RMS_VOLTAGE (240)

// AVR constants
#define ADC_SAMPLE_FREQ (8900)
#define MEASURED_DC_OFFSET_MILLIVOLTS (2410)
const int TOTAL_SAMPLES = round(8900/MAINS_VOLTAGE_FREQ_HZ);

// Special characters
#define CR (13)
#define HORIZONTAL_LINE ("======================================================")

// Custom pin definitions
#define baseTempADCPin (A0)
#define steamTempADCPin (A1) 
#define currentSenseADCPin (A8)
#define SPI_SCK  (52)
#define SPI_MOSI (51) 
#define SPI_MISO (50)
#define SPI_SS (53)

// Custom baud rates
#define MEGA_BAUD_RATE (9600)
#define GSM_BAUD_RATE (9600)

// External class instantiations
Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);


// MCP9700 offset constants
#define BASE_TEMP_OFFSET (14)
#define STEAM_TEMP_OFFSET (14)

// MLX90614 constants
#define MLX90614_ADDR (0x5A)
#define MLX_TEMP_FACTOR (0.02) // 0.02 degrees per LSB (measurement resolution of the MLX90614)
#define KELVIN_CELSIUS_CONV_FACTOR (273.15)

// Voltage sensor constants
#define MEASURED_DC_OFFSET_MILLIVOLTS (2410)

// Voltage sensor variables
volatile int voltage_samples[TOTAL_SAMPLES];
double voltage_filtered_samples[TOTAL_SAMPLES];
double realVcc = 0;
int measuredoffset = 0;
int cnt = 0;
double trf_rms = 11.4;
double trf_peak = sqrt(2)*trf_rms;
double aux = 0;

// Variables
volatile boolean primary = true;
volatile double primary_frac = 0;
volatile double second_frac = 0;

int current_duty_cycle = 0;
int8_t answer = 0;
int previous_update_time = 0;

// Sensor variables
volatile boolean updated_sensor_data = false;
volatile int rawBaseADC = 0;
volatile int rawSteamADC = 0;
int baseTemp = 0;
int steamTemp = 0;


int dev = (MLX90614_ADDR << 1);
int data_low = 0;
int data_high = 0;
int pec = 0;
double tempData = 0x0000;
int coffeeTemp = 0;

char dummy = 0;
char main_menu_command = 0;
char debug_command = 0;
String temp = "";
String inputString = "";
boolean stringComplete = false;
boolean busy_wait = false;

char platform_response[200];
char value_array[4];

void wdt_init()
{


}

void setup()
{
  realVcc = 4960;
  measuredoffset = round((MEASURED_DC_OFFSET_MILLIVOLTS/realVcc)*1024);
  timer4_init();
  stopTRIACpulse();
  Serial.begin(MEGA_BAUD_RATE);
  Serial1.begin(GSM_BAUD_RATE);
  answer = sendATcommand("AT", "OK", 2000);
  answer += sendATcommand("AT+CREG?", "+CREG: 0,1", 2000);
  if(answer == 0)
  {
    Serial.print(F("ERROR: GSM modem did not properly initialized"));
  }
  //i2c_init(); //Initialise the i2c bus
  // Enable pullups for I2C bus
  //PORTC = (1 << PORTC4) | (1 << PORTC5);
  //	delay(200);

  Serial.println(HORIZONTAL_LINE);
  Serial.println(F("\tCoffee Percolator Monitor"));
  //Serial.println(F("\tType 'd' to enter debug mode"));
  Serial.println(HORIZONTAL_LINE);
  inputString.reserve(200);

  while (sendATcommand("AT+AWTDA=c*d*", "OK", 2000) == 0);
  Serial1.print(F("AT+AWTDA=d,\"Coffee.misc\",1,\"currentState,INT32,2"));
  if(sendATcommand("\"", "OK", 2000))
  {
    Serial.println(F("INFO: Operating state: Idle"));
  }


  // Acknowledge an old command triggers new commands being sent from platform faster.
  sendAck("1032831");

  delay(2000);

}

void loop()
{

  serial1_event();
  if(stringComplete == true)
  {  
    if(inputString.length() > 2)
    {
      if(DEBUG_MODE)
      {
        Serial.println(HORIZONTAL_LINE);
        Serial.print(F("Response received >>> "));
        Serial.print(F(" [ "));
        Serial.print(inputString);
        Serial.println(F(" ] "));
        Serial.println(HORIZONTAL_LINE);
      }
      
    
    // Wait for an unsolicited response from the platform
    inputString.trim();
    temp = inputString.substring(0,7);
	
    if (temp.equals("+AWTDA:"))
    {
      busy_wait = true;
      unpack_response(inputString.substring(8));
    }
	}
    stringComplete = false;
    inputString = "";


  }
  else
  {

    if(busy_wait == false && updated_sensor_data == true)
    {

      baseTemp = rawBaseADC/4 - BASE_TEMP_OFFSET;
      steamTemp = rawSteamADC/4 - STEAM_TEMP_OFFSET;
      Serial1.print(F("AT+AWTDA=d,\"Coffee.temp\",3,\"base,INT32,"));
      Serial1.print(baseTemp);
      Serial1.print(F("\",\"steam,INT32,"));
      Serial1.print(steamTemp);
      Serial1.print(F("\",\"coffee,INT32"));
      if(DEBUG_MODE)
      {
        Serial.print(F("Base Temp:\t"));
        Serial.println(baseTemp);
        Serial.print(F("Steam Temp:\t"));
        Serial.println(steamTemp);
        Serial.print(F("Coffee Temp:\t"));
        Serial.println(coffeeTemp);
        Serial.print(F("Mains Voltage:"));
        Serial.println(get_RMS_value());
      }
      if(sendATcommand("\"", "OK", 2000))
      {
        if(DEBUG_MODE)
          Serial.println(F("DEBUG: Sensor data updated!"));

      }
      else
      {
        if(DEBUG_MODE)
          Serial.println(F("ERROR: Sensor data not updated!"));
      }

      updated_sensor_data = false;
      delay(2000);

    }
  }

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


void serial1_event()
{
  while(Serial1.available() > 0)
  {

    char inChar = (char)Serial1.read();

    if(inChar == '\n')
    {
      stringComplete = true; 
    }
    else
    {
      inputString += inChar;
    }
  }



}

double get_RMS_value()
{
  double temp;
  int i;
  for(i = 0; i < TOTAL_SAMPLES; i++)
  {
    aux = (((float)(voltage_samples[i]-measuredoffset))/1024)*(realVcc/1000);
    aux = aux * (16/1.53)*21;
    voltage_filtered_samples[i] = aux;
  }
  for(i = 0; i < TOTAL_SAMPLES; i++)
  {
    // Summation of all samples^2
    temp += voltage_filtered_samples[i] * voltage_filtered_samples[i];
  }
  temp = temp/TOTAL_SAMPLES;
  temp = sqrt(temp);
  return (int)round(temp);
}


double measureVcc()
{
   // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

int MLX90614_retrieve_temp()
{
  // request data at ram address 0x07 (refer to datasheet for more info)
  i2c_start_wait(dev + I2C_WRITE);
  i2c_write(0x07);

  // read value sent back
  i2c_rep_start(dev + I2C_READ);
  data_low = i2c_readAck(); // Read 1 byte and then send ack
  data_high = i2c_readAck(); // read 1 byte and the send ack
  pec = i2c_readNak();
  i2c_stop();

  tempData = (double)(((data_high & 0x007F) << 8) + data_low);
  tempData = (tempData * MLX_TEMP_FACTOR) - 0.01;

  return (int)round(tempData - KELVIN_CELSIUS_CONV_FACTOR);
}

void stopTRIACpulse()
{
  startTRIACpulse(10);
  delay(5000);
  startTRIACpulse(0);
}

void startTRIACpulse (int duty_cycle)
{
  current_duty_cycle = duty_cycle;
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  cli();    // disable global interrupts
  TCCR3A = 0;
  TCCR3B = 0;

  // set compare match register to desired timer count
  OCR3A = 1;

  // turn on CTC mode
  TCCR3B |= _BV(WGM32);

  // set CS30 and CS32 for 1024 prescalar
  TCCR3B |= _BV(CS30)|_BV(CS32);
  // enable timer compare interrupt
  TIMSK3 |= _BV(OCIE3A);
  sei();    // enable global interrupts
}

char get_array_length(char *arr)
{
  int k = 0;
  char *seek = arr;
  while('\n' != *seek++)
  {
    k++;
  }
  return k;
}

void timer4_init()
{
  cli();    // disable global interrupts
  TCCR4A = 0;
  TCCR4B = 0;

  // set compare match register to desired timer count
  OCR4A = 1562;

  // turn on CTC mode
  TCCR4B |= _BV(WGM42);

  // set CS40 and CS42 for 1024 prescalar
  TCCR4B |= _BV(CS40)|_BV(CS42);
  // enable timer compare interrupt
  TIMSK4 |= _BV(OCIE4A);
  sei();    // enable global interrupts
}


void unpack_response(String content)
{
  int8_t i = 0;
  int duty_cycle = 0;
  int content_length = 0;
  String type;
  String ticketID;
  String asset;
  String command;
  String data_type;
  String value;
  String crc;

  char *str;
  char *p;
  // Convert String to char array
  content.toCharArray(platform_response, 200);

  // Calculate the content length to save time
  content_length = get_array_length(platform_response);

  //Serial.print("Length: ");
  //Serial.println(content_length, DEC);

  // Separate the string into values delimited by a comma (',')
  for( str = strtok_r(platform_response, ",", &p);
    str;
    str = strtok_r(NULL, ",", &p))
  {
    switch(i)
    {
    case 0:
      type = str;
      break;
    case 1:
      ticketID = str;
      break;
    case 2:
      asset = str;
      break;
    case 3:
      command = str;
      break;
    case 4:
      data_type = str;
      break;
    case 5:
      value = str;
      break;
    case 6:
      crc = str;
      break;
    default:
      Serial.print("Unknown value: ");
      Serial.println(str);
      break;
    }
    i++;
  }
  value.toCharArray(value_array,4);


  // for debugging purposes
  if(DEBUG_MODE){
    Serial.println(HORIZONTAL_LINE);
    Serial.println(F("\tAWTDA Response Content"));
    Serial.println(HORIZONTAL_LINE);  
    Serial.print("Response Type:\t"); 
    Serial.println(type);
    Serial.print("Ticket ID:\t"); 
    Serial.println(ticketID);
    Serial.print("Asset:\t\t");
    Serial.println(asset);
    Serial.print("Command:\t");
    Serial.println(command);
    Serial.print("Data Type:\t");
    Serial.println(data_type);
    Serial.print("Value:\t\t");
    Serial.println(atoi(value_array));
    Serial.print("CRC:\t\t");
    Serial.println(crc);
    Serial.println(HORIZONTAL_LINE);
  }
  if(type.equals("UP"))
  {
    busy_wait = false;
  }
  else if(type.equals("BOOT"))
  {
	while(sendATcommand("AT", "OK", 10000) == 0);
	busy_wait = false;  
  }
  else if(type.equals("DOWN"))
  {
	  // reset the modem
	  busy_wait = false;
  }
  else if(type.equals("TIMEOUT"))
  {
      // Do something
      busy_wait = false;
  }
  else  if(command.equals("TriggerPulse"))
  {


    duty_cycle = atoi(value_array);
    Serial1.print(F("AT+AWTDA=d,\"Coffee.power\",1,\"duty_cycle,INT32,"));
    Serial1.print(duty_cycle);
    if (sendATcommand("\"", "OK", 2000))
    {
      if(duty_cycle > 0)
      {
        startTRIACpulse(duty_cycle);

        if(duty_cycle != 100)
        {
          Serial.print(F("INFO: Percolator is given "));
          Serial.print(duty_cycle);
          Serial.print("%");
          Serial.println(" of maximum power");
        }
        else
        {
          Serial.println(F("INFO: Percolator is given maximum power"));
        }
      }
      else
      { 
        Serial.println(F("INFO: Percolator is switched off"));
        stopTRIACpulse();
      }
      if(sendAck(ticketID))
      {
        Serial.print(F("SUCCESS: "));
        Serial.print(F("Ticket ID [ ")); 
        Serial.print(ticketID);
        Serial.println(F(" ] was acknowledged."));
        busy_wait = false;
      }
      else
      {
        Serial.print(F("FAILURE: "));
        Serial.print(F("Ticket ID [ "));  
        Serial.print(ticketID);
        Serial.print(F(" ]was NOT acknowledged."));  
        busy_wait = false;
      }
    }
  }
  type = NULL;
  ticketID = NULL;
  asset = NULL;
  command = NULL;
  data_type = NULL;
  value = NULL;
  crc = NULL;
}

void clean_serial1_buffer()
{
  while(Serial1.available() > 0)
  {
    dummy = Serial1.read();
  }
}

int8_t sendAck(String ticketID)
{
  int8_t success = 0;
  Serial1.print("AT+AWTDA=a,\"Coffee\",");
  Serial1.print(ticketID);
  success = sendATcommand("", "OK", 2000); 
  return success; 
}
ISR(TIMER3_COMPA_vect)
{
  if(current_duty_cycle == 100)
  {
    digitalWrite(2, HIGH);
    digitalWrite(4, HIGH);
  }
  else
  {
    primary_frac = 15624*(current_duty_cycle/100.0);
    second_frac = 15624*(1 - (current_duty_cycle/100.0));


    if(primary)
    {
      // on time
      digitalWrite(2, HIGH);
       digitalWrite(4, HIGH);
      OCR3A = primary_frac;
      primary = false;
    }
    else
    {
      digitalWrite(2, LOW);
       digitalWrite(4, LOW);
      // off time (the larger OCR3A is here the shorter is the off time)
      OCR3A = second_frac;
      primary = true;    
    }
  }


}
ISR(TIMER4_COMPA_vect)
{

  if(updated_sensor_data == false)
  {

    rawBaseADC = analogRead(A0);
    rawSteamADC = analogRead(A1);
    for (cnt = 0; cnt < TOTAL_SAMPLES; cnt++)
    {
       voltage_samples[cnt] = analogRead(A9);
    }

    updated_sensor_data = true;
  }
}














