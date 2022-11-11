#include <WebServer.h>             
#include <WiFi.h>
#include <TimeLib.h>

//...............BMP085...............................
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>


#define SDA_2 33
#define SCL_2 32

TwoWire I2Ctwo = TwoWire(1);

Adafruit_BMP085 bme2;

//...............Anémometre...........................
#define ANEMOMETRE  39  

#define PI        3.1415
#define RAYON     0.065  //rayon en mètre de l'anémomètre en mètre
volatile unsigned long Rotations         = 0;       // Cup rotation counter used in interrupt routine
unsigned long WindSampleTimePrevMillis   = 0;       // Store the previous millis   
unsigned long ReportTimerLongPrevMillis  = 0;       // Store the previous millis
unsigned long  lastWindIRQ               =0;
const unsigned long WindSampleTime       = 1000;    // Timer in milliseconds for windspeed (gust)calculation    
const unsigned long ReportTimerLong      = 600000;  // Timer in milliseconds (10 min) to report temperature, average wind and restart max/min calculation  600000    

float windSpeed                          = 0;       // Wind speed in miles per hour sampled during WindSampleTime
float windSpeed_sum                      = 0;       // Summarizing of all windspeed measurements to calculate average wind
float WindSpeedAverage                   = 0;       // Average wind measured during ReportIntervall
float windSamples                        = 0;
float windGustin                         = 0;

//...............................Pluviométre..................................
#include <TimeLib.h>
#include "RTClib.h"
RTC_DS3231 rtc;
#define PLUVIOMETRE 34
#define VALEUR_PLUVIOMETRE 0.3 //valeur en mm d'eau à chaque bascule d'auget
volatile unsigned int countPluviometre = 0;
int currentPulseCount;
unsigned long dailyPulseCount;
float currentRain;
float DailytRain;
unsigned long  previousMillis =  0;
unsigned long delaipluviometre =  1000;   //1 sec

//........................windVanne..................
const byte WDIR = 36;
int analogWindDir = 0;
float windDir = -1;
//...............................................

 uint32_t chipId = 0;
 char buffer1[30];
#define NUM_RELAYS  8 
int relayGPIOs[NUM_RELAYS] = {13,12,02,27,26,25,33,32};
enum relay {Rly1=13,Rly2 =12,Rly3 =02,Rly4 =27,Rly5 =26,Rly6 =25,Rly7=33,Rly8 =32};     
 

#define debug true     
  
float Temperature;
float Humidity;
float Pressure;


enum { NONE=0, INDEX, XML };
WiFiServer server(80);
WiFiServer tcpServer(0);
WebServer Server;

#define Inp1  36
#define Inp2  39
#define Inp3  34
#define Inp4  35
#define Inp5  23
#define Inp6  03
#define Inp7  15
#define Inp8  14

#define red 5
#define green 0
const char* ssid = "";//"REPLACE_WITH_YOUR_SSID";
const char* password = "";//"REPLACE_WITH_YOUR_PASSWORD";

char Inputs[8] = {13,12,02,27,26,25,33,32};

void setup()
{ 

    Serial.begin(115200);
    delay(10);
    

  bool status2 = bme2.begin(0x76, &I2Ctwo);  
 if (!status2) {
    Serial.println("Could not find a valid BME280_2 sensor, check wiring!");
    while (1);
  }
  
  Serial.println();

for(int i=1; i<=NUM_RELAYS; i++)

{  pinMode(relayGPIOs[i-1], OUTPUT); 
   digitalWrite(relayGPIOs[i-1], LOW);}
  
    pinMode(red, OUTPUT);
    pinMode(green, OUTPUT);
    pinMode(Inp1, INPUT);
    pinMode(Inp2, INPUT);
    pinMode(Inp3, INPUT);
    pinMode(Inp4, INPUT);
    pinMode(Inp5, INPUT);
    pinMode(Inp6, INPUT);
    pinMode(Inp7, INPUT);
    pinMode(Inp8, INPUT);

    setup_wifi();
    server.begin();
     rtc.begin();
   
  
   
  pinMode(ANEMOMETRE,INPUT); 
  attachInterrupt(ANEMOMETRE,interruptAnemometre,FALLING) ;
  pinMode(PLUVIOMETRE,INPUT_PULLUP); 
  attachInterrupt(PLUVIOMETRE,interruptPluviometre,FALLING) ;

}
//............................pluviométre.......................
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
  DateTime now = rtc.now();
  int  ura=now.hour();
  int minuta=now.minute();
  
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
  
  if((ura== 0)&&(minuta==0)) {
   currentPulseCount = 0.0; // clear daily-rain at midnight
                          
  }  
 if((ura== 23)&&(minuta==59)) {
   
   dailyPulseCount= currentPulseCount;
                                      
  }
}
//..........Anémometre......................................
ICACHE_RAM_ATTR void interruptAnemometre()
{
  float deltaTime = micros() - lastWindIRQ;
  if (deltaTime > 15) // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
  {
    lastWindIRQ = micros(); //Grab the current time
    
    Rotations++; //There is 1.492MPH for each click per second.
    Serial.print(Rotations);
  }
}
 

