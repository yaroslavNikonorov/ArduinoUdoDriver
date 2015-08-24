#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68
#include <Servo.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

//initialize lcd
LiquidCrystal lcd(12, 11, 7, 4, 3, 2);


//Display params
void (*printLCD)();
boolean displayUpdated=false;

//Button pins
#define DEBOUNCE 50
#define BTN_ONE A1
#define BTN_TWO A2
#define BTN_THREE A3
byte buttons[] = {BTN_ONE, BTN_TWO, BTN_THREE};
#define NUMBUTTONS sizeof(buttons)
boolean pressed[] = {false, false, false};
long pressedTime[] = {0,0,0};

//Led
byte ledPin=13;
int ledState=LOW;

//constants level one
#define CLOCK 0
#define MENU 1

//constants level two
#define CLOCK_SETTINGS 0
#define SERVO_SETTINGS 1
#define LIGHT_SETTINGS 2

//constants level three
#define DONOTEDIT 0
#define EDIT 1
#define SELECTED 2

//const
#define NOTHING 255

//Menu tree
byte menuTree[]={CLOCK,CLOCK_SETTINGS,NOTHING,NOTHING};
boolean menuSwitched=false;
//levels
#define LEVEL_ONE 0
#define LEVEL_TWO 1
#define LEVEL_THREE 2
#define LEVEL_FOUR 3
#define LEVEL_FIVE 4

//Main menu 
String mainMenu[]={"Clock settings", "Servo settings", "Light settings"};
#define NUMMAINMENU 3


//Clock menu
long timeUpdated=millis();
#define TIME_UPDATE_INTERVAL 500
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
#define SECOND 6
#define MINUTE 5
#define HOUR 4
#define DAYOFWEEK 0
#define DAYOFMONTH 1
#define MONTH 2
#define YEAR 3
byte clockEdit[]={0,0,0,0,0,0,0};
#define CLOCK_EDIT sizeof(clockEdit)
#define BLINK_INTERVAL 500
long lastTimeBlinked=millis();
String dateString="";
String timeString="";
boolean clockEditBlink=true;

//Light menu
String lightMenu[]={"Turn off", "Turn on"};
#define NUMLIGHTMENU 2
byte lightState=EEPROM.read(0);

//Servo settings
#define SERVO_ONE 0
#define SERVO_TWO 1
#define SERVO_THREE 2
#define SERVO_FOUR 3

#define SERVO_ONE_PIN 5
#define SERVO_TWO_PIN 6
#define SERVO_THREE_PIN 9
#define SERVO_FOUR_PIN 10

#define SERVO_ENABLE 1
#define SERVO_DISABLE 0

#define SERVO_ISENABLED 0
#define SERVO_DAY 1
#define SERVO_HOUR 2
#define SERVO_MINUTE 3
#define SERVO_ANGEL 4
#define SERVO_NUMBEROFTIMES 5

Servo servos[4];

String servoSettingsString[]={"E", "D", "H", "M", "Ang", "Num"};
byte editServoSetting=0;

#define SERVO_COUNT 4
#define SERVO_SETTINGS_COUNT 6


void setup()
{
  Wire.begin();
  //setDS3231time(0, 47, 23, 3, 11, 8, 15);
  
  digitalWrite(ledPin, lightState);
  lcd.begin(16, 2);
  //buttons
  for(byte i=0;i<NUMBUTTONS;i++){
    pinMode(buttons[i], INPUT);
  }
  printLCD=&showCurrentDateTime;
  menuSwitched=true;
  
  //Servo
  servos[SERVO_ONE].attach(SERVO_ONE_PIN);
  servos[SERVO_ONE].write(0);
  servos[SERVO_TWO].attach(SERVO_TWO_PIN);
  servos[SERVO_TWO].write(0);
  servos[SERVO_THREE].attach(SERVO_THREE_PIN);
  servos[SERVO_THREE].write(0);
  servos[SERVO_FOUR].attach(SERVO_FOUR_PIN);
  servos[SERVO_FOUR].write(0);

}

