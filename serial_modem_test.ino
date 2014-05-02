#include <Arduino.h>
#include <SoftwareSerial.h>


#define unoTXPin (13) // Uno -> GSM
#define unoRXPin (12) // Uno <- GSM
#define unoBaudRate (115200)

#define gsmBaudRate (9600)

SoftwareSerial gsmSerial (unoRXPin, unoTXPin);

char temp;
int8_t answer = 0;

void setup()
{

  Serial.begin(unoBaudRate);
  gsmSerial.begin(gsmBaudRate);
  answer = sendATcommand("AT", "OK", 2000);
  if(answer == 0)
	{
		Serial.print("Failure");
	}
	else
	{
		Serial.print("Success");	
	
	}
}

void loop()
{
	
	while(gsmSerial.available() > 0)
	{
		
			temp = (char)gsmSerial.read();
			Serial.print(">> ");Serial.print(temp);Serial.println();
			
		
		
	
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

	while( gsmSerial.available() > 0) gsmSerial.read();    // Clean the input buffer

	gsmSerial.print(ATcommand);    // Send the AT command
	gsmSerial.write(13);

	x = 0;
	previous = millis();

	// this loop waits for the answer
	do{

		if(gsmSerial.available() != 0){
			response[x] = gsmSerial.read();
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
