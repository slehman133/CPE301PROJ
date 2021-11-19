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
The exact time (using real-time clock) should record transition times
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
#include <Adafruit_10DOF.h>
#include <DHT.h>

// for rDS1307 RTC (real time clock)
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
unsigned int adc_read();


// TODO set DHT input pin, water sensor input 
// definitions
#define DHT_IPIN A0 // placeholder
#define WATER_SENSOR_INPUT A1 // placeholder
#define WATER_LEVEL_THRESHOLD 50
#define TEMP_HIGH_THRESHOLD 80
#define YELLOW_LED_PIN
#define GREEN_LED_PIN
#define RED_LED_PIN
#define BLUE_LED_PIN

// TODO set pin values for lcd
// creates object lcd(pins)
LiquidCrystal lcd(1,2,3,4,5,6); // pins in currently are just place holders
DHT dht(DHT_IPIN, DHT11);

// DIGITAL PORT B REGISTERS
volatile unsigned char* portB = (unsigned char*) 0x25;
volatile unsigned char* portDDRB = (unsigned char*) 0x24;
volatile unsigned char* pinB = (unsigned char*) 0x23;

// ANALOG
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADCH_DATA = (unsigned int*) 0x79;
volatile unsigned int* my_ADCL_DATA = (unsigned int*) 0x78;

void setup() {
  adc_init();
  Serial.begin(9600);
  lcd.begin(16, 2);
  dht.begin();
  *portB &= 0b01111111;
  *portDDRB &= 0b01111111;
  *portDDRB |= 0b01111110;
}

void loop() {
  int waterLevel = adc_read(WATER_SENSOR_INPUT);
  int tempState;

  bool isDisabled = getDisabledState();
  bool isRunning = false;
  bool isIdle = false;
  bool isError = false;

  int state = getState();
  if(!isDisabled){
    clearLEDS();
    do{
      tempState = getState();
      switch(state){
        // error 
        case 0:
          isError = true;
          error_state();
          // TODO set error LED high
          if(getDisabledState()) state = 3; // if disabled, go to disabled state
          break;
        // running
        case 1:
          isRunning = true;
          running_state();  
          // TODO set running LED high
          if(getDisabledState()) state = 3;
          break;
        // idle
        case 2:
          isIdle = true;
          idle_state();
          // TODO set idle LED high
          if(getDisabledState()) state = 3;
          break;
      }
    }while(state == tempState);
    clearLEDS();
  }else{
    disabled_state();
    //TODO set disabled LED high
    state = getState();
  }
}

void displayHumidTemp(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  dht.read(DHT_IPIN);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.setCursor(0,1);
  lcd.print("Humid: ");
  lcd.print(h);
}

void checkWaterLevel(int waterLevel){
  if (waterLevel < 100){
    Serial.print("WARNING: Water level low");
  }
}

int getState(){
  int waterLevel = adc_read(WATER_SENSOR_INPUT);
  float t = dht.readTemperature();
  dht.read(DHT_IPIN);
  if(waterLevel < WATER_LEVEL_THRESHOLD){
    // error
    return 0;
  }else if(t > TEMP_HIGH_THRESHOLD){
    // running
    return 1;
  }else if(waterLevel > WATER_LEVEL_THRESHOLD && t < TEMP_HIGH_THRESHOLD){
    // idle
    return 2;
  }
}

/* States */
void disabled_state(){
  // TODO add code to send date from realtime clock to host computer
}

void idle_state(){
  displayHumidTemp();
}

void error_state(){
  displayHumidTemp();
}

void running_state(){
  displayHumidTemp();
}

/* Analog/Digital Conversion */
void adc_init() {
  // Register A
  *my_ADCSRA |= 0x80; // Set Bit 7 to 1
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

unsigned int adc_read(unsigned char adc_channel_num){
  // Clear Analog channel selection bits
  *my_ADMUX &= 0b11100000;
  *my_ADMUX &= 0b11011111;
  // Set channel number
  if (adc_channel_num > 7) {
    adc_channel_num -= 8;
    *my_ADCSRB |= 0b00001000;
  }
  // Set channel selection bits
  *my_ADMUX += adc_channel_num;
  *my_ADCSRA |= 0b01000000;
  // Wait for conversion to complete and return result
  while ((*my_ADCSRA & 0x40) != 0);
  return pow(2 * (*my_ADCH_DATA & (1 << 0)), 8) + pow(2 * (*my_ADCH_DATA & (1 << 1)), 9) + *my_ADCL_DATA; 
}

void clearLEDS(){
  // TODO add code to turn off all LEDs set them low
}

bool getDisabledState(){
  //TODO add code to check if disabled button is pressed
  return false;
}
