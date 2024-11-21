#include <Wire.h>
#include "MAX30105.h"
#include <Adafruit_MLX90614.h>
#include "heartRate.h"
#include "spo2_algorithm.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int ledPin = 13;
const int ledPin1 = 12;
unsigned long startTime;
MAX30105 particleSensor;
Adafruit_MLX90614 mlx;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;

const byte IR_SIZE = 4;
long irValues[IR_SIZE];
byte irSpot = 0;
long irAvg;

const byte TEMP_SIZE = 4;
float tempValues[TEMP_SIZE];
byte tempSpot = 0;
float tempAvg;

void setup()
{
  startTime = millis();
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin1, OUTPUT);
  Serial.begin(115200);
  Serial.println("Initializing...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  mlx.begin();

  // Initialize the OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000); // Pause for 2 seconds
  display.clearDisplay();
}

void loop()
{
  digitalWrite(ledPin, LOW);

  long irValue = particleSensor.getIR();


  if (checkForBeat(irValue) == true)
  {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  irValues[irSpot++] = irValue;
  irSpot %= IR_SIZE;
  irAvg = 0;
  for (byte x = 0; x < IR_SIZE; x++)
    irAvg += irValues[x];
  irAvg /= IR_SIZE;

  float objectTempC = mlx.readObjectTempC();
  float objectTempF = (objectTempC * 9 / 5) + 32.0;

  tempValues[tempSpot++] = objectTempF;
  tempSpot %= TEMP_SIZE;
  tempAvg = 0;
  for (byte x = 0; x < TEMP_SIZE; x++)
    tempAvg += tempValues[x];
  tempAvg /= TEMP_SIZE;

  long redValue = particleSensor.getRed();

  // Calculate SpO2 using the spo2_algorithm library
  int32_t spo2 = 0;
  int8_t validSPO2 = 0;
  int32_t hr = 0;
  int8_t validHR = 0;

  // Call the function with correct arguments
  maxim_heart_rate_and_oxygen_saturation((uint32_t *)irValues, IR_SIZE, (uint32_t *)&redValue, &spo2, &validSPO2, &hr, &validHR);

  // Display sensor values on the serial monitor in CSV format


  Serial.print(irValue);
  Serial.print(",");
  Serial.print(irAvg);
  Serial.print(",");
  Serial.print(beatsPerMinute);
  Serial.print(",");
  Serial.print(beatAvg);
  Serial.print(",");
  Serial.print(objectTempF);
  Serial.print(",");
  Serial.print(tempAvg);
  Serial.print(",");
  Serial.print(spo2);

  if (irValue < 50000)
  {
    Serial.print(", No finger");
    digitalWrite(ledPin, HIGH);
  }

  Serial.println();

  // Update and display the timer on the OLED screen
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - startTime;
  unsigned long remainingTime = 60000 - elapsedTime;
  int seconds = remainingTime / 1000;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Psy-Sense Time= ");
  display.print(seconds);
  display.print("s");



    display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.print("Temp: ");
  display.print(tempAvg);
  display.print("F");

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.print("BPM: ");
  display.print(beatsPerMinute);

  display.display();

  if (millis() - startTime >= 60000)
  
  {
    digitalWrite(ledPin1, HIGH);
  
  }
  else
  {
    digitalWrite(ledPin1, LOW);
  }
  
}
