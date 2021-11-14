// 22/01/2021 Last Update
// Konstantinos Chouliaras

#include <DS3232RTC.h>        // https://github.com/JChristensen/DS3232RTC
#include <LowPower.h>         // https://github.com/rocketscream/Low-Power
#include <Streaming.h>        // https://www.arduino.cc/reference/en/libraries/streaming/
#include <HX711.h>            // https://github.com/RobTillaart/HX711
#include <SoftwareSerial.h>   // https://www.arduino.cc/en/Reference/softwareSerial
#include <Wire.h>             // https://www.arduino.cc/en/Reference/wire

#define DOUT  11
#define CLK  12
#define control 9 //mosfet pin

//Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(8, 7); //SIM800L Tx & Rx is connected to Arduino #8 & #7
HX711 scale;

double calibration_factor = -23498; // this calibration factor is adjusted according to my load cell
double units;
double kilos;
const int wakeupPin = 2 ;
int alarmHour = 0;
String minima, minima2;
volatile int woken = 0 ;
time_t tm;
const time_t ALARM_INTERVAL(86400); //3600 = 1hour        86400 = 24hours 


//==============================================================================================================================
void wakeUp()
{
    woken = 1 ;
}


//==============================================================================================================================
void setup()
{
  digitalWrite(control, HIGH); //mosfet on
  delay(200);
  initializeScale();
  minima2= String("apotyxia");

  pinMode(wakeupPin, INPUT_PULLUP) ;
  

  Serial.begin(9600) ;
  delay(50) ;
  Wire.begin() ;
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  scaleActive();
  
  if(timeStatus() != timeSet)
      Serial.println("Unable to sync with the RTC");
  else
      Serial.println("RTC has set the system time");

  Serial.print("::: INITIAL TIME ") ;
  printDateTime((RTC.get())+7UL*3600UL) ;
  Serial.println() ;
  delay(50) ;
  // initialize the alarms to known values, 
  // clear the alarm flags, clear the alarm interrupt flags
  // ALARM_1 will trigger every hour
  // ALARM_2 will trigger every minute, at 0 seconds.
  RTC.setAlarm(ALM1_MATCH_MINUTES, 0, 0, 0, 0);
  RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);

   
  // clear both alarm flags
  RTC.alarm(ALARM_1);

  // We are going to output the alarm by going low onto the 
  // SQW output, which should be wired to wakeupPin
  RTC.squareWave(SQWAVE_NONE);

  // Both alarms should generate an interrupt
  RTC.alarmInterrupt(ALARM_1, true);
  RTC.alarmInterrupt(ALARM_2, false);
 
  
}

//==============================================================================================================================
void loop()
{
  digitalWrite(control, HIGH); //mosfet on
  tm = RTC.get();
  
  time_t alarmTime = tm + ALARM_INTERVAL;
  Serial.println(tm);
  Serial.println(alarmTime);
//  alarmHour =((tm / 60) % 60) + 5; //reapeat every 5 minutes (testing)
//  if (alarmHour >= 60)
//  {
//    alarmHour = alarmHour % 60;
//  }

  
  RTC.setAlarm(ALM1_MATCH_HOURS, second(alarmTime), minute(alarmTime), hour(alarmTime), 0);
  //RTC.setAlarm(ALM1_MATCH_MINUTES, 0, alarmHour, 0, 0); // for minutes testing

  RTC.alarmInterrupt(ALARM_1, true);
  
  scaleActive();
  
  //digitalWrite(control, HIGH); //mosfet on
  delay(3000);
  gsmActive();
  digitalWrite(control, LOW); //mosfet off

  attachInterrupt(digitalPinToInterrupt(wakeupPin), wakeUp, FALLING) ;
  mySerial.end();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF) ;
  detachInterrupt(digitalPinToInterrupt(wakeupPin)) ;
  RTC.alarm(ALARM_1) ; 
  RTC.alarm(ALARM_2) ;
  
}

//=====================================================================================
void scaleActive()
{
  Serial.print("Reading: ");
  units = scale.get_units(), 5;
  if (units < 0.00)
  {
    units = 0.0000;
  }
  kilos = units;

  Serial.print(kilos,3);
  Serial.println(" kilos"); 
  Serial.print("calibration_factor: ");
  Serial.println(calibration_factor);
}

//=======================================================================================
void gsmActive()
{
  mySerial.begin(9600);
  mySerial.println("AT+CGATT=0"); //GPRS off
  updateSerial();

  mySerial.println("AT+CBC"); //voltage
  delay(5000);
  minima2 = String(mySerial.readString()); //voltage sms
  updateSerial();
  delay(5000);
  
  mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  mySerial.println("AT+CMGS=\"+306937575598\"");//change ZZ with country code and xxxxxxxxxxx with phone number to sms
  updateSerial();

  minima = String("H kypseli einai: "+String(kilos,3)+" Kg\n");
  mySerial.println(minima2);
  mySerial.println(minima); //text content
  
  updateSerial();
  mySerial.write(26);
  updateSerial();
  mySerial.println("AT+CFUN=0"); //sleep mode
  updateSerial();
  delay(2000);
}

//===================================================================================================================================
void initializeScale()
{
  Serial.begin(9600);
  Serial.println("Remove all weight from scale");

  
  
  scale.begin(DOUT, CLK);
  scale.set_scale();
  scale.tare();  //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
  
  //Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);

  Serial.println("Initializing..."); 
  delay(1000);
  pinMode(control, OUTPUT);
  Serial.begin(9600);
  delay(5000);
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  
  
}
//=======================================================================================================================================
void printDateTime(time_t t)
{
    Serial << ((day(t)<10) ? "0" : "") << _DEC(day(t)) << ' ';
    Serial << monthShortStr(month(t)) << ' ' << _DEC(year(t)) << ' ';
    Serial << ((hour(t)<10) ? "0" : "") << _DEC(hour(t)) << ':';
    Serial << ((minute(t)<10) ? "0" : "") << _DEC(minute(t)) << ':';
    Serial << ((second(t)<10) ? "0" : "") << _DEC(second(t));
}

//=======================================================================================================================================
void updateSerial()
{
  delay(1000);
  while (Serial.available()) 
  {
    mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(mySerial.available()) 
  {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
  }
}
