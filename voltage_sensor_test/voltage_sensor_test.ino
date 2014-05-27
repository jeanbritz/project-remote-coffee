#include <wiring.c>

// Constants
#define MAINS_VOLTAGE_FREQ_HZ (50)
#define MAINS_RMS_VOLTAGE (240)
#define ADC_SAMPLE_FREQ (8900)
#define MEASURED_DC_OFFSET_MILLIVOLTS (2410)
const int TOTAL_SAMPLES = round(ADC_SAMPLE_FREQ/MAINS_VOLTAGE_FREQ_HZ);
const int TRANSFORMER_RATIO = (20);

// Arrays
int samples[TOTAL_SAMPLES];
double filtered_samples[TOTAL_SAMPLES];


// Auxilary variablesva
double realVcc = 0;
int measuredoffset = 0;
int cnt = 0;
double aux = 0;
double trf_rms = 11.4;
double trf_peak = sqrt(2)*trf_rms;
double factor = (11*20)/1024;
void setup()
{

  Serial.begin(9600);
  realVcc = measureVcc();
  realVcc = 4960;
  measuredoffset = round((MEASURED_DC_OFFSET_MILLIVOLTS/realVcc)*1024);
  Serial.print("Actual Vcc = ");
  Serial.println(realVcc);
  Serial.print("Measured ADC signal offset = ");
  Serial.println(measuredoffset);
  obtain_samples();

  Serial.print("RMS Voltage : ");
  Serial.println(get_RMS_value());
}

void obtain_samples()
{
  int x = 0;
  // Obtaining the samples from ADC port labeled as A9
  // Note: If analogRead is used directly as a parameter in Serial.println() then your results
  // will be inaccurate.
  // ===========================================================================
  // ATTENTION : DON'T USE Serial.println(analogRead(A9)) TO GET MEASUREMENTS!!!!
  // ===========================================================================

  for(x = 0; x < TOTAL_SAMPLES; x++)
    samples[x] = analogRead(A9);

  // Remove the DC offset of realVcc/2
  // TODO: - Use a band-pass digital filter instead of eliminating the offset by subtracting 512
  //         This filter will ensure that the RMS Voltage remain constant if there are
  //         spikes on the mains voltage.
  //       - Use another analog input to measure the real DC offset voltage that is appearing at the VIN+
  //         terminal of the op-amp
  
  for(x = 2; x < TOTAL_SAMPLES; x++)
  {
    aux = (((float)(samples[x]-measuredoffset))/1024)*(realVcc/1000);
    aux = aux * (16/1.53)*21;
    filtered_samples[cnt++] = aux; //- w0*samples[x-1] + samples[x-2];
    //Serial.println(filtered_samples[cnt-1]);  
  }
  for(x=0; x<cnt; x++)
  {
    Serial.println(filtered_samples[x]); 
  }

}

double get_RMS_value()
{
  double temp;
  int i;
  for(i = 0; i < TOTAL_SAMPLES; i++)
  {
    // Summation of all samples^2
    temp += filtered_samples[i] * filtered_samples[i];
  }
  temp = temp/TOTAL_SAMPLES;
  temp = sqrt(temp);
  return round(temp);
}

void loop()
{



}

double measureVcc() {
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