void buttonHandler(){
  for(byte i=0; i<NUMBUTTONS;i++){
    if(digitalRead(buttons[i]) == HIGH){
      if(millis()-pressedTime[i]>DEBOUNCE){
        pressed[i]=true;
      }
    }else{
      pressed[i]=false;
      pressedTime[i]=millis();
    }
  }
}


void menuHandler(){
  for(byte i=0;i<NUMBUTTONS;i++){
    if(pressed[i]){
      switch(i){
        case 0:
          btnOneHandler();
          break;
        case 1:
          btnTwoHandler();
          break;
        case 2:
          btnThreeHandler();
        break;
      }
      delay(200);
    }
  }
}

void btnOneHandler(){
  switch(menuTree[LEVEL_ONE]){
    case CLOCK:
      menuTree[LEVEL_ONE]=MENU;
      printLCD=&showMenu;
      break;
    case MENU:
      menuTree[LEVEL_ONE]=CLOCK;
      printLCD=&showCurrentDateTime;
      break;
  }
  menuTree[LEVEL_THREE]=NOTHING;
  menuTree[LEVEL_FOUR]=NOTHING;
  menuTree[LEVEL_FIVE]=NOTHING;
  menuSwitched=true;
}

void btnTwoHandler(){
  if(menuTree[LEVEL_ONE]==MENU){
    if(menuTree[LEVEL_THREE]==NOTHING){
      switchMenu();
    } else {
      runSettingHandler(BTN_TWO);
    }
  }
}

void btnThreeHandler(){
  if(menuTree[LEVEL_ONE]==MENU){
    if(menuTree[LEVEL_THREE]==NOTHING){
      menuTree[LEVEL_THREE]=DONOTEDIT;
      runSettingHandler(BTN_THREE);
    }else{
      runSettingHandler(BTN_THREE);
    }
  }
}

void runSettingHandler(byte btn){
  switch(menuTree[LEVEL_TWO]){
    case CLOCK_SETTINGS:
     clockSettingHandler(btn);
     break;
    case SERVO_SETTINGS:
     servoSettingHandler(btn);
     break;
    case LIGHT_SETTINGS:
     lightSettingHandler(btn);
     break; 
  }
}


void clockSettingHandler(byte btn){
    if(menuTree[LEVEL_THREE]==DONOTEDIT&&menuTree[LEVEL_FOUR]!=NOTHING&&btn==BTN_TWO){
      switch(menuTree[LEVEL_FOUR]){
        case DAYOFWEEK:
          menuTree[LEVEL_FOUR]=DAYOFMONTH;
          break;
        case DAYOFMONTH:
          menuTree[LEVEL_FOUR]=MONTH;
          break;
        case MONTH:
          menuTree[LEVEL_FOUR]=YEAR;
          break;
        case YEAR:
          menuTree[LEVEL_FOUR]=HOUR;
          break;
        case HOUR:
          menuTree[LEVEL_FOUR]=MINUTE;
          break;
        case MINUTE:
          menuTree[LEVEL_FOUR]=SECOND;
          break;
        case SECOND:
          menuTree[LEVEL_FOUR]=DAYOFWEEK;
          break;
     }
    }else if(menuTree[LEVEL_THREE]==DONOTEDIT&&menuTree[LEVEL_FOUR]==NOTHING&&btn==BTN_THREE){
      byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
      readDS3231time(&clockEdit[6], &clockEdit[5], &clockEdit[4], &clockEdit[0], &clockEdit[1], &clockEdit[2],&clockEdit[3]);
      //clockEdit={dayOfWeek,dayOfMonth,month,year,hour,minute,second};
      menuTree[LEVEL_FOUR]=DAYOFWEEK;
      printLCD=&showClockSettings;
    }else if(menuTree[LEVEL_THREE]==EDIT&&btn==BTN_TWO){
        editClockSettings();
    }else if(menuTree[LEVEL_THREE]==EDIT&&btn==BTN_THREE){
      saveClockSettings();
      menuTree[LEVEL_THREE]=DONOTEDIT;
      printLCD=&showClockSettings;
    }else if(menuTree[LEVEL_THREE]==DONOTEDIT&&btn==BTN_THREE){
      printLCD=&showEditClockSettings;
      menuSwitched=true;
      menuTree[LEVEL_THREE]=EDIT;
    }else{
      printLCD=&showClockSettings;
   }
}

