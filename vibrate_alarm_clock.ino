#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <RTC_DS1307.h>
#include "tune.h"

#define BUTTON_ALARM_SET_PIN A4
#define BUTTON_TIME_SET_PIN A5

#define MOTOR_PIN1 A3
#define MOTOR_PIN2 A2
#define MOTOR_SLEEP_PIN 7 //D7

#define SPEAKER_PIN 9 //D9
#define LCD_OFF_PIN 6 //D6

#define RXLED_PIN 17
#define OLED_RESET_PIN A0

#define INITIAL_TEXT "Happy 24th birthday\n Jason!\n\nBy: Yeo Kheng Meng\n(14 May 2014)\n\nCompiled on:"
#define INITIAL_TEXT_DELAY 8000
#define MIN_TIME_BETWEEN_BUTTON_PRESSES 100  //Debouncing purposes
#define BLINK_INTERVAL 100

#define MIN_TIME_BETWEEN_ALARM_STARTS 60000 //60 seconds
#define MIN_TIME_TO_CHANGE_MOTOR_DIRECTION 1500

RTC_DS1307 RTC;
Adafruit_SSD1306 display(OLED_RESET_PIN);


typedef enum {NORMAL, ALARM, SETTING_TIME, SETTING_ALARM} CLOCK_STATE ;
CLOCK_STATE currentState = NORMAL;

#define MIN_YEAR 2014
#define MAX_YEAR 2050

typedef enum {C_HOUR, C_MINUTE, C_SECOND, C_YEAR, C_MONTH, C_DAY} SETTING_TIME_STATE ;
SETTING_TIME_STATE settingClockProcess;

typedef enum {A_HOUR, A_MINUTE, A_TYPE} SETTING_ALARM_STATE ;
SETTING_ALARM_STATE settingAlarmProcess;

boolean alarmVibrate = false;
boolean alarmSound = false;

int alarmHour = 07;
int alarmMinute = 00;

unsigned long alarmLastStarted;

unsigned long lastVibration;
boolean lastMotorDirection;

unsigned long timeLastPressedAlarmSetButton;
unsigned long timeLastPressedTimeSetButton;
unsigned long timeLastPressedLCDOffButton;

boolean previousBlinkState;
unsigned long previousBlinkTime;


unsigned long timePlayedPreviousTone;
int toneNow = 0;
int previousNoteDuration;
  
boolean showLCD = true;

void loop(){

      
  DateTime now = RTC.now();
    
  int alarmSetButtonState = digitalRead(BUTTON_ALARM_SET_PIN);
  int timeSetButtonState = digitalRead(BUTTON_TIME_SET_PIN);
  int lcdOffPinState = digitalRead(LCD_OFF_PIN);
  
  boolean blinkOnForSetting = shouldBlinkNow();
  
  
  if(alarmSetButtonState == HIGH){
    processAlarmSetButtonPressed();
  }
  
  if(timeSetButtonState == HIGH){
    processTimeSetButtonPressed();
  }
  
  if(lcdOffPinState == HIGH){
    processLCDOffButtonPressed();
  }
  
  checkAndSoundAlarm(now.hour(), now.minute());
  soundAlarmAtThisPointIfNeeded();
  
  if(showLCD || currentState == ALARM){
    display.clearDisplay();

    writeDateTimeToDisplayBuffer(now, blinkOnForSetting);
    writeAlarmToDisplayBuffer(blinkOnForSetting);
    display.display();
  }
  
  


  
}

void checkAndSoundAlarm(int hour, int minute){
  
  unsigned long currentMillis = millis();
  
  if(hour == alarmHour && minute == alarmMinute && currentState == NORMAL
  && ((currentMillis - alarmLastStarted) > MIN_TIME_BETWEEN_ALARM_STARTS)
  && (alarmVibrate || alarmSound)){
    Serial.println("Alarm started");
    currentState = ALARM;
    alarmLastStarted = currentMillis;
  }
  
  
}

