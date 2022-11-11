const byte WDIR = 36;
int analogWindDir = 0;
float windDir = -1;

void setup()
{ 

    Serial.begin(115200);
    delay(10);

}

 int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0;

  for (int x = 0; x < numberOfReadings; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;

  return (runningValue);
}

float windDirectionReading(){
  unsigned int adc;
  pinMode(WDIR, INPUT);
  adc = averageAnalogRead(WDIR); //get the current reading from the sensor
  analogWindDir = adc;

    
   if (  adc < 1148 && adc < 1189) 
   {windDir = 90;
   float Girouette=windDir;
   Serial.println(Girouette);}
  else if (adc < 1190 && adc < 2490)
  {windDir = 135;
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if (adc < 2492 && adc <2505 )
  {windDir = 180;
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if ( adc < 3072 && adc <3092)
  {windDir = 45;
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if (adc <3410 && adc<3425) 
  {windDir = 225;
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if (adc ==4095 )
  {windDir = 0;
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if (adc <4036 && adc<4049 ) 
  {windDir = 315;
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if (adc <3795 && adc<4030 ) 
  {windDir = 270;
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  
}

void loop(){

windDirectionReading();
}
