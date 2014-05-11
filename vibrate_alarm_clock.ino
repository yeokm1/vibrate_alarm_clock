#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <RTC_DS1307.h>
#include "LowPower.h"

#include "constants.h"
#include "tune.h"

#define DO_NOT_SHOW_SERIAL_OUT //Comment this line if you want serial debugging for button press

#define MIN_BATTERY_MILLIVOLT 3300 //You may need to calibrate this
#define MAX_BATTERY_MILLIVOLT 4300 //You may need to calibrate this

#define DISPLAY_CONTRAST 64

#define INITIAL_TEXT "Happy 24th birthday\n Jason!\n\nBy: Yeo Kheng Meng\n(2014)"
#define INITIAL_TEXT_DELAY 8000
#define MIN_TIME_BETWEEN_BUTTON_PRESSES 100  //Debouncing purposes
#define BLINK_INTERVAL 100



#define MIN_TIME_BETWEEN_ALARM_STARTS 60000 //60 seconds
#define MIN_TIME_TO_CHANGE_MOTOR_DIRECTION 1500

#define MAX_ALARM_LENGTH 1200000 //Alarm rings for 20 minutes max

RTC_DS1307 RTC;
Adafruit_SSD1306 display(OLED_RESET_PIN);

typedef enum {NORMAL, ALARM, SETTING_TIME, SETTING_ALARM} CLOCK_STATE ;
CLOCK_STATE currentState = NORMAL;

typedef enum {T_HOUR, T_MINUTE, T_SECOND, T_DAY, T_MONTH, T_YEAR} SETTING_TIME_STATE ;
SETTING_TIME_STATE settingTimeProcess;

typedef enum {A_HOUR, A_MINUTE, A_TYPE} SETTING_ALARM_STATE ;
SETTING_ALARM_STATE settingAlarmProcess;

boolean alarmVibrate = false;
boolean alarmSound = false;

int alarmHour = 07;
int alarmMinute = 00;

unsigned long alarmLastStarted;

unsigned long lastVibration;
boolean lastMotorDirection;

unsigned long timeLastPressedLeftButton;
unsigned long timeLastPressedMiddleButton;
unsigned long timeLastPressedRightButton;


boolean previousBlinkState;
unsigned long previousBlinkTime;


unsigned long timePlayedPreviousTone;
int toneNow = 0;
int previousNoteDuration;
  
boolean showLCD = true;

void loop(){
    
  int leftButtonState = digitalRead(LEFT_BUTTON_PIN);
  int middleButtonState = digitalRead(MIDDLE_BUTTON_PIN);
  int rightPinState = digitalRead(RIGHT_BUTTON_PIN);

  if(leftButtonState == HIGH){
    processLeftButtonPressed();
  }
  
  if(middleButtonState == HIGH){
    processMiddleButtonPressed();
  }
  
  if(rightPinState == HIGH){
    processRightButtonPressed();
  }
 
  DateTime now = RTC.now();
  

  checkAndSoundAlarm(now.hour(), now.minute());
  soundAlarmAtThisPointIfNeeded();
  
  //Disable alarm if it has gone longer than maximum
  if(currentState == ALARM && (millis() - alarmLastStarted) > MAX_ALARM_LENGTH){
    stopAlarm();
  }
  
  if(showLCD || currentState == ALARM){
    
    display.clearDisplay();

    boolean blinkOn = shouldBlinkNow();
    
   
    int voltageADCReading = analogRead(VOLTAGE_MEASURE_PIN);
     //Multiply 2 as we are are using a half voltage divider
    int batteryMilliVolt = ((long) voltageADCReading * 2 * VOLTAGE_OF_VCC_MV) / ADC_PRECISION;

    writeDateTimeToDisplayBuffer(now, blinkOn);
    writeAlarmToDisplayBuffer(blinkOn);
    writeVoltageToDisplayBuffer(batteryMilliVolt);
    writeButtonStateToDisplayBuffer(blinkOn);
    
    
    display.display();
  }
  
  LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF);
  
}

