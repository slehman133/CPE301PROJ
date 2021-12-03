/**************************************************************************************************************************************
FILENAME: swamp_cooler.ino
AUTHOR: Samuel Lehman, Gordan Tan
DATE: 12/5/2021
DESCRIPTION: code for a swamp cooler

Project should:
Monitor the water levels in a reservoir and print an alert when the level is too low
Monitor and display the current air temp and humidity on an LCD screen
Start and stop a fan motor as needed when the temperature falls out of a specified range (high or low)
Allow a user to use a control to adjust the angle of an output vent from the system
Allow a user to enable or disable the system using an on/off button
Record the time and date every time the motor is turned on or off. This information should be transmitted to a host computer (over USB)

Cooler states:
All states except DISABLED

Humidity and temperature should be continuously monitored and reported on the LDC screen
The system should respond to changes in vent position control
Stop button should turn off motor (if on) and system should go to DISABLED state
DISABLED

YELLOW LED should be lit
No monitoring of temperature or water should be performed
The start button should be monitored
IDLE

The system should monitor temperature and transition to running state when temperature > threshold (you determine the threshold)
The exact time (using real-time rtc) should record transition times
The water level should be continuously monitored and the state changed to error if the level is too low
GREEN LED should be lit
ERROR

RED LED should be turned on (all other LEDs turned off)
The motor should be off and not start regardless of temperature
The system should transition to IDLE as soon as the water is at an acceptable level
An error message should be displayed on LCD
RUNNING

BLUE LED should be turned on (all other LEDs turned off)
The motor should be on
The system should transition to IDLE as soon as the temperature drops below the lower threshold
The system should transition to the ERROR state if water becomes too low
**************************************************************************************************************************************/
// for lcd1602
#include <LiquidCrystal.h>

// for motor
#include <Servo.h>

// for dth11
//#include <Adafruit_10DOF.h>
#include <DHT.h>

// for rDS1307 RTC (real time rtc)
#include <RTC.h>

// prototypes
void displayHumidTemp();
void checkWaterLevel(int);
int getState();
void disabled_state();
void idle_state();
void error_state();
void running_state();
void adc_init();
void clearLEDS();
bool getDisabledState();
unsigned int adc_read(unsigned char);
void setServoPos();

// TODO set all pins
// definitions
#define DHT_IPIN A1
#define WATER_SENSOR_INPUT A0
#define WATER_LEVEL_THRESHOLD 50
#define TEMP_HIGH_THRESHOLD 80
#define DIS_EN_BTN_PIN A2
#define YELLOW_LED_PIN 9
#define GREEN_LED_PIN 8
#define RED_LED_PIN 7
#define BLUE_LED_PIN 6
#define INCREMENT_SERVO_ANGLE
#define DECREMENT_SERVO_ANGLE
#define SERVO_PIN

// global variables
int servoPos = 0;

// 0 means disabled, 1 means enabled
int dis_en_btn_state = 1;

// TODO set pin values for lcd
// creates object lcd(pins)
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
DHT dht(DHT_IPIN, DHT11);
Servo myServo(SERVO_PIN);
DS1307 rtc;
RTCDateTime dt;

// DIGITAL PORT B REGISTERS
volatile unsigned char *portB = (unsigned char *)0x25;
volatile unsigned char *portDDRB = (unsigned char *)0x24;
volatile unsigned char *pinB = (unsigned char *)0x23;

// ANALOG
volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADCH_DATA = (unsigned int *)0x79;
volatile unsigned int *my_ADCL_DATA = (unsigned int *)0x78;

void setup()
{
  Serial.begin(9600);
  adc_init();
  lcd.begin(16, 2);
  dht.begin();
  *portB &= 0b01111111;
  *portDDRB &= 0b01111111;
  *portDDRB |= 0b01111110;
  rtc.begin();
  clock.setTime(__DATE__, __TIME__);
}

void loop()
{
  displayHumidTemp();
  // digitalWrite(GREEN_LED_PIN, HIGH);
  // digitalWrite(RED_LED_PIN, HIGH);
  // digitalWrite(BLUE_LED_PIN, HIGH);
  // digitalWrite(YELLOW_LED_PIN, HIGH);
  delay(500);
  int waterLevel = analogRead(WATER_SENSOR_INPUT);
  checkWaterLevel(waterLevel);
  int tempState;

  bool isRunning = false;
  bool isIdle = false;
  bool isError = false;

  int state = getState();
  bool isDisabled = getDisabledState();
  if (!isDisabled)
  {
    clearLEDS();
    do
    {
      tempState = getState();
      switch (state)
      {
      // error
      case 0:
        isError = true;
        error_state();
        break;
      // running
      case 1:
        isRunning = true;
        running_state();
        break;
      // idle
      case 2:
        isIdle = true;
        idle_state();
        break;
      case 3:
        disabled_state();
        isDisabled = true;
        break;
      }
      delay(500);
    } while (state != tempState);
    clearLEDS();
  }
  else
  {
    disabled_state();
    state = getState();
    delay(500);
  }
}
}

