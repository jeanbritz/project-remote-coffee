
// Special characters
#define CR (13)
#define LF (10)
#define HORIZONTAL_LINE ("======================================================")

// Volatile variables
volatile boolean primary = true;
volatile double primary_frac = 0;
volatile double second_frac = 0;

char received_int[3];
int8_t cnt = 0;
int current_duty_cycle = 0;
boolean new_input = false;

// Custom baud rates
#define MEGA_BAUD_RATE (9600)

void setup()
{
  Serial.begin(MEGA_BAUD_RATE);
  Serial.println(HORIZONTAL_LINE);
  Serial.println(F("Type in the duty cycle  to run the test. Type `0` to stop the test"));
  Serial.println(F("Accceptable input => [0, 100]"));
  Serial.println(HORIZONTAL_LINE);
}

void loop()
{
  if(Serial.available() > 0)
  {
    // Add a delay so that the data can be transferred to the receive buffer
    delay(500);

    while(Serial.available() > 0)
    {
      if(Serial.peek() != CR & Serial.peek() != LF)
      {
        received_int[cnt++] = (char)Serial.read();
      }
      else
      {
        Serial.read();
      }

    }
    if(Serial.available() == 0)
    {
      new_input = true;
    }
  }
  if(new_input == true)
  {
    current_duty_cycle = startTRIACpulse(atoi(received_int));
    memset(received_int, '\0', 3);    // clear array
    cnt = 0;
    Serial.print("Current Duty Cycle = ");
    Serial.println(current_duty_cycle, DEC);
    new_input = false;
  }
}

void stopTRIACpulse()
{
  startTRIACpulse(10);
  delay(1000);
  startTRIACpulse(0);
}

int startTRIACpulse (int duty_cycle)
{
  int prev_duty_cycle = current_duty_cycle;
  if(prev_duty_cycle == duty_cycle)
  {
    Serial.println(F("Same duty cycle. Nothing is necessary to change."));
    return prev_duty_cycle;
  }
  else if(duty_cycle >= 0 & duty_cycle <= 100)
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
    return duty_cycle;
  }
  else
  {
    Serial.println(F("ERROR : Invalid duty cycle. Range = [0, 100]"));
    return prev_duty_cycle;
  }
}

ISR(TIMER3_COMPA_vect)
{
  if(current_duty_cycle == 100)
  {
    digitalWrite(2, HIGH);
  }
  else if (current_duty_cycle == 0)
  {
    primary_frac = 0;
    second_frac = 15624;
  }
  else
  {
    primary_frac = 15624*(current_duty_cycle/100.0);
    second_frac = 15624*(1 - (current_duty_cycle/100.0));


    if(primary)
    {

      digitalWrite(2, HIGH);
      // on time
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