void soundAlarmAtThisPointIfNeeded(){

  if(currentState == ALARM){
    Serial.println("Alarm now");
    
    unsigned long currentMillis = millis();

    if(alarmVibrate && ((currentMillis - lastVibration) > MIN_TIME_TO_CHANGE_MOTOR_DIRECTION)){
      lastVibration = currentMillis;
      lastMotorDirection = !lastMotorDirection;
      turnMotor(true, lastMotorDirection);
    }
    
    currentMillis = millis();
    
    if(alarmSound && ((currentMillis - timePlayedPreviousTone) >  previousNoteDuration)){
      timePlayedPreviousTone = currentMillis;
      toneNow++;
      toneNow %= NUM_TONES;
      int noteDuration = 1000/getDurationAtThisPosition(toneNow);
      
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      previousNoteDuration = noteDuration * 1.3;
      tone(SPEAKER_PIN, getToneAtThisPosition(toneNow),noteDuration);
      
    }
   


    
  }
  
}

void turnOffLCD(){
  if(!showLCD){
    display.clearDisplay();
    display.display();
  }
}

void stopAlarm(){
  currentState = NORMAL;
  turnMotor(false, false);
  noTone(SPEAKER_PIN);
  toneNow = 0;
  
  turnOffLCD();
  
}



void  processLCDOffButtonPressed(){
  unsigned long currentMillis = millis();
  
  if((currentMillis - timeLastPressedLCDOffButton) < MIN_TIME_BETWEEN_BUTTON_PRESSES){
    return;
  }
  
  timeLastPressedLCDOffButton = currentMillis;
  
  Serial.println("LCD Off Button Press");
  
  if(currentState == ALARM){
    stopAlarm();
    return;
  }

  showLCD = !showLCD;
  turnOffLCD();

  
}

void processAlarmSetButtonPressed(){
  unsigned long currentMillis = millis();
  

  if((currentMillis - timeLastPressedAlarmSetButton) < MIN_TIME_BETWEEN_BUTTON_PRESSES){
    return;
  }
  
  timeLastPressedAlarmSetButton = currentMillis;
  
  Serial.println("Alarm Set Button Pressed");

  switch(currentState)
  {
    case ALARM: stopAlarm();
    break;
    case NORMAL :
    { 
      currentState = SETTING_ALARM;
      settingAlarmProcess = A_HOUR;
      Serial.println("Entering alarm setting mode");
    }
    break;
    case SETTING_ALARM:
    {
      if(settingAlarmProcess == A_HOUR){
         settingAlarmProcess = A_MINUTE;
      } else if(settingAlarmProcess == A_MINUTE){
         settingAlarmProcess = A_TYPE;
      } else {
        Serial.println("Exit alarm setting mode");
        currentState = NORMAL;
      }

    }
    break;
    case SETTING_TIME:
    break;
    default: break;
    }
  


}


void processTimeSetButtonPressed(){
  
  unsigned long currentMillis = millis();
  

  if((currentMillis - timeLastPressedTimeSetButton) < MIN_TIME_BETWEEN_BUTTON_PRESSES){
    return;
  }
   
  timeLastPressedTimeSetButton = currentMillis;
  
  Serial.println("Time Set Button Pressed");
  
  switch(currentState)
  {
    case ALARM: stopAlarm();
    break;
    case NORMAL :
   { 

   }
    break;
    case SETTING_ALARM:
    {
        if(settingAlarmProcess == A_HOUR){
          alarmHour = getNextHourFromCurrentHour(alarmHour);
        } else if(settingAlarmProcess == A_MINUTE){
          alarmMinute = getNextMinSecFromCurrentMinSec(alarmMinute);
        } else if(settingAlarmProcess == A_TYPE){
              
          if(!alarmVibrate && !alarmSound){
                //Both off, turn on sound
                alarmVibrate = false;
                alarmSound = true;
              } else if(!alarmVibrate && alarmSound){
                //Vibrate off, alarm on, turn on vibrate only
                alarmVibrate = true;
                alarmSound = false;
              } else if(alarmVibrate && !alarmSound){
              //Vibrate on, alarm off, turn on both
                alarmVibrate = true;
                alarmSound = true;  
              } else {
                //Both are on, turn both off
                alarmVibrate = false;
                alarmSound = false;  
              }
        }
    }
    break;
    case SETTING_TIME:
    break;
    default: break;
  }
  
  


}


void writeAlarmToDisplayBuffer(boolean blinkOn){
    
    char buff[3];

    String alarmHourString;
    
    if(currentState == SETTING_ALARM && settingAlarmProcess == A_HOUR && !blinkOn){
     //Now setting this, do not show 
     alarmHourString = "  ";
    } else {
      sprintf(buff, "%02d", alarmHour);
      alarmHourString = buff;
    }
    
    String alarmMinuteString;
    
    if(currentState == SETTING_ALARM && settingAlarmProcess == A_MINUTE && !blinkOn){
     //Now setting this, do not show 
     alarmMinuteString = "  ";
    } else {
      sprintf(buff, "%02d", alarmMinute);
      alarmMinuteString = buff;
    }

    String alarmTimeString;
    if(currentState == ALARM && !blinkOn){
      alarmTimeString = "      " + alarmHourString + ":" + alarmMinuteString;
    } else {
      alarmTimeString = "Alarm " + alarmHourString + ":" + alarmMinuteString;
    }
    
    
    
    display.setCursor(0,49);
    display.println(alarmTimeString);
    
    
    String alarmSetting;
    
    if(currentState == SETTING_ALARM && settingAlarmProcess == A_TYPE && !blinkOn){
     //Now setting this, do not show 
     alarmSetting = "";
    } else {
      if(alarmVibrate && alarmSound){
          alarmSetting = "(Sound + Vibrate)"; 
      } else if(alarmVibrate){
          alarmSetting = "(Vibrate)"; 
      } else if(alarmSound){
          alarmSetting = "(Sound)"; 
      } else {
        alarmSetting = "(Off)";
      }
    }
    
  
    display.println(alarmSetting);
    
    
}

void writeDateTimeToDisplayBuffer(DateTime now, boolean blinkOn){
  display.setTextSize(4);
  display.setCursor(0,0);
  String timeString = generateTimeString(now.hour(), now.minute(), now.second());
  display.print(timeString);
  char buff[3];

  display.setCursor(116,22);
  sprintf(buff, "%02d", now.second());
  display.setTextSize(1);
  display.println(buff);
  
  
  display.setCursor(0,34);
  display.setTextSize(1);
  String dateString = generateDateString(now.year(), now.month(), now.day());
  display.println(dateString);

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

boolean shouldBlinkNow(){
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousBlinkTime >  BLINK_INTERVAL) {
    previousBlinkTime = currentMillis;   
    previousBlinkState = !previousBlinkState;
  }
  return previousBlinkState;
}

int getNextHourFromCurrentHour(int current){
  return (current + 1) % 24;
}

int getNextMinSecFromCurrentMinSec(int current){
  return (current + 1) % 60;
}


int getNextYearFromCurrentYear(int current){
  current++;
  
  if(current > MAX_YEAR){
    current = MIN_YEAR;
  }
  
  return current;
}


void setup(){

  Serial.begin(9600);
  
 //Turn off TX and RX pin
  pinMode(RXLED_PIN, OUTPUT);
  digitalWrite(RXLED_PIN, LOW);
  TXLED1;
  
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
  
  char compiledDate[30];
  
  compiled.toString(compiledDate, 30);
  
  
  
  // initialize with the OLED with I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D); 
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(INITIAL_TEXT);
  
  display.println(compiledDate);
  
  
  display.display();
  
  delay(INITIAL_TEXT_DELAY);
  

  pinMode(BUTTON_ALARM_SET_PIN, INPUT); 
  pinMode(BUTTON_TIME_SET_PIN, INPUT); 
  pinMode(LCD_OFF_PIN, INPUT);
  
  
  pinMode(MOTOR_PIN1, OUTPUT); 
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(MOTOR_SLEEP_PIN, OUTPUT);
  
}