void processLeftButtonPressed(){
  unsigned long currentMillis = millis();
  
  if((currentMillis - timeLastPressedLeftButton) < MIN_TIME_BETWEEN_BUTTON_PRESSES){
    return;
  }
  
  timeLastPressedLeftButton = currentMillis;
  

 #ifndef DO_NOT_SHOW_SERIAL_OUT
   Serial.println("Left Button Press");
 #endif
 
 
  switch(currentState)
  {
  case ALARM: stopAlarm();
  break;
  case NORMAL :
  {
    showLCD = !showLCD;
    turnOffLCD();
    
  }
  break;
  case SETTING_ALARM:
  {
   if(settingAlarmProcess == A_HOUR){
       alarmHour = getNextHourFromCurrentHour(alarmHour, false);
    } else if(settingAlarmProcess == A_MINUTE){
       alarmMinute = getNextMinSecFromCurrentMinSec(alarmMinute, false);
    } else if(settingAlarmProcess == A_TYPE){
              
       if(!alarmVibrate && !alarmSound){
            //Both off, turn on sound
          alarmVibrate = true;
          alarmSound = true;
       } else if(!alarmVibrate && alarmSound){
          //Vibrate off, alarm on, turn on vibrate only
          alarmVibrate = false;
          alarmSound = false;
       } else if(alarmVibrate && !alarmSound){
         //Vibrate on, alarm off, turn on both
         alarmVibrate = false;
         alarmSound = true;  
       } else {
          //Both are on, turn both off
          alarmVibrate = true;
          alarmSound = false;  
      }
   }
  
  }
  break;
  case SETTING_TIME:
  {
    DateTime now = RTC.now();
    DateTime newTime;
    switch(settingTimeProcess){
      case T_HOUR:
        newTime = DateTime(now.year(), now.month(), now.day(), getNextHourFromCurrentHour(now.hour(), false), now.minute(), now.second());
        break;
      case T_MINUTE:
        newTime = DateTime(now.year(), now.month(), now.day(), now.hour(), getNextMinSecFromCurrentMinSec(now.minute(), false), now.second());
        break;
      case T_SECOND:
        newTime = DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), getNextMinSecFromCurrentMinSec(now.second(), false));
        break;
      case T_DAY:
        newTime = DateTime(now.year(), now.month(), now.day() - 1, now.hour(), now.minute(), now.second());
        break;
      case T_MONTH:
      {
        int month = now.month();
        if(month == 1){
          month = 12;
        } else {
          month--;
        }
      
        newTime = DateTime(now.year(), month, now.day(), now.hour(), now.minute(), now.second());
      }
        break;
      case T_YEAR:
        newTime = DateTime(now.year() - 1, now.month(), now.day(), now.hour(), now.minute(), now.second());
        break;
      default: break;
     
    }
       
    RTC.adjust(newTime);
  }
  break;
  default: break;
  }
  
  
}

void processMiddleButtonPressed(){
  unsigned long currentMillis = millis();
  

  if((currentMillis - timeLastPressedMiddleButton) < MIN_TIME_BETWEEN_BUTTON_PRESSES){
    return;
  }
  
  timeLastPressedMiddleButton = currentMillis;

 #ifndef DO_NOT_SHOW_SERIAL_OUT
   Serial.println("Middle Button Pressed");
 #endif
   
  switch(currentState)
  {
    case ALARM: stopAlarm();
    break;
    case NORMAL :
    {
      if(showLCD){
        currentState = SETTING_ALARM;
        settingAlarmProcess = A_HOUR;
      } else {
        showLCD = true;
      }

    }
    break;
    case SETTING_ALARM:
    {
      if(settingAlarmProcess == A_HOUR){
         settingAlarmProcess = A_MINUTE;
      } else if(settingAlarmProcess == A_MINUTE){
         settingAlarmProcess = A_TYPE;
      } else {
        currentState = NORMAL;
      }

    }
    break;
    case SETTING_TIME:
    {
      if(settingTimeProcess == T_HOUR){
         settingTimeProcess = T_MINUTE;
      } else if(settingTimeProcess == T_MINUTE){
         settingTimeProcess = T_SECOND;
      } else if(settingTimeProcess == T_SECOND){
         settingTimeProcess = T_DAY;      
      } else if(settingTimeProcess == T_DAY){
        settingTimeProcess = T_MONTH; 
      } else if(settingTimeProcess == T_MONTH){
         settingTimeProcess = T_YEAR;           
      } else {
        currentState = NORMAL;
      }
      
    }
    break;
    default: break;
    }
  


}


