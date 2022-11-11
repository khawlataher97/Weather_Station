//#include <TimeLib.h>
#include "RTClib.h"
#include <SPI.h>
#include <Wire.h>
RTC_DS3231 rtc;



#define PLUVIOMETRE 34 //34   //pin D2, interruption n°0
#define VALEUR_PLUVIOMETRE 0.3 //0.2794 //valeur en mm d'eau à chaque bascule d'auget
volatile unsigned int countPluviometre = 0;
int currentPulseCount;
unsigned long dailyPulseCount;
float currentRain;
float DailytRain;
unsigned long  previousMillis =  0;
unsigned long delaipluviometre =  1000;   //1 sec
int ura = 0;
int minuta = 0;



void setup(){
Serial.begin(9600); 
pinMode(PLUVIOMETRE,INPUT_PULLUP); 
attachInterrupt(PLUVIOMETRE,interruptPluviometre,FALLING) ;
rtc.begin();
  


}

ICACHE_RAM_ATTR void interruptPluviometre(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // debounce for a quarter second = max. 4 counts per second
  if (interrupt_time - last_interrupt_time > 350){//350milliseconde
  countPluviometre++;
  Serial.print(countPluviometre);
  last_interrupt_time = interrupt_time;}
  }
  
void interruptrain(){
 


   unsigned long currentMillis   = millis(); // read time passed 
  if (currentMillis - previousMillis >= delaipluviometre){
    noInterrupts();
    currentPulseCount+=countPluviometre; // add to current counter
    countPluviometre=0; // reset ISR counter
    interrupts();
    dailyPulseCount+=currentPulseCount;
    currentRain=currentPulseCount*VALEUR_PLUVIOMETRE;//.....................
    currentPulseCount=0;
    DailytRain=dailyPulseCount*VALEUR_PLUVIOMETRE;//.........
    Serial.print("currentRain : ");
    Serial.println(currentRain);  
    Serial.print("DailytRain : ");
    Serial.println(DailytRain);
    previousMillis=millis();
}
  
  if((ura== 01)&&(minuta==10)) {
   currentPulseCount = 0.0; // clear daily-rain at midnight
                          
  }  
 if((ura== 01)&&(minuta==11)) {
   
   dailyPulseCount= currentPulseCount;
                                      
  } 
}

void loop(){ 
  /*DateTime now = rtc.now();
  int ura=now.hour(),DEC;
 /* Serial.print("ura:");// interruption avec serial print jamais
    Serial.println(ura);

  int minuta=now.minute(),DEC;
   /* Serial.print("minuta:");
        Serial.println(minuta);*/
 
   interruptrain();
 }
