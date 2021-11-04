/**************************************************************************************************************************************
FILENAME: swamp_cooler.ino
AUTHOR: Samuel Lehman, Gordan Tan, Andrew Cunnignham
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

// for dth11
#include <Adafruit_10DOF.h>
#include <DHT.h>

// prototypes
void displayHumidTemp();
void checkWaterLevel(int);
int getState();

// TODO set DHT input pin, water sensor input 
// definitions
#define DHT_IPIN A0 // placeholder
#define WATER_SENSOR_INPUT A1 // placeholder
#define WATER_LEVEL_THRESHOLD 50

// TODO set pin values for lcd
// creates object lcd(pins)
LiquidCrystal lcd(1,2,3,4,5,6); // pins in currently are just place holders
DHT dht(DHT_IPIN, DHT11);

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
}

void loop() {
  int waterLevel = analogRead(WATER_SENSOR_INPUT);
  int state = getState();
  // 0 error, 1 running, 2 idle, 3 disabled
  if(state == 0){
    displayHumidTemp();
  }else if(state == 1){
    displayHumidTemp();
  }else if (state == 2){
    displayHumidTemp();
  }else if (state == 3){

  }else{}
  
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
  delay(500);
}

void checkWaterLevel(int waterLevel){
  if (waterLevel < 100){
    Serial.print("WARNING: Water level low");
  }
  delay(500);
}

int getState(){
  int waterLevel = analogRead(WATER_SENSOR_INPUT);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  dht.read(DHT_IPIN);
  if (waterLevel < WATER_LEVEL_THRESHOLD){
    // error
    return 0;
  }
  else if (t > 30){
    // running
    return 1;
  }
  else if (t < 20){
    // idle
    return 2;
  }
  else if (h > 80){
    // running  
    return 1;
  }
  else if (h < 60){
    // idle
    return 2;
  }
  else{
    // disabled
    return 3;
  }
}