/**************************************************************************************************************************************
FILENAME: swamp_cooler.ino
Group 21
AUTHOR: Samuel Lehman, Gordon Tan
DATE: 12/5/2021
DESCRIPTION: Code for a swamp cooler CPE 301
**************************************************************************************************************************************/
// for lcd1602
#include <LiquidCrystal.h>

// for motor
#include <Servo.h>

// for dth11
#include <DHT.h>

// for rDS1307 RTC (real time rtc)
#include <RTClib.h>

// prototypes
void displayHumidTemp();
void checkWaterLevel();
int getState();
void disabled_state();
void idle_state();
void error_state();
void running_state();
void clearLEDS();
void getButtonPushed();
unsigned int adc_read(unsigned char);
void setServoPos();
void fanSpeedController(int);

// definitions
#define DHT_IPIN A1
#define WATER_SENSOR_INPUT A0
#define WATER_LEVEL_THRESHOLD 10
#define TEMP_HIGH_THRESHOLD 0
#define DIS_EN_BTN_PIN A2
#define YELLOW_LED_PIN 9
#define GREEN_LED_PIN 8
#define RED_LED_PIN 7
#define BLUE_LED_PIN 6
#define INCREMENT_SERVO_ANGLE A5
#define DECREMENT_SERVO_ANGLE A6
#define SERVO_PIN A7
#define FAN_SPEED 255
#define FAN_PIN 13

// global variables
int servoPos = 0;
// 0 means disabled, 1 means enabled. by default system should be disabled
int dis_en_btn_state;
float h;
float t;
int waterLevel;
int state;
DateTime now;

// creates object lcd(pins)
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
DHT dht(DHT_IPIN, DHT11);
Servo myServo;
RTC_DS1307 rtc;

void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  dht.begin();
  myServo.attach(SERVO_PIN);
  dis_en_btn_state = 0;
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop()
{
  waterLevel = adcRead(WATER_SENSOR_INPUT);
  checkWaterLevel();
  int tempState;

  state = getState();
  if (dis_en_btn_state == 1)
  {
    clearLEDS();
    do
    {
      tempState = getState();
      Serial.print("\n");
      Serial.print("Current state: ");
      Serial.print(state);
      Serial.print("\n");
      Serial.print("Temp state: ");
      Serial.print(tempState);
      Serial.print("\n");
      Serial.print("Current btn state: ");
      Serial.print(dis_en_btn_state);
      Serial.print("\n");

      switch (state)
      {
      case 0:
        error_state();
        break;
      case 1:
        running_state();
        break;
      case 2:
        idle_state();
        break;
      default:
        break;
      }
      delay(2000);
    } while (dis_en_btn_state == 1 && tempState == state);
    clearLEDS();
  }
  else if (dis_en_btn_state == 0)
  {
    disabled_state();
    delay(2000);
    Serial.print("\n");
    Serial.print("Current state: ");
    Serial.print(state);
    Serial.print("\n");
    Serial.print("Temp state: ");
    Serial.print(tempState);
    Serial.print("\n");
    Serial.print("Current btn state: ");
    Serial.print(dis_en_btn_state);
    Serial.print("\n");
  }
}

void displayHumidTemp()
{
  h = dht.readHumidity();
  t = dht.readTemperature();
  dht.read(DHT_IPIN);
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.setCursor(0, 1);
  lcd.print("Humid: ");
  lcd.print(h);
  delay(500);
}

void checkWaterLevel()
{
  if (waterLevel < WATER_LEVEL_THRESHOLD)
  {
    Serial.print("\n");
    Serial.print("WARNING: Water level low");
    Serial.print("\t");
    Serial.print("Current level: ");
    Serial.print(waterLevel, DEC);
    Serial.print("\n");
  }
  delay(500);
}

int getState()
{
  waterLevel = analogRead(WATER_SENSOR_INPUT);
  t = dht.readTemperature();
  dht.read(DHT_IPIN);
  getButtonPushed();
  if (dis_en_btn_state == 0)
  {
    return 3;
  }
  else if (waterLevel > WATER_LEVEL_THRESHOLD && t > TEMP_HIGH_THRESHOLD)
  {
    // running
    return 1;
  }
  else if (waterLevel < WATER_LEVEL_THRESHOLD)
  {
    // error
    return 0;
  }
  else if (waterLevel > WATER_LEVEL_THRESHOLD && t < TEMP_HIGH_THRESHOLD)
  {
    // idle
    return 2;
  }
}

/* States */
void disabled_state()
{
  digitalWrite(YELLOW_LED_PIN, HIGH);
  fanSpeedController(0);
  getButtonPushed();
}

void idle_state()
{
  digitalWrite(GREEN_LED_PIN, HIGH);
  displayHumidTemp();
  setServoPos();
  fanSpeedController(0);
  getButtonPushed();
}

void error_state()
{
  digitalWrite(RED_LED_PIN, HIGH);
  displayHumidTemp();
  setServoPos();
  fanSpeedController(0);
  getButtonPushed();
}

void running_state()
{
  digitalWrite(BLUE_LED_PIN, HIGH);
  displayHumidTemp();
  setServoPos();
  fanSpeedController(1);
  getButtonPushed();
}

void clearLEDS()
{
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
}

void fanSpeedController(int enable)
{
  if (enable)
  {

    t = dht.readTemperature();
    if (t > TEMP_HIGH_THRESHOLD)
    {
      analogWrite(FAN_PIN, FAN_SPEED);
    }
    else if (t < TEMP_HIGH_THRESHOLD)
    {
      analogWrite(FAN_PIN, 0);
    }
  }
  else
  {
    analogWrite(FAN_PIN, 0);
  }
}

void getButtonPushed()
{
  btnPushed = digitalRead(DIS_EN_BTN_PIN);
  if (btnPushed == HIGH && dis_en_btn_state == 1)
  {
    dis_en_btn_state = 0;
    printTime();
    delay(1000);
  }
  else if (btnPushed == HIGH && dis_en_btn_state == 0)
  {
    dis_en_btn_state = 1;
    printTime();
    delay(1000);
  }
}

void setServoPos()
{
  int incBtn = analogRead(INCREMENT_SERVO_ANGLE);
  int decBtn = analogRead(DECREMENT_SERVO_ANGLE);
  if (incBtn == HIGH)
  {
    if (servoPos < 180)
      servoPos++;
  }
  if (decBtn == HIGH)
  {
    if (servoPos > 0)
      servoPos--;
  }
  myServo.write(servoPos);
}

void printTime()
{
  DateTime now = rtc.now();
  if (dis_en_btn_state == 1)
  {
    Serial.print("\n");
    Serial.print("Enabled: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  }
  else if (dis_en_btn_state == 0)
  {
    Serial.print("\n");
    Serial.print("Disabled: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  }
}
