//esp32
#define ANEMOMETRE 39 //carte 39   //pin D3, interruption n°1
#define PI        3.1415
#define RAYON     0.065  //rayon en mètre de l'anémomètre en mètre

volatile unsigned long Rotations         = 0;       // Cup rotation counter used in interrupt routine
unsigned long WindSampleTimePrevMillis   = 0;       // Store the previous millis   
unsigned long ReportTimerLongPrevMillis  = 0;       // Store the previous millis
unsigned long  lastWindIRQ               =0;
const unsigned long WindSampleTime       = 2000;    // Timer in milliseconds for windspeed (gust)calculation    
const unsigned long ReportTimerLong      = 600000;  // Timer in milliseconds (10 min) to report temperature, average wind and restart max/min calculation      

float windSpeed                          = 0;       // Wind speed in miles per hour sampled during WindSampleTime
float windSpeed_sum                      = 0;       // Summarizing of all windspeed measurements to calculate average wind
float WindSpeedAverage                   = 0;       // Average wind measured during ReportIntervall
float windSamples                        = 0;
float windGustin                         = 0;


void setup(){

Serial.begin(9600); 
pinMode(ANEMOMETRE,INPUT_PULLUP); 
attachInterrupt(ANEMOMETRE,interruptAnemometre,FALLING) ;

}

ICACHE_RAM_ATTR void interruptAnemometre()
// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
{
  float deltaTime = micros() - lastWindIRQ;
  if (deltaTime > 15) // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
  {
    lastWindIRQ = micros(); //Grab the current time
    
    Rotations++; //There is 1.492MPH for each click per second.
    //Serial.print(Rotations);
  }
}
 
void get_WindSpeed() {
  if((millis() - WindSampleTimePrevMillis) > WindSampleTime ) {   // Calculate gust wind speed  .....windsampletime=2000                 

  Serial.print("Rotations "); 
  Serial.print(Rotations);
                
    
  windSpeed  = 2*PI*RAYON*3.6;
  windSpeed       *= Rotations;
  Rotations        = 0; //after I take the sample I do need to reset the number of clicks!

   Serial.print("  WindSpeed (km/h)  "); 
   Serial.println(windSpeed);
  
  windSpeed_sum   += windSpeed;
  windSamples++;

  
  float averageWindSpeed=(windSpeed_sum / windSamples);
  
  WindSampleTimePrevMillis = millis();
  }
}

void getAverageWindSpeed() {
    
     if (windSamples > 0) {
       WindSpeedAverage = windSpeed_sum / windSamples; }  //  Calculate average windspeed, if rotations is 0 the division fails 
     else {WindSpeedAverage = 0;}                                                                           
}

void resetAverageWindSpeed() {

    WindSpeedAverage = 0;
    windSamples = 0;
    windSpeed_sum = 0;
}
void loop(){ 
    
    get_WindSpeed();
    
    if((millis() - ReportTimerLongPrevMillis) > ReportTimerLong ) { //report timelong=600000 = 10min

        getAverageWindSpeed(); 

        Serial.print("Average WindSpeed (m/s) "); 
        Serial.print(WindSpeedAverage);
        Serial.print("    windSamples ");
        Serial.println(windSamples);
        Serial.println("");

        
        ReportTimerLongPrevMillis = millis();
        resetAverageWindSpeed();
   }

}
