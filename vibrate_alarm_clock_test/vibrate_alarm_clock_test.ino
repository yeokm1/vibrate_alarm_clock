#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <RTC_DS1307.h>
#include "pitches.h"


#define BUTTON_SET_PIN A4
#define BUTTON_ADJUST_PIN A5

#define MOTOR_PIN1 A3
#define MOTOR_PIN2 A2
#define MOTOR_SLEEP_PIN 7 //D7

#define SPEAKER_PIN 9 //D9
#define LCD_OFF_PIN 6 //D6

#define RXLED 17
#define OLED_RESET A0

#define ADC_PRECISION 1024
#define VOLTAGE_MEASURE_PIN A1
#define VOLTAGE_OF_VCC_MV 3310

RTC_DS1307 RTC;
Adafruit_SSD1306 display(OLED_RESET);

int melody[] = {
  NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4};
  
int noteDurations[] = {
  4, 8, 8, 4,4,4,4,4
};
  
boolean showLCD = true;  
  
void setup(){

  Serial.begin(9600);
  
 //Turn off TX and RX pin
  pinMode(RXLED, OUTPUT);
  digitalWrite(RXLED, LOW);
  TXLED1;
  
  
  // initialize with the OLED with I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D); 
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  Wire.begin();
  RTC.begin();
  
  if (!RTC.isrunning()) {
    Serial.println("RTC is NOT running");
  }

  // This section grabs the current datetime and compares it to
  // the compilation time.  If necessary, the RTC is updated.
  DateTime now = RTC.now();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  
  if (now.unixtime() < compiled.unixtime()) {
    Serial.println("RTC is older than compile time! Updating");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  } 
  
  pinMode(BUTTON_SET_PIN, INPUT); 
  pinMode(BUTTON_ADJUST_PIN, INPUT); 
  pinMode(LCD_OFF_PIN, INPUT);
  pinMode(VOLTAGE_MEASURE_PIN, INPUT);
  
  
  pinMode(MOTOR_PIN1, OUTPUT); 
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(MOTOR_SLEEP_PIN, OUTPUT);
  
  

  
  
  playTune();
  
  //Motor one direction
  turnMotor(true,true);

  delay(500);
  
  //Motor another direction
  turnMotor(true,false);
  
  delay(500);
  
  //Motor off
  turnMotor(false,false);
  
}


void loop(){
  
  int setPinState = digitalRead(BUTTON_SET_PIN);
  int adjustPinState = digitalRead(BUTTON_ADJUST_PIN);
  int lcdOffPinState = digitalRead(LCD_OFF_PIN);
  int voltageADCReading = analogRead(VOLTAGE_MEASURE_PIN);
  
  //Press both to play tune
  if(setPinState == HIGH && adjustPinState == HIGH){
    turnMotor(false,false);
    Serial.println("Tune");
    playTune();
  
  } else if(setPinState == HIGH){
     turnMotor(true,true);
    Serial.println("Motor on2, setpin");
    
  } else if(adjustPinState == HIGH){
     turnMotor(true,false);  
      Serial.println("Motor on2, adjust pin");
    
  }  else {
    turnMotor(false,false);
    Serial.println("Motor off");
  }
  
 if(lcdOffPinState == HIGH){
    Serial.println("LCD value change");
    showLCD = !showLCD;
  }
  
  int batteryVoltage = getMVOfBattery(voltageADCReading);
  Serial.print("VoltageADC: ");
  Serial.println(voltageADCReading);
  
  Serial.print("Voltage: ");
  Serial.print(batteryVoltage);
  Serial.println("mV");
  
  
    DateTime now = RTC.now();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    

    display.clearDisplay();

    
    if(showLCD){
      Serial.println("Update Display");
      display.setTextSize(4);

      display.setCursor(0,0);
      String timeString = generateTimeString(now.hour(), now.minute(), now.second());
      display.print(timeString);
      char buff[3];

      display.setCursor(116,22);
      sprintf(buff, "%02d", now.second());
      display.setTextSize(1);
      display.println(buff);
  
      display.setTextSize(1);
      String dateString = generateDateString(now.year(), now.month(), now.day());
      display.println(dateString);
      
      display.print("Voltage: ");
      display.print(batteryVoltage);
      display.println("mV");

    } 

    display.display();
  
  
  delay(200);
  
  

  
  
}

int getMVOfBattery(int ADCReading){
  return ((float) ADCReading / ADC_PRECISION) * 2 * VOLTAGE_OF_VCC_MV;
}

void playTune(){
    for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000/noteDurations[thisNote];
    tone(SPEAKER_PIN, melody[thisNote],noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(SPEAKER_PIN);
    }

}


String generateTimeString(int hour, int minute, int second){
  char buff[3];

  sprintf(buff, "%02d", hour);
  String hourString = buff;

  sprintf(buff, "%02d", minute);
  String minuteString = buff;
 
  String result = hourString + ":" + minuteString;

  return result;
}

String generateDateString(int year, int month, int day)
{
  String dayOfWeek = getDayOfTheWeek(year, month, day);  
  String monthString = getMonth(month);
  String result = dayOfWeek + ", " + day + " " + monthString + " " + year;
  
  return result;
}

// From http://stackoverflow.com/a/21235587
String getDayOfTheWeek(int y, int m, int d) {
  String weekdayname[] = {"Sun", "Mon", "Tue",
  "Wed", "Thu", "Fri", "Sat"};

  int weekday = (d+=m<3?y--:y-2,23*m/9+d+4+y/4-y/100+y/400)%7;
  
  
  return weekdayname[weekday];
}


String getMonth(int month){
  switch(month){
    case 1: return "Jan";
    case 2: return "Feb";
    case 3: return "Mar";
    
    case 4: return "Apr";
    case 5: return "May";
    case 6: return "Jun";
    
    case 7: return "Jul";
    case 8: return "Aug";
    case 9: return "Sep";
    
    case 10: return "Oct";
    case 11: return "Nov";
    case 12: return "Dec";
    default : return "Nil";
  }
}


void turnMotor(bool state, bool direction){
  if(state){
    digitalWrite(MOTOR_SLEEP_PIN, HIGH);
    if(direction){
      digitalWrite(MOTOR_PIN1, HIGH);
      digitalWrite(MOTOR_PIN2, LOW);     
    } else {
      digitalWrite(MOTOR_PIN1, LOW);
      digitalWrite(MOTOR_PIN2, HIGH); 
    }
    
  } else {
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, LOW);  
    digitalWrite(MOTOR_SLEEP_PIN, LOW);
  }

}
