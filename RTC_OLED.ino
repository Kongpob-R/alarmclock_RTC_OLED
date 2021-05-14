#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#define OLED_MOSI  P1_6
#define OLED_CLK   P1_5
#define OLED_DC    P3_7
#define OLED_CS    P5_1
#define OLED_RESET P3_5
Adafruit_SSD1306 OLED(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#include "interrupt.h"
#define DS3231_READ  0xD1
#define DS3231_WRITE 0xD0
#define DS3231_ADDR  0x68

//DS3231 Registers
#define DS3231_SECONDS  0x00
#define DS3231_MINUTES  0x01
#define DS3231_HOURS  0x02
#define DS3231_DAY 0x03
#define DS3231_DATE 0x04
#define DS3231_CEN_MONTH 0x05
#define DS3231_DEC_YEAR 0x06
#define DS3231_ALARM1_SECONDS 0x07
#define DS3231_ALARM1_MINUTES 0x08
#define DS3231_ALARM1_HOURS 0x09
#define DS3231_ALARM1_DAY_DATE 0x0a
#define DS3231_ALARM2_MINUTES 0x0b
#define DS3231_ALARM2_HOURS 0x0c
#define DS3231_ALARM2_DAY_DATE 0x0d
#define DS3231_CONTROL 0x0e
#define DS3231_CTL_STATUS 0x0f
#define DS3231_AGING_OFFSET 0x10
#define DS3231_TEMP_MSB 0x11
#define DS3231_TEMP_LSB 0x12
int Day; 
int Month;
int Year; 
int Secs;
int Minutes;
int Hours;
int aHours;
int aMinutes;
String myDate; 
String myTime;
int daysInMonths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
int tempY1, tempY2;
int mode = 0;
int syncCounter = 0;
int alarmRGB = 0;
bool alarmState = false;
bool alarmMode = false;
int deBounce = 0;
int button1Flag, button2Flag = 0;
const byte interruptPin_S1 = P1_1;
const byte interruptPin_S2 = P1_4;

#include <Wire.h>

void button1Handler(){
  button1Flag = 1;
}

void button2Handler(){
  button2Flag = 1;
}

uint8_t _toBcd(uint8_t num)
{
  uint8_t bcd = ((num / 10) << 4) + (num % 10);
  return bcd;
}

uint8_t _fromBcd(uint8_t bcd) {
  uint8_t num = (10*((bcd&0xf0) >>4)) + (bcd & 0x0f);
  return num;
}
uint8_t readTimeRegister(uint8_t reg){
  Wire.beginTransmission(DS3231_ADDR);  
  Wire.write(reg);      
  Wire.endTransmission();       
  Wire.requestFrom(DS3231_ADDR,1);    
  return _fromBcd(Wire.read());       
}

void writeTimeRegister(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(DS3231_ADDR);  
  Wire.write(reg);                
  Wire.write(_toBcd(data));             
  Wire.endTransmission();           
}

void OLEDshow(){
  OLED.clearDisplay(); 
  OLED.setTextColor(WHITE);   //Text is white ,background is black
  OLED.setTextSize(2);
  OLED.setCursor(0,0);
  String text = "";
  String text2 = "";
  switch(mode){
      case 1:
        text = text + "Hours: ";
        if(Hours < 10){
          text2 = text2 + "0";
        }
        text2 = text2 + Hours;
        break;
      case 2:
        text = text + "Minutes: ";
        if(Minutes < 10){
          text2 = text2 + "0";
        }
        text2 = text2 + Minutes;
        break;
      case 3:
        text = text + "Year 20X0: ";
        text2 = text2 + (Year/10);
        break;
      case 4:
        text = text + "Year 200X: ";
        text2 = text2 + (Year%10);
        break;
      case 5:
        text = text + "Month: ";
        text2 = text2 + Month;
        break;
      case 6:
        text = text + "Date: ";
        text2 = text2 + Day;
        break;
      case 7:
        text = text + "A_Hour: ";
        text2 = text2 + aHours;
        break;
      case 8:
        text = text + "A_Minutes: ";
        text2 = text2 + aMinutes;
        break;
      case 9:
        text = text + "Alarm Mode: ";
        if(alarmMode){
          text2 = text2 + "ON";
        }else{
          text2 = text2 + "OFF";
        }
        break;
      default:
        myDate = myDate + " "+ Day + "/" + Month + "/" + Year ; 
        if(Hours < 10){
          myTime = myTime + "0";
        }
        myTime = myTime + Hours +":" ;
        if(Minutes < 10){
          myTime = myTime + "0";
        }
        myTime = myTime + Minutes +":" ;
        if(Secs < 10){
          myTime = myTime + "0";
        }
        myTime = myTime + Secs ;
        text = myTime;
        text2 = myDate;
        myDate = "";   
        myTime = ""; 
        break;
    }
    Serial.print(text);
    Serial.print(" --- ");
    Serial.println(text2);
    OLED.println(text);
    OLED.setCursor(0,25);
    OLED.println(text2);
    OLED.display(); 
}  


void modeCounter() {
  if (alarmState && alarmMode){
    alarmState = false;
  }else{
    mode++;
    if (mode > 9){
      mode = 0;
    }
  }
  Serial.print("modeCounter: ");
  Serial.println(mode);
  button1Flag = 0;
}

void incrementRegister() {
  Serial.println("incrementRegister");
  if (alarmState && alarmMode){
    alarmState = false;
  }else{
    int extraFebDay = 0;
    switch(mode){
      case 1:
        Hours++;
        if(Hours > 23){
          Hours = 0;
        }
        writeTimeRegister(DS3231_HOURS,Hours);
        break;
      case 2:
        Minutes++;
        if(Minutes > 59){
          Minutes = 0;
        }
        writeTimeRegister(DS3231_MINUTES,Minutes);
        break;
      case 3:
        tempY1 = (Year)/10;
        tempY2 = (Year)%10;
        tempY1++;
        if(tempY1 > 9){
          tempY1 = 0;
        }
        writeTimeRegister(DS3231_DEC_YEAR, (tempY1*10)+tempY2);
        break;
      case 4:
        tempY1 = (Year)/10;
        tempY2 = (Year)%10;
        tempY2++;
        if(tempY2 > 9){
          tempY2 = 0;
        }
        writeTimeRegister(DS3231_DEC_YEAR, (tempY1*10)+tempY2);
        break;
      case 5:
        Month++;
        if(Month > 12){
          Month = 1;
        }
        writeTimeRegister(DS3231_CEN_MONTH, Month);
        break;
      case 6:
        Day++;
        if(Month == 2 && (Year%4 == 0)){
          extraFebDay = 1;
        }
        if(Day > (daysInMonths[Month-1]+extraFebDay)){
          Day = 1;
        }
        writeTimeRegister(DS3231_DATE, Day);
        break;
      case 7:
        aHours++;
        if(aHours > 23){
          aHours = 0;
        }
        writeTimeRegister(DS3231_ALARM1_HOURS, aHours);
        break;
      case 8:
        aMinutes++;
        if(aMinutes > 59){
          aMinutes = 0;
        }
        writeTimeRegister(DS3231_ALARM1_MINUTES, aMinutes);
        break;
      case 9:
        alarmMode = !alarmMode;
        break;
    }
  syncTime();
  button2Flag = 0;
  }
}

void syncTime() {
  Day = readTimeRegister(DS3231_DATE); 
  Month = readTimeRegister(DS3231_CEN_MONTH); 
  Year = readTimeRegister(DS3231_DEC_YEAR);
  Secs = readTimeRegister(DS3231_SECONDS); 
  Hours = readTimeRegister(DS3231_HOURS); 
  Minutes = readTimeRegister(DS3231_MINUTES);
  aHours = readTimeRegister(DS3231_ALARM1_HOURS); 
  aMinutes = readTimeRegister(DS3231_ALARM1_MINUTES);
}

void RGBalarm() {
  if(alarmState && alarmMode){
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    alarmRGB++;
    if (alarmRGB > 2){
      alarmRGB = 0;
    }
    switch(alarmRGB){
      case 0:
        digitalWrite(RED_LED, HIGH);
        break;
      case 1:
        digitalWrite(BLUE_LED, HIGH);
        break;
      case 2:
        digitalWrite(GREEN_LED, HIGH);
        break;
    }
  }else{
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
  }
}

void setup() {
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);   
  pinMode(RED_LED, OUTPUT);  
  pinMode(interruptPin_S1, INPUT_PULLUP);
  pinMode(interruptPin_S2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin_S1), button1Handler, RISING);
  attachInterrupt(digitalPinToInterrupt(interruptPin_S2), button2Handler, RISING);
  // put your setup code here, to run once:
  Serial.begin(115200);       //Init serial port
  while(!Serial){;}         //Wait for serial port connection
  Serial.println("Hello");  //Show some sign of life
  //Initialize the I2C bus using Arduino Wire library
  Wire.begin();
  OLED.begin(SSD1306_SWITCHCAPVCC,0xAA);
  //Let's set the time by writing to the registers, any values you like
  writeTimeRegister(DS3231_HOURS,12);
  writeTimeRegister(DS3231_MINUTES,30);
  writeTimeRegister(DS3231_SECONDS,0);
  writeTimeRegister(DS3231_CEN_MONTH,3);
  writeTimeRegister(DS3231_DEC_YEAR,19);
}

void loop() {
  if(mode == 0 && syncCounter > 4){
    syncTime();
    syncCounter = 0;
  }
  syncCounter++;  
  if(aHours == Hours && aMinutes == Minutes && Secs == 0 &&alarmMode == true){
    alarmState = 1;
  }
  if((button1Flag == 1) && (deBounce > 3)){
    deBounce = 0;
    modeCounter();
  }
  if((button2Flag == 1) && (deBounce > 3)){
    deBounce = 0;
    incrementRegister();
  }
  OLEDshow();
  RGBalarm();
  delay(50);
  if(deBounce < 10){
    deBounce++;
  }
}