void get_WindSpeed() {
  if((millis() - WindSampleTimePrevMillis) > WindSampleTime ) {   // Calculate gust wind speed  .....windsampletime=2000                 
 
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

     Serial.print("Average WindSpeed (km/h) "); 
     Serial.print(WindSpeedAverage);
}

void resetAverageWindSpeed() {

    WindSpeedAverage = 0;
    windSamples = 0;
    windSpeed_sum = 0;
}

//..............................windvanne............................................
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
  {windDir = 225;//s7i7a..........
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if (adc ==4095 )
  {windDir = 0;
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if (adc <4036 && adc<4049 ) 
  {windDir = 315;//s7i7a
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  else if (adc <3795 && adc<4030 ) 
  {windDir = 270;//
  float Girouette=windDir;
  Serial.println(Girouette);
  }
  
}
//..............................................

void loop(){

 modeHttp();

}


void wifi_connect(void)
{
    unsigned int x;
    
    digitalWrite(red, HIGH); digitalWrite(green, LOW);
    Serial.println("");
 
   server.begin();
  
}

void setup_wifi() {
  delay(10);
  digitalWrite(red, HIGH); digitalWrite(green, LOW);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    // wait 10 seconds for connection:
    //delay(10000);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("WiFi connected: " + WiFi.localIP().toString());
  Serial.println(WiFi.getHostname());
}
//..................httpServer.........................................
void modeHttp(void)
 {
  WiFiClient client = server.available();   // listen for incoming clients

 int pos, page = 0;
 
  if (client) {                             // if you get a client,
Temperature =bme2.readTemperature();
Pressure    =bme2.readPressure() / 100.0F;
get_WindSpeed();
 if((millis() - ReportTimerLongPrevMillis) > ReportTimerLong ) { //report timelong=600000 = 10min

        getAverageWindSpeed();
        //delay(2000); 

        ReportTimerLongPrevMillis = millis();
   }
   //resetAverageWindSpeed();c'est pas la peine

 //   Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
//        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {

            if(page==INDEX) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              // the content of the HTTP response follows the header:
 
  
              client.println("<!DOCTYPE HTML><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Smart WiFi Controller </title></head><body>");
              client.println("<p style='text-align:center'><b><p style='color:blue'>Smart Controller </b> <br></p>");
           //client.println("<style>button{width:44%;height:35px;margin:5px;}</style>");-->
            client.println("<style> html {font-family: Arial; display: inline-block; text-align: center;}");
   client.println(" h2 {font-size: 3.0rem;}  p {font-size: 3.0rem;}");
   client.println(" body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}");
  client.println("  .switch {position: relative; display: inline-block; width: 120px; height: 68px}");
   client.println(" .switch input {display: none}");
    client.println(".slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}");
  client.println("  .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}");
   client.println(" input:checked+.slider {background-color: #2bf320}");
   client.println(" input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}</style>");
              client.println("<button id='Rly1'; onclick='setRly(1)'; >Relay 1</button>");
              client.println("<button id='Rly2'; onclick='setRly(2)'; >Relay 2</button>");
              client.println("<button id='Rly3'; onclick='setRly(3)'; >Relay 3</button>");
              client.println("<button id='Rly4'; onclick='setRly(4)'; >Relay 4</button>");
              client.println("<button id='Rly5'; onclick='setRly(5)'; >Relay 5</button>");
              client.println("<button id='Rly6'; onclick='setRly(6)'; >Relay 6</button>");
              client.println("<button id='Rly7'; onclick='setRly(7)'; >Relay 7</button>");
              client.println("<button id='Rly8'; onclick='setRly(8)'; >Relay 8</button>");

              client.println("<br><br><div> Inputs 1<input type='radio' value='' id='Inp1'>");
              client.println("<input type='radio' id='Inp2'>");
              client.println("<input type='radio' id='Inp3'>");
              client.println("<input type='radio' id='Inp4'>");
              client.println("<input type='radio' id='Inp5'>");
              client.println("<input type='radio' id='Inp6'>");
              client.println("<input type='radio' id='Inp7'>");
              client.println("<input type='radio' id='Inp8'>8 </div>");
              
client.println("</th></tr>");//..................................
client.println("<table><tr><th>MEASUREMENT</th><th><th>VALUE</th></tr>");

client.println("<tr><td>Temp. Celsius</td><th><td><span class=\"sensor\">");
client.println(Temperature);
client.println(" &deg;C</span></td></tr>"); 

client.println("<td>Pressure</td><th><td><span class=\"sensor\">");
client.println(Pressure);
client.println(" Pa;</span></td></tr>"); 

client.println("<td>windSpeed</td><th><td><span class=\"sensor\">");
client.println(windSpeed);
client.println(" km/h;</span></td></tr>"); 


client.println("<td>WindSpeedAverage</td><th><td><span class=\"sensor\">");
client.println(WindSpeedAverage);
client.println(" km/h;</span></td></tr>");

client.println("<td>currentRain</td><th><td><span class=\"sensor\">");
client.println(currentRain);
client.println(" mm;</span></td></tr>");

client.println("<td>DailytRain</td><th><td><span class=\"sensor\">");
client.println(DailytRain);
client.println(" mm;</span></td></tr>");

client.println("<td>windDir </td><th><td><span class=\"sensor\">");
client.println(windDir);
client.println(" </span></td></tr>");

              
              client.println("<script>");
              client.println("var xhttp = new XMLHttpRequest();");
              client.println("var xmlDoc;");
              client.println("var Rly=0;");
              client.println("var Working=50;");
              client.println("setInterval(toggleRly, 10);");              
              client.println("xhttp.onreadystatechange = function() {");
              client.println("    if (this.readyState == 4 && this.status == 200) {");
              client.println("        myFunction(this);");
              client.println("    }");
              client.println("};");

              client.println("function setRly(num) {");
              client.println("    Rly = num");
              client.println("};");
              
              client.println("function getValue(tag) {");
              client.println("    var x = xmlDoc.getElementsByTagName(tag)[0];");
              client.println("    var y = x.childNodes[0].nodeValue;");
              client.println("    return y;");
              client.println("};");
              client.println("function toggleRly() {");
              client.println("  var file = '';");
              client.println("  if(Working>0) { --Working; return; }");
              client.println("  switch(Rly) {");
              client.println("    case 0: file = \"status.xml\"; break;");
              client.println("    case 1: file = \"?Rly1=2\"; break;");
              client.println("    case 2: file = \"?Rly2=2\"; break;");
              client.println("    case 3: file = \"?Rly3=2\"; break;");
              client.println("    case 4: file = \"?Rly4=2\"; break;");
              client.println("    case 5: file = \"?Rly5=2\"; break;");
              client.println("    case 6: file = \"?Rly6=2\"; break;");
              client.println("    case 7: file = \"?Rly7=2\"; break;");
              client.println("    case 8: file = \"?Rly8=2\"; break;");
              client.println("  };");
              client.println("  Rly = 0; Working = 500;");
              client.println("  xhttp.open(\"GET\", file, true);");
              client.println("  xhttp.send();");
              client.println("}");
              client.println("function myFunction(xml) {");
              client.println("  xmlDoc = xml.responseXML;");
              client.println("  document.getElementById('Rly1').style.backgroundColor = (getValue('RLY1')=='on') ? 'rgba(255,0,0,1)' : 'rgba(0,255,0.1)';");
              client.println("  document.getElementById('Rly2').style.backgroundColor = (getValue('RLY2')=='on') ? 'rgba(255,0,0,1)' : 'rgba(0,255,0.1)';");
              client.println("  document.getElementById('Rly3').style.backgroundColor = (getValue('RLY3')=='on') ? 'rgba(255,0,0,1)' : 'rgba(0,255,0.1)';");
              client.println("  document.getElementById('Rly4').style.backgroundColor = (getValue('RLY4')=='on') ? 'rgba(255,0,0,1)' : 'rgba(0,255,0.1)';");
              client.println("  document.getElementById('Rly5').style.backgroundColor = (getValue('RLY5')=='on') ? 'rgba(255,0,0,1)' : 'rgba(0,255,0.1)';");
              client.println("  document.getElementById('Rly6').style.backgroundColor = (getValue('RLY6')=='on') ? 'rgba(255,0,0,1)' : 'rgba(0,255,0.1)';");
              client.println("  document.getElementById('Rly7').style.backgroundColor = (getValue('RLY7')=='on') ? 'rgba(255,0,0,1)' : 'rgba(0,255,0.1)';");
              client.println("  document.getElementById('Rly8').style.backgroundColor = (getValue('RLY8')=='on') ? 'rgba(255,0,0,1)' : 'rgba(0,255,0.1)';");
              client.println("  if(getValue('INP1')=='1') document.getElementById('Inp1').checked = true; else document.getElementById('Inp1').checked = false;");
              client.println("  if(getValue('INP2')=='1') document.getElementById('Inp2').checked = true; else document.getElementById('Inp2').checked = false;");
              client.println("  if(getValue('INP3')=='1') document.getElementById('Inp3').checked = true; else document.getElementById('Inp3').checked = false;");
              client.println("  if(getValue('INP4')=='1') document.getElementById('Inp4').checked = true; else document.getElementById('Inp4').checked = false;");
              client.println("  if(getValue('INP5')=='1') document.getElementById('Inp5').checked = true; else document.getElementById('Inp5').checked = false;");
              client.println("  if(getValue('INP6')=='1') document.getElementById('Inp6').checked = true; else document.getElementById('Inp6').checked = false;");
              client.println("  if(getValue('INP7')=='1') document.getElementById('Inp7').checked = true; else document.getElementById('Inp7').checked = false;");
              client.println("  if(getValue('INP8')=='1') document.getElementById('Inp8').checked = true; else document.getElementById('Inp8').checked = false;");
              
              client.println("  Working=0;");
              client.println("}");

              client.println("</script>");
              client.println("");
  
              client.print("</body>");
              
              // The HTTP response ends with another blank line:
              client.println();             
            }
            else if(page==XML) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:application/xml");
              client.println();
              client.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
              client.println("<Adviot>");
              client.println("<RELAYS>");
              if(digitalRead(Rly1)) client.println("<RLY1>on</RLY1>"); else client.println("<RLY1>off</RLY1>");
              if(digitalRead(Rly2)) client.println("<RLY2>on</RLY2>"); else client.println("<RLY2>off</RLY2>");
              if(digitalRead(Rly3)) client.println("<RLY3>on</RLY3>"); else client.println("<RLY3>off</RLY3>");
              if(digitalRead(Rly4)) client.println("<RLY4>on</RLY4>"); else client.println("<RLY4>off</RLY4>");
              if(digitalRead(Rly5)) client.println("<RLY5>on</RLY5>"); else client.println("<RLY5>off</RLY5>");
              if(digitalRead(Rly6)) client.println("<RLY6>on</RLY6>"); else client.println("<RLY6>off</RLY6>");
              if(digitalRead(Rly7)) client.println("<RLY7>on</RLY7>"); else client.println("<RLY7>off</RLY7>");
              if(digitalRead(Rly8)) client.println("<RLY8>on</RLY8>"); else client.println("<RLY8>off</RLY8>");
              client.println("</RELAYS>");
              client.println("<INPUTS>");
              if(!digitalRead(Inp1)) client.println("<INP1>1</INP1>"); else client.println("<INP1>0</INP1>");
              if(!digitalRead(Inp2)) client.println("<INP2>1</INP2>"); else client.println("<INP2>0</INP2>");
              if(!digitalRead(Inp3)) client.println("<INP3>1</INP3>"); else client.println("<INP3>0</INP3>");
              if(!digitalRead(Inp4)) client.println("<INP4>1</INP4>"); else client.println("<INP4>0</INP4>");
              if(!digitalRead(Inp5)) client.println("<INP5>1</INP5>"); else client.println("<INP5>0</INP5>");
              if(!digitalRead(Inp6)) client.println("<INP6>1</INP6>"); else client.println("<INP6>0</INP6>");
              if(!digitalRead(Inp7)) client.println("<INP7>1</INP7>"); else client.println("<INP7>0</INP7>");
              if(!digitalRead(Inp8)) client.println("<INP8>1</INP8>"); else client.println("<INP8>0</INP8>");
              
              client.println("</INPUTS>");
              client.println("</Adviot>");
              client.println();                  
            }
            else {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              client.println();              
            }
            // break out of the while loop:
            break;
          } else {    // check for a GET command
            if(currentLine.startsWith("GET / ")) page=INDEX;
            else if(currentLine.startsWith("GET /INDEX.HTM")) page=INDEX;
            else if(currentLine.startsWith("GET /STATUS.XML")) page=XML;
            else if(currentLine.startsWith("GET /?RLY")) page=XML;
            currentLine = "";
          }
          
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          c = toupper(c);
          currentLine += c;      // add it to the end of the currentLine
        }

        if(currentLine.length()==12) {
          pos = currentLine.indexOf('?');
          if( (currentLine.substring(pos, pos+4) == "?RLY") && (pos>0) ) {
            int rly = currentLine.charAt(pos+4)-0x30;
            int action = currentLine.charAt(pos+6)-0x30;
            switch(rly) {
              case 1:
                switch(action) {
                  case 0: digitalWrite(Rly1, 0); break;
                  case 1: digitalWrite(Rly1, 1); break;
                  case 2: digitalWrite(Rly1, !digitalRead(Rly1)); break;
                }
                break;
              case 2:
                switch(action) {
                  case 0: digitalWrite(Rly2, 0); break;
                  case 1: digitalWrite(Rly2, 1); break;
                  case 2: digitalWrite(Rly2, !digitalRead(Rly2)); break;
                }
                break;
              case 3:
                switch(action) {
                  case 0: digitalWrite(Rly3, 0); break;
                  case 1: digitalWrite(Rly3, 1); break;
                  case 2: digitalWrite(Rly3, !digitalRead(Rly3)); break;
                }
                break;
              case 4:
                switch(action) {
                  case 0: digitalWrite(Rly4, 0); break;
                  case 1: digitalWrite(Rly4, 1); break;
                  case 2: digitalWrite(Rly4, !digitalRead(Rly4)); break;
                }
                break;
              case 5:
                switch(action) {
                  case 0: digitalWrite(Rly5, 0); break;
                  case 1: digitalWrite(Rly5, 1); break;
                  case 2: digitalWrite(Rly5, !digitalRead(Rly5)); break;
                }
                break;
              case 6:
                switch(action) {
                  case 0: digitalWrite(Rly6, 0); break;
                  case 1: digitalWrite(Rly6, 1); break;
                  case 2: digitalWrite(Rly6, !digitalRead(Rly6)); break;
                }
                break;
              case 7:
                switch(action) {
                  case 0: digitalWrite(Rly7, 0); break;
                  case 1: digitalWrite(Rly7, 1); break;
                  case 2: digitalWrite(Rly7, !digitalRead(Rly7)); break;
                }
                break;
              case 8:
                switch(action) {
                  case 0: digitalWrite(Rly8, 0); break;
                  case 1: digitalWrite(Rly8, 1); break;
                  case 2: digitalWrite(Rly8, !digitalRead(Rly8)); break;
                }
                break;                
            }
          }  
        }
      }
    }
    // close the connection:
    client.stop();
//Serial.println("Client Disconnected.");
  }
 }
