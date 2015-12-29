#include <Wire.h>
//DHT11
//Library: http://github.com/adafruit/DHT-sensor-library 
#include "DHT.h"
#define DHTPIN 7     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// LCD I2C Library
// https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
#include <LiquidCrystal_I2C.h>
const int buttonPin = 2;
const int lcdtimer = 10;
int buttonState = 0;  //button pin for LCD screen open
int buttonAlive = 10;  //contdown for screen show cycle

// Sharp GP2Y1010AU
int measurePin = 0; //Connect dust sensor to Arduino A0 pin
int ledPower = 2;   //Connect Digital 2 led driver pins of dust sensor to Arduino D2
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

// built quene to average the value for sharp GP2Y1010AU0F
const int MAX_QUEUE = 4;
float Queue[5]={0.0} ;
int Quene_position = -1;
float Avg = 0.0;

int Index_add() {
  Quene_position++;
  if (Quene_position > MAX_QUEUE){
    Quene_position = 0;
    return 1;
  }
  return 0;
}

void Add(float data){
  float total = 0;
  Quene_position++;
  if (Quene_position > MAX_QUEUE) Quene_position = 0;
  Serial.print("Quene_position:");
  Serial.println(Quene_position);
  Queue[Quene_position] = data;
  for (int i=0;i<=MAX_QUEUE;i++){
    Serial.print(Queue[i]);
    Serial.print("/");
    total += Queue[i];
  }
  Serial.println("-------Queue raw data");
  Avg = total / (MAX_QUEUE + 1);
}
// eof quene

// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // LCD I2C address

void setup(){
  pinMode(ledPower,OUTPUT);
  Serial.begin(9600);

  dht.begin();
  
  lcd.begin(16, 2);
    // turn on&off 3 times
  for(int i=0; i<3; i++) {
    lcd.backlight(); // open back light
    delay(250);
    lcd.noBacklight(); // close back light
    delay(250);
  }
  lcd.backlight();
  // LCD screen initial
  lcd.setCursor(0, 0); 
  lcd.print("Humidity & Temp");
  delay(1000);
  lcd.setCursor(0, 1);
  lcd.print("Dust PM2.5");
  delay(500);
  
  //run 10 times to catch stabilize value
  Serial.println("Run 10 times");
  for (int i=1;i<=10;i++) {
    digitalWrite(ledPower,LOW);
    delayMicroseconds(samplingTime);
    voMeasured = analogRead(measurePin); 
    delayMicroseconds(deltaTime);
    digitalWrite(ledPower,HIGH);
    delayMicroseconds(sleepTime);
    calcVoltage = voMeasured * (5.0 / 1024.0);
    dustDensity = (0.17 * calcVoltage - 0.1) * 1000;
    Add(dustDensity);
    delay(1000);
  }
  Serial.print("Average = ");
  Serial.println(Avg);
  Serial.println("----------------");
  Serial.println(" ");
}

void loop(){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  int h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  int t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  int f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.setCursor(0, 0);
    lcd.print("!! DHT Fail !!");
    delay(1000);
    //return;
    }
    // Compute heat index in Fahrenheit (the default)
  //float hif = dht.computeHeatIndex(f, h); // don't show hif in screen
  // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);
  
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print("%\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println("*C\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.println("*C\t");
  Serial.println("------------");

  // ledPower is any digital pin on the arduino connected to Pin 3 on the sensor
  digitalWrite(ledPower,LOW); // power on the LED
  delayMicroseconds(samplingTime);
  voMeasured = analogRead(measurePin); // read the dust value via pin 0 on the sensor
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH); // turn the LED off
  delayMicroseconds(sleepTime);

  // 0 - 5V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (5.0 / 1024.0);

  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = (0.17 * calcVoltage - 0.1) * 1000;

  Add(dustDensity);
  Serial.print("Raw Signal Value (0-1023): ");
  Serial.print(voMeasured);
  Serial.print(" - Voltage: ");
  Serial.print(calcVoltage);
  Serial.print(" - Dust Density: ");
  Serial.print(dustDensity); // unit is ( ug/m3 )
  Serial.println(" ug/m3 ");
  Serial.print("Average = ");
  Serial.println(Avg);

  buttonState = analogRead(buttonPin);
  Serial.println("------------");
  Serial.print("Button state = ");
  Serial.print(buttonState);
  Serial.print(" / buttonAlive = ");
  Serial.println(buttonAlive);
  Serial.println("------------");
  if (buttonAlive < 0) buttonAlive = 99; // if countdown 10 times, will close LCD back light
  if (buttonState > 800) buttonAlive = 9;
  if (buttonState < 800 && buttonAlive > lcdtimer) {
    lcd.noBacklight();
    delay(2000);
    return;
  }
  if (buttonAlive <= lcdtimer){
    buttonAlive--;
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Humidity:");
    lcd.setCursor(10, 0);
    lcd.print(h);
    lcd.setCursor(13, 0);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("Temp:");
    lcd.setCursor(10, 1);
    lcd.print(t);
    lcd.setCursor(13, 1);
    lcd.print("C");
    delay(5000);
    lcd.clear();  
    lcd.setCursor(0,0);
    lcd.print("  Dust Density: ");
    lcd.setCursor(5,1);
    lcd.print(Avg);
    lcd.setCursor(11,1);
    lcd.print("ug/m3");
  }
  delay(2000);
  lcd.clear();
}
