#include <Arduino.h>

#define MEGA_BAUD_RATE (9600)
#define GSM_BAUD_RATE (9600)
#define HORIZONTAL_LINE ("================================")
#define AT_COMMAND_TIMEOUT (2000)

char temp;
int8_t totalTests = 2;
int8_t countTests = 0;
int8_t answer = 0;

void setup()
{

  Serial.begin(MEGA_BAUD_RATE);
  Serial.println(HORIZONTAL_LINE);
  Serial.println(F("        GSM Modem Test          "));
  Serial.println(HORIZONTAL_LINE);
  Serial.println(HORIZONTAL_LINE);
  Serial.println(F("| Type \'t\' to start the test   |"));
  Serial.println(HORIZONTAL_LINE);
  Serial1.begin(GSM_BAUD_RATE);

}
void loop()
{

  if(Serial.read() == 't')
  {
    countTests = 0;
    while(countTests < totalTests)
    {
      countTests = 0;
      Serial.print(F("\n*****    Test: "));
      Serial.print(F("Send \'AT\'"));
      Serial.println(F("    *****"));
      if(sendATcommand("AT", "OK", AT_COMMAND_TIMEOUT))
      {
        Serial.println(F("Response: OK"));
        countTests++;
      }
      else
      {
        Serial.println(F("Response: No response"));
        Serial.println(F("Error: Test failed!"));
      }
      Serial.print(F("\n*****    Test: "));
      Serial.print(F("Send \'AT+CREG?\'"));
      Serial.println(F("    *****"));
      if(sendATcommand("AT+CREG?", "+CREG: 0,1", AT_COMMAND_TIMEOUT))
      {
        Serial.println(F("Response: +CREG: 0,1"));
        Serial.println(F("Info: Registered on the home network")); 
        countTests++;
      }
      else
      {
        Serial.println(F("Response: Unexpected response"));
        Serial.println(F("Info: Not registered to a network"));
      }
      while(Serial1.available() > 0)
      {
        temp = Serial1.read();
        Serial.write(temp);
      }

      delay(1000);
    }
    Serial.println(HORIZONTAL_LINE);
    Serial.println("|       Test has ended         |");
    Serial.println(HORIZONTAL_LINE);
  }

}
int8_t sendATcommand(char* ATcommand, char* expected_answer,
unsigned int timeout)
{

  uint8_t x=0,  answer=0;
  char response[100];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialize the string

  delay(100);

  while( Serial1.available() > 0) Serial1.read();    // Clean the input buffer

  Serial1.print(ATcommand);    // Send the AT command
  Serial1.write(13);           // Send carriage return char

  x = 0;
  previous = millis();

  // this loop waits for the answer
  do{

    if(Serial1.available() != 0){
      response[x] = Serial1.read();
      x++;
      // check if the desired answer is in the response of the module
      if (strstr(response, expected_answer) != NULL)
      {
        answer = 1;
      }
    }
   // Serial.println(response);
    // Waits for the answer with time out
  }
  while((answer == 0) && ((millis() - previous) < timeout));

  return answer;
}



