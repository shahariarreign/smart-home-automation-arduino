/*
  DHT11 Temperature + Fan - Standalone Test
  Wiring:
    DHT11: VCC->5V, GND->GND, DATA->D2
    Relay (fan): IN->D3, 5V, GND
  Requires: "DHT sensor library" by Adafruit (+ "Adafruit Unified Sensor")
  Open Serial Monitor at 9600 baud.
*/

#include <DHT.h>

#define DHT_PIN  2
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

#define FAN_RELAY 3
const float TEMP_THRESHOLD = 31.0; // deg C, adjust as needed
bool fanOn = false;

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(FAN_RELAY, OUTPUT);
  digitalWrite(FAN_RELAY, LOW); // adjust LOW/HIGH to match your relay's active state
}

void loop() {
  float tempC = dht.readTemperature();

  if (isnan(tempC)) {
    Serial.println("Failed to read from DHT11 - check wiring");
    delay(1000);
    return;
  }

  if (!fanOn && tempC >= TEMP_THRESHOLD) {
    fanOn = true;
    digitalWrite(FAN_RELAY, HIGH);
  } else if (fanOn && tempC < TEMP_THRESHOLD - 1) { // 1C hysteresis
    fanOn = false;
    digitalWrite(FAN_RELAY, LOW);
  }

  Serial.print("Temp: ");
  Serial.print(tempC);
  Serial.print(" C | Fan: ");
  Serial.println(fanOn ? "ON" : "OFF");

  delay(1000); // DHT11 needs >=1s between reads
}