void editClockSettings(){
  byte type=menuTree[LEVEL_FOUR];
  switch(type){
    case SECOND:
    case MINUTE:
      clockEdit[type]=++clockEdit[type]%60;
      break;
    case HOUR:
      clockEdit[type]=++clockEdit[type]%24;
      break;
    case DAYOFWEEK:
      clockEdit[type]=1+clockEdit[type]%7;
      break;
    case DAYOFMONTH:
      clockEdit[type]=1+clockEdit[type]%31;
      break;
    case MONTH:
      clockEdit[type]=1+clockEdit[type]%12;
      break;
    case YEAR:
      clockEdit[type]=1+clockEdit[type]%20;
      break;
  }
  menuSwitched=true;
}

void saveClockSettings(){
  setDS3231time(clockEdit[menuTree[LEVEL_FOUR]], menuTree[LEVEL_FOUR]);
}

void showEditClockSettings(){
  if(menuSwitched){
    String showString;
    if(menuTree[LEVEL_FOUR]==DAYOFWEEK){
      showString=getDayOfWeek(clockEdit[menuTree[LEVEL_FOUR]]);
    } else{
      showString=String(clockEdit[menuTree[LEVEL_FOUR]]);
    }
    lcd.clear();
    lcd.print(showString);
    menuSwitched=false;
  }
}

void showClockSettings(){
  if(millis()-lastTimeBlinked>BLINK_INTERVAL){
    String dateTime[7];

    for(byte i=0;i<CLOCK_EDIT;i++){
      if(i==DAYOFWEEK){
        dateTime[i]=getDayOfWeek(clockEdit[i]);
      } else {
        dateTime[i]=String(clockEdit[i]);
      }
    }
  
    if(clockEditBlink==true){
      dateTime[menuTree[LEVEL_FOUR]]=getEmptyString(dateTime[menuTree[LEVEL_FOUR]]);
      clockEditBlink=false;
    }else{
      clockEditBlink=true;
    }
    
    String date = dateTime[DAYOFWEEK]+" "+dateTime[DAYOFMONTH]+"/"+dateTime[MONTH]+"/"+dateTime[YEAR];
    lcd.setCursor(0,0);
    lcd.print(date);
    String time = dateTime[HOUR]+":"+dateTime[MINUTE]+":"+dateTime[SECOND];
    lcd.setCursor(0,1);
    lcd.print(time);
    lastTimeBlinked=millis();
  }
}


String getEmptyString(String str){
  String empty="";
  for(byte i=0;i<str.length();i++){
    empty+=" ";
  }
  return empty;
}

