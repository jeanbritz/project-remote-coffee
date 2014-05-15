#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <SPI.h>
#include <Adafruit_PN532.h>

// Special characters
#define CR (13)
#define HORIZONTAL_LINE ("==================================================================")

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

// External class intantisations
Adafruit_PN532 nfc(SCK, MISO, MOSI, SS);

// Variables
volatile boolean primary = true;
volatile double primary_frac = 0;
volatile double second_frac = 0;
int current_duty_cycle = 0;
int8_t answer = 0;
int prev_baseTemp = 0;
int prev_steamTemp = 0;


char dummy = 0;
char main_menu_command = 0;
char debug_command = 0;

String inputString = "";
boolean stringComplete = false;

char platform_response[200];
char value_array[4];

void wdt_init()
{


}

void setup()
{

  
  stopTRIACpulse();
  Serial.begin(MEGA_BAUD_RATE);
  Serial1.begin(GSM_BAUD_RATE);
  answer = sendATcommand("AT", "OK", 2000);
  answer += sendATcommand("AT+CREG?", "+CREG: 0,1", 2000);
  if(answer == 0)
  {
    Serial.print(F("ERROR: Init"));
  }

  Serial.println(HORIZONTAL_LINE);
  Serial.println(F("\tCoffee Percolator Monitor          "));
  Serial.println(F("\tType 'd' to enter debug mode"));
  Serial.println(HORIZONTAL_LINE);
  inputString.reserve(500);
  Serial1.flush();
  sendATcommand("AT+AWTDA=c*d*", "OK", 2000);
  Serial1.print("AT+AWTDA=d,\"Coffee.misc\",1,\"currentState,INT32,2");
  if(sendATcommand("\"", "OK", 2000))
  {
    Serial.println("Operating state: Idle");
  }
  
  

  Serial1.print("AT+AWTDA=a,\"Coffee\",1032831");
  Serial1.write(CR);
  delay(200);

  while(Serial1.available() > 0)
  {
    dummy = (char)Serial1.read();
  }

  Serial.flush();


}

void loop()
{
  serial1_event();

  if(stringComplete == true)
  {
    Serial.print(inputString);

    // Wait for an unsolicited response from the platform
    if (inputString.startsWith("+AWTDA:"))
    {
      unpack_response(inputString.substring(8));
    }
    else
    {
      // Do something else
    }
    stringComplete = false;
    inputString = "";
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

int getTemperature(int analogPin, int offset)
{
  int i = 0;
  double temp = 0;
  double rawADC = 0;

  // Read the raw voltage 5 times to reduce the noise
  for(i = 0; i < 5; i++)
  {       
    rawADC += analogRead(analogPin);
  }

  // Get the mean value
  rawADC /= 5;

  // Convert the voltage to a temperature	
  temp = rawADC/4 - offset;

  // Return it as an integer
  return (int)(temp);
}
void serial1_event()
{
  while(Serial1.available() > 0)
  {
    char inChar = (char)Serial1.read();
    inputString += inChar;

    if(inChar == '\n')
      stringComplete = true; 
  }
}

void stopTRIACpulse()
{
  cli();
  TCCR3A = 0;
  TCCR3B = 0;
  OCR3A=0;
  sei();
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

  // Seperate the string into values delimited by a comma (',')
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
  if(true){
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
  if(command == "TriggerPulse")
  {
    
    
    duty_cycle = atoi(value_array);
    if(duty_cycle > 0)
    {
      startTRIACpulse(duty_cycle);
      
      if(duty_cycle != 100)
      {
        Serial.print(F("INFO: Percolator is given "));Serial.print(duty_cycle);Serial.print("%");Serial.println(" of maximum power");
      }
      else
      {
        Serial.print(F("INFO: Percolator is given maximum power"));
      }
    }
    else
    { 
      Serial.print(F("INFO: Percolator is switched off"));
      stopTRIACpulse();
    }
    if(sendAck(ticketID))
    {
      Serial.print(F("SUCCESS: "));Serial.print(F("Ticket ID [ ")); Serial.print(ticketID);Serial.print(F(" ] was acknowledged."));
    }
    else
    {
      Serial.print(F("FAILURE: "));Serial.print(F("Ticket ID [ "));  Serial.print(ticketID);Serial.print(F(" ]was NOT acknowledged."));  
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
  clean_serial1_buffer();
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
  }
  else
  {
    primary_frac = 15624*(current_duty_cycle/100.0);
    second_frac = 15624*(1 - (current_duty_cycle/100.0));
  
 
  if(primary)
  {
    // on time
    digitalWrite(2, HIGH);
    OCR3A = primary_frac;
    primary = false;
  }
  else
  {
    digitalWrite(2, LOW);
    // off time (the larger OCR3A is here the shorter is the off time)
    OCR3A = second_frac;
    primary = true;    
  }
  }

  
}