void processRightButtonPressed(){
  
  unsigned long currentMillis = millis();
  

  if((currentMillis - timeLastPressedRightButton) < MIN_TIME_BETWEEN_BUTTON_PRESSES){
    return;
  }
   
  timeLastPressedRightButton = currentMillis;

  #ifndef DO_NOT_SHOW_SERIAL_OUT
   Serial.println("Right Set Button Pressed");
  #endif
   
  switch(currentState)
  {
    case ALARM: stopAlarm();
    break;
    case NORMAL :
   { 
      if(showLCD){
        currentState = SETTING_TIME;
        settingTimeProcess = T_HOUR;
      } else {
        showLCD = true;
      }
      
   }
    break;
    case SETTING_ALARM:
    {
        if(settingAlarmProcess == A_HOUR){
          alarmHour = getNextHourFromCurrentHour(alarmHour, true);
        } else if(settingAlarmProcess == A_MINUTE){
          alarmMinute = getNextMinSecFromCurrentMinSec(alarmMinute, true);
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
      {
    DateTime now = RTC.now();
    DateTime newTime;
    switch(settingTimeProcess){
      case T_HOUR:
        newTime = DateTime(now.year(), now.month(), now.day(), getNextHourFromCurrentHour(now.hour(), true), now.minute(), now.second());
        break;
      case T_MINUTE:
        newTime = DateTime(now.year(), now.month(), now.day(), now.hour(), getNextMinSecFromCurrentMinSec(now.minute(), true), now.second());
        break;
      case T_SECOND:
        newTime = DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), getNextMinSecFromCurrentMinSec(now.second(), true));
        break;
      case T_DAY:
        newTime = DateTime(now.year(), now.month(), now.day() + 1, now.hour(), now.minute(), now.second());
        break;
      case T_MONTH:
      {
        int month = now.month();
        if(month == 12){
          month = 1;
        } else {
          month++;
        }
        newTime = DateTime(now.year(), month, now.day(), now.hour(), now.minute(), now.second());
      }
        break;
      case T_YEAR:
        newTime = DateTime(now.year() + 1, now.month(), now.day(), now.hour(), now.minute(), now.second());
        break;
      default: break;
     
    }
       
    RTC.adjust(newTime);
  }
    
    
    break;
    default: break;
  }

}


void writeButtonStateToDisplayBuffer(boolean blinkOn){
  
  String buttonFunction;
  switch(currentState)
  {
    case ALARM: 
    {
      if(!blinkOn){
        buttonFunction = "Stop     Stop    Stop";
      }
    
    }
    break;
    case NORMAL : buttonFunction =  "OLED    Alarm    Time";
    break;
    case SETTING_ALARM: 
    //Fallthrough
    case SETTING_TIME: buttonFunction = "--       Next      ++";
    break;
    default: break;
  }
  
  display.setCursor(0,57);
  display.print(buttonFunction);
 
}


void writeVoltageToDisplayBuffer(int batteryMilliVolt){
  
  display.setTextSize(1);

//  float voltage = (float) batteryMilliVolt / 1000;
  
//  char buff[5];
//  String voltageString = dtostrf(voltage, 3, 1, buff);
// 
//  display.setCursor(104,49);
//  display.print(voltageString);
//  display.print("V");
  
  int batteryRange = MAX_BATTERY_MILLIVOLT - MIN_BATTERY_MILLIVOLT;

  //Cast to long as Arduino int is only 16 bits wide. Not enough to hold this result
  long numerator =  ((long) ((batteryMilliVolt - MIN_BATTERY_MILLIVOLT))) * 100;
  int batteryPercent = numerator / batteryRange;

  if(batteryPercent < 100){
     display.setCursor(110,40);
  } else {
     display.setCursor(104,40);
  }
  display.print(batteryPercent);
  display.print("%");


}

void checkAndSoundAlarm(int hour, int minute){
  
  unsigned long currentMillis = millis();
  
  if(hour == alarmHour && minute == alarmMinute && currentState == NORMAL
  && ((currentMillis - alarmLastStarted) > MIN_TIME_BETWEEN_ALARM_STARTS)
  && (alarmVibrate || alarmSound)){
    currentState = ALARM;
    alarmLastStarted = currentMillis;
  }

}