//Servo 
void servoSettingHandler(byte btn){
    if(menuTree[LEVEL_THREE]==DONOTEDIT&&menuTree[LEVEL_FOUR]!=NOTHING&&menuTree[LEVEL_FIVE]==NOTHING&&btn==BTN_TWO){
      menuTree[LEVEL_FOUR]=++menuTree[LEVEL_FOUR]%4;
      displayUpdated=true;
    }else if(menuTree[LEVEL_THREE]==DONOTEDIT&&menuTree[LEVEL_FOUR]==NOTHING&&menuTree[LEVEL_FIVE]==NOTHING&&btn==BTN_THREE){
      menuTree[LEVEL_FOUR]=SERVO_ONE;
      printLCD=&chooseServoShow;
      displayUpdated=true;
    }else if(menuTree[LEVEL_THREE]==DONOTEDIT&&menuTree[LEVEL_FOUR]!=NOTHING&&menuTree[LEVEL_FIVE]==NOTHING&&btn==BTN_THREE){
      menuTree[LEVEL_THREE]=SELECTED;
      menuTree[LEVEL_FIVE]=SERVO_ISENABLED;
      displayUpdated=true;
      printLCD=&servoSettingsShow;
    }else if(menuTree[LEVEL_THREE]==SELECTED&&menuTree[LEVEL_FOUR]!=NOTHING&&menuTree[LEVEL_FIVE]!=NOTHING&&btn==BTN_TWO){
      menuTree[LEVEL_FIVE]=++menuTree[LEVEL_FIVE]%SERVO_SETTINGS_COUNT;
      displayUpdated=true;
    }else if(menuTree[LEVEL_THREE]==SELECTED&&menuTree[LEVEL_FOUR]!=NOTHING&&menuTree[LEVEL_FIVE]!=NOTHING&&btn==BTN_THREE){
      menuTree[LEVEL_THREE]=EDIT;
      editServoSetting=getServoSetting(menuTree[LEVEL_FOUR], menuTree[LEVEL_FIVE]);
      printLCD=&servoSettingsEditShow;
      displayUpdated=true;
    }else if(menuTree[LEVEL_THREE]==EDIT&&menuTree[LEVEL_FOUR]!=NOTHING&&menuTree[LEVEL_FIVE]!=NOTHING&&btn==BTN_TWO){
      servoSettingsEdit();
      displayUpdated=true;
    }else if(menuTree[LEVEL_THREE]==EDIT&&menuTree[LEVEL_FOUR]!=NOTHING&&menuTree[LEVEL_FIVE]!=NOTHING&&btn==BTN_THREE){
      servoSettingsSave();
      menuTree[LEVEL_THREE]=SELECTED;
      displayUpdated=true;
      printLCD=&servoSettingsShow;
    }
}

void chooseServoShow(){
  if(displayUpdated){
    lcd.clear();
    lcd.print("Servo "+String(1+menuTree[LEVEL_FOUR]));
    displayUpdated=false;
  }
}

void servoSettingsShow(){
  String settingsString[6];
    if(millis()-lastTimeBlinked>BLINK_INTERVAL){
      String settingsString[6];

      for(byte i=0;i<SERVO_SETTINGS_COUNT;i++){
        settingsString[i]=servoSettingsString[i]+":"+String(getServoSetting(menuTree[LEVEL_FOUR], i));
      }
  
    if(clockEditBlink==true){
      settingsString[menuTree[LEVEL_FIVE]]=getEmptyString(settingsString[menuTree[LEVEL_FIVE]]);
      clockEditBlink=false;
    }else{
      clockEditBlink=true;
    }
    
    String firstString = settingsString[SERVO_ISENABLED]+","+settingsString[SERVO_DAY]+","+settingsString[SERVO_ANGEL];
    lcd.setCursor(0,0);
    lcd.print(firstString);
    String secondString = settingsString[SERVO_HOUR]+","+settingsString[SERVO_MINUTE]+","+settingsString[SERVO_NUMBEROFTIMES];
    lcd.setCursor(0,1);
    lcd.print(secondString);
    lastTimeBlinked=millis();
  }

}

void servoSettingsEditShow(){
  if(displayUpdated){
    lcd.clear();
    lcd.print(editServoSetting);
    displayUpdated=false;
  }
}

void servoSettingsEdit(){
  editServoSetting=++editServoSetting%60;
}

void servoSettingsSave(){
  setServoSetting(menuTree[LEVEL_FOUR], menuTree[LEVEL_FIVE], editServoSetting);
}

void lightSettingHandler(byte btn){
  if(menuTree[LEVEL_THREE]==EDIT&&btn==BTN_TWO){
    switch(lightState){
      case HIGH:
        lightState=LOW;
        break;
      case LOW:
        lightState=HIGH;
        break;
      } 
  }else if(menuTree[LEVEL_THREE]==EDIT&&btn==BTN_THREE){
    digitalWrite(ledPin, lightState);
    EEPROM.write(0, lightState);
  }else{
      menuTree[LEVEL_THREE]=EDIT;
  }
  printLCD=&showLightSettings;
  menuSwitched=true;
}