void displayHumidTemp()
{
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  dht.read(DHT_IPIN);
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.setCursor(0, 1);
  lcd.print("Humid: ");
  lcd.print(h);
  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print("Humid: ");
  Serial.print(h);
  delay(500);
}

void checkWaterLevel(int waterLevel)
{
  if (waterLevel < 20)
  {
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
  int waterLevel = analogRead(WATER_SENSOR_INPUT);
  float t = dht.readTemperature();
  dht.read(DHT_IPIN);

  /*if (waterLevel < WATER_LEVEL_THRESHOLD)
  {
    // error
    return 0;
  }
  else if (t > TEMP_HIGH_THRESHOLD)
  {
    // running
    return 1;
  }*/
  if (waterLevel > WATER_LEVEL_THRESHOLD && t < TEMP_HIGH_THRESHOLD)
  {
    // idle
    return 2;
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
  else if (getDisabledState)
  {
    return 3;
  }
}

/* States */
void disabled_state()
{
  // TODO add code to send date from realtime rtc to host computer
  //  printTime();
  digitalWrite(YELLOW_LED_PIN, HIGH);
}

void idle_state()
{
  displayHumidTemp();
  setSevoPosition();
  digitalWrite(GREEN_LED_PIN, HIGH);
}

void error_state()
{
  displayHumidTemp();
  setSevoPosition();
  digitalWrite(RED_LED_PIN, HIGH);
}

void running_state()
{
  displayHumidTemp();
  setSevoPosition();
  digitalWrite(BLUE_LED_PIN, HIGH);
}

/* Analog/Digital Conversion */
void adc_init()
{
  // Register A
  *my_ADCSRA |= 0x80;       // Set Bit 7 to 1
  *my_ADCSRA &= 0b11011111; // Clear Bit 5
  *my_ADCSRA &= 0b11110111; // Clear Bit 3
  *my_ADCSRA &= 0b11111000; // Clear bits 2-0

  // Register B
  *my_ADCSRB &= 0b11110111; // Clear bit 3
  *my_ADCSRB &= 0b11111000; // Clear bit 2-0

  // MUX
  *my_ADMUX &= 0b01111111; // Clear bit 7
  *my_ADMUX |= 0b01000000; // Set bit 6
  *my_ADMUX &= 0b11011111; // Clear bit 5. Right adjusted result.
  *my_ADMUX &= 0b11100000; // Clear Bits 4-0
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  // Clear Analog channel selection bits
  *my_ADMUX &= 0b11100000;
  *my_ADMUX &= 0b11011111;
  // Set channel number
  if (adc_channel_num > 7)
  {
    adc_channel_num -= 8;
    *my_ADCSRB |= 0b00001000;
  }
  // Set channel selection bits
  *my_ADMUX += adc_channel_num;
  *my_ADCSRA |= 0b01000000;
  // Wait for conversion to complete and return result
  while ((*my_ADCSRA & 0x40) != 0)
    ;
  return pow(2 * (*my_ADCH_DATA & (1 << 0)), 8) + pow(2 * (*my_ADCH_DATA & (1 << 1)), 9) + *my_ADCL_DATA;
}

void clearLEDS()
{
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
}

bool getDisabledState()
{
  int btnPushed = digitalRead(DIS_EN_BTN_PIN);
  Serial.print(btnPushed);
  if (btnPushed == HIGH && dis_en_btn_state == 1)
  {
    dis_en_btn_state = 0;
    return true;
  }
  else
  {
    dis_en_btn_state = 1;
    return false;
  }
}

void setSevoPosition()
{
  int incBtn = digitalRead(INCREMENT_SERVO_ANGLE);
  int decBtn = digitalRead(DECREMENT_SERVO_ANGLE);

  if (incBtn == HIGH)
  {
    if (servoPos <= 180)
      servoPos++;
  }
  if (decBtn == HIGH)
  {
    if (servoPos >= 0)
      servoPos--;
  }
}

// void printTime(){
//   rtc.getTime();
//   Serial.print(rtc.hour, DEC);
//   Serial.print(":");
//   Serial.print(rtc.minute, DEC);
//   Serial.print(":");
//   Serial.print(rtc.second, DEC);
//   Serial.print("  ");
//   Serial.print(rtc.month, DEC);
//   Serial.print("/");
//   Serial.print(rtc.dayOfMonth, DEC);
//   Serial.print("/");
//   Serial.print(rtc.year+2000, DEC);
//   Serial.print(" ");
//   Serial.print(rtc.dayOfMonth);
//   Serial.print("*");
//   switch (rtc.dayOfWeek){
//       case MON:
//       Serial.print("MON");
//       break;
//       case TUE:
//       Serial.print("TUE");
//       break;
//       case WED:
//       Serial.print("WED");
//       break;
//       case THU:
//       Serial.print("THU");
//       break;
//       case FRI:
//       Serial.print("FRI");
//       break;
//       case SAT:
//       Serial.print("SAT");
//       break;
//       case SUN:
//       Serial.print("SUN");
//       break;
//   }
//   Serial.println(" ");