void soundAlarmAtThisPointIfNeeded(){

  if(currentState == ALARM){
    
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
    String alarmSetting;
       
    if(currentState == SETTING_ALARM && settingAlarmProcess == A_TYPE && !blinkOn){
     //Now setting this, do not show 
     alarmSetting = "";
    } else {
     if(alarmVibrate && alarmSound){
          alarmSetting = " Ring+Vib"; 
      } else if(alarmVibrate){
          alarmSetting = " Vibrate"; 
      } else if(alarmSound){
          alarmSetting = " Ring"; 
      } else {
        alarmSetting = " Alarm Off";
      }
     }

      alarmTimeString = alarmHourString + ":" + alarmMinuteString + alarmSetting;
    
    
    
    
    display.setCursor(0,47);
    display.println(alarmTimeString);

}

void writeDateTimeToDisplayBuffer(DateTime now, boolean blinkOn){
  display.setTextSize(4);
  display.setCursor(0,0);
  String timeString = generateTimeString(now.hour(), now.minute(), now.second(), blinkOn);
  display.print(timeString);
  
  char buff[3];


  if(!(currentState == SETTING_TIME && settingTimeProcess == T_SECOND && !blinkOn)){
    display.setCursor(116,22);
    sprintf(buff, "%02d", now.second());
    display.setTextSize(1);
    display.println(buff);
  }
  

  display.setCursor(0,34);
  display.setTextSize(1);
  String dateString = generateDateString(now.year(), now.month(), now.day(), blinkOn);
  display.println(dateString);

}



String generateTimeString(int hour, int minute, int second, boolean blinkOn){
  char buff[3];
  
  String hourString;
  
  if(currentState == SETTING_TIME && settingTimeProcess == T_HOUR && !blinkOn){
    hourString = "  ";
  } else {
    sprintf(buff, "%02d", hour);
    hourString = buff;
  }

  String minuteString;
  if(currentState == SETTING_TIME && settingTimeProcess == T_MINUTE && !blinkOn){
    minuteString = "  ";
  } else {
    sprintf(buff, "%02d", minute);
    minuteString = buff;
  }


  String result = hourString + ":" + minuteString;

  return result;
}

String generateDateString(int year, int month, int day, boolean blinkOn)
{
  String dayOfWeek = getDayOfTheWeek(year, month, day);
  String dayString;
  String monthString;
  String yearString;
  
  
  if(currentState == SETTING_TIME && settingTimeProcess == T_DAY && !blinkOn){
    dayString = "  ";
  } else {
    char buff[3];
    sprintf(buff, "%02d", day);
    dayString = buff;
  }
  
  if(currentState == SETTING_TIME && settingTimeProcess == T_MONTH && !blinkOn){
    monthString = "   ";
  } else {
    monthString = getMonth(month);
  }
  
  if(currentState == SETTING_TIME && settingTimeProcess == T_YEAR && !blinkOn){
    yearString = "    ";
  } else {
    yearString = String(year);
  }
  
  
 
  String result = dayOfWeek + ", " + dayString + " " + monthString + " " + yearString;
  
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

int getNextHourFromCurrentHour(int current, boolean increment){
  int change;
  if(increment){
    change = 1;
  } else {
    change = -1;
  }
  
  int newValue = current + change;
  
  //To produce positive modulo result
  return (newValue % 24 + 24) % 24;
}

int getNextMinSecFromCurrentMinSec(int current, boolean increment){
    int change;
  if(increment){
    change = 1;
  } else {
    change = -1;
  }
  
  int newValue = current + change;
  
  //To produce positive modulo result
  return (newValue % 60 + 60) % 60;
}

void setup(){

  #ifndef DO_NOT_SHOW_SERIAL_OUT
    Serial.begin(9600);
  #endif
  
 //Turn off TX and RX pin
  pinMode(RXLED_PIN, OUTPUT);
  digitalWrite(RXLED_PIN, LOW);
  TXLED1;
  
  Wire.begin();
  RTC.begin();
  

  // This section grabs the current datetime and compares it to
  // the compilation time.  If necessary, the RTC is updated.
  DateTime now = RTC.now();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  
  if (now.unixtime() < compiled.unixtime()) {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  } 
  
  
  // initialize with the OLED with I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D); 
  
  display.dim(DISPLAY_CONTRAST);
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(INITIAL_TEXT);
 
  
  display.display();
  
  
  delay(INITIAL_TEXT_DELAY);
  

  pinMode(LEFT_BUTTON_PIN, INPUT); 
  pinMode(MIDDLE_BUTTON_PIN, INPUT); 
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  pinMode(VOLTAGE_MEASURE_PIN, INPUT);
  
  pinMode(MOTOR_PIN1, OUTPUT); 
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(MOTOR_SLEEP_PIN, OUTPUT);
  
}