void showLightSettings(){
  if(menuSwitched==true){
    lcd.clear();
    lcd.print(lightMenu[lightState]);
    menuSwitched=false;
  }
}

void showCurrentDateTime(){
    String currentDateString=getDateString();
    String currentTimeString=getTimeString();
     
    if(menuSwitched==true||currentDateString!=dateString){
      dateString=currentDateString;
      lcd.setCursor(0,0);
      lcd.print(dateString);
    }
   
    if(menuSwitched==true||currentTimeString!=timeString){
      timeString=currentTimeString;
      lcd.setCursor(0,1);
      lcd.print(timeString);
    }
    menuSwitched=false;
}

void showMenu(){
  if(menuSwitched==true){
      lcd.clear();
      lcd.print(mainMenu[menuTree[LEVEL_TWO]]);
      menuSwitched=false;
  }
}

void switchMenu(){
  switch(menuTree[LEVEL_TWO]){
    case CLOCK_SETTINGS:
      menuTree[LEVEL_TWO]=SERVO_SETTINGS;
      break;
    case SERVO_SETTINGS:
      menuTree[LEVEL_TWO]=LIGHT_SETTINGS;
      break;
    case LIGHT_SETTINGS:
      menuTree[LEVEL_TWO]=CLOCK_SETTINGS;
      break;
    }
  showMenu();  
  menuSwitched=true;
}

byte getEEPROMAddress(byte servoNum, byte settingsNum){
  return 10+servoNum*SERVO_SETTINGS_COUNT+settingsNum;
}

byte getServoSetting(byte servoNum, byte settingsNum){
  return EEPROM.read(getEEPROMAddress(servoNum, settingsNum));
}

void setServoSetting(byte servoNum, byte settingsNum, byte setting){
  EEPROM.write(getEEPROMAddress(servoNum, settingsNum), setting);
}

void setDS3231time(byte dateTime, byte type)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  switch(type){
    case SECOND:
      Wire.write(0);
      Wire.write(decToBcd(dateTime)); // set seconds
      break;
    case MINUTE:
      Wire.write(1);
      Wire.write(decToBcd(dateTime)); // set minutes
      break;
    case HOUR:
      Wire.write(2);
      Wire.write(decToBcd(dateTime)); // set hours
      break;
    case DAYOFWEEK:
      Wire.write(3);
      Wire.write(decToBcd(dateTime)); // set day of week (1=Sunday, 7=Saturday)
      break;
    case DAYOFMONTH:
      Wire.write(4);
      Wire.write(decToBcd(dateTime)); // set date (1 to 31)
      break;
    case MONTH:
      Wire.write(5);
      Wire.write(decToBcd(dateTime)); // set month
      break;
    case YEAR:
      Wire.write(6);
      Wire.write(decToBcd(dateTime)); // set year (0 to 99)
      break;
  }
  Wire.endTransmission();
}

void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}

String getTimeString()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  String minn;
  if (minute<10)
  {
    minn = "0"+String(minute);
  } else {
    minn = String(minute);
  }
  
  String sec;
  if (second<10)
  {
    sec="0"+String(second);
  } else{
    sec = String(second);
  }
  return String(hour)+":"+minn+":"+sec;
}

String getDateString()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  return String(getDayOfWeek(dayOfWeek)) +" "+String(dayOfMonth)+"/"+String(month)+"/"+String(year);
}

String getDayOfWeek(byte dayOfWeek){
  String week="";
  switch(dayOfWeek){
    case 1:
      week="Sunday";
      break;
    case 2:
      week="Monday";
      break;
    case 3:
      week="Tuesday";
      break;
    case 4:
      week="Wednesday";
      break;
    case 5:
      week="Thursday";
      break;
    case 6:
      week="Friday";
      break;
    case 7:
      week="Saturday";
      break;
  }
  return week;
}

byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

void loop()
{
  buttonHandler();
  menuHandler();
  printLCD();
}
