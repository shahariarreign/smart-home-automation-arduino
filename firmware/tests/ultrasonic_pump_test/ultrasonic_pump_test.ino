/*
  HC-SR04 Water Level + Pump - Standalone Test
  Wiring:
    HC-SR04: Trig->D7, Echo->D8, 5V, GND
    Relay (pump): IN->D4, 5V, GND
  Logic:
    distance >= 13 cm -> water LOW -> pump ON
    distance <= 3 cm  -> water FULL -> pump OFF
  Open Serial Monitor at 9600 baud.
*/

#define TRIG_PIN 7
#define ECHO_PIN 8
#define PUMP_RELAY 4

const float LOW_THRESHOLD_CM  = 13.0; // distance at/above this -> pump ON
const float FULL_THRESHOLD_CM = 3.0;  // distance at/below this -> pump OFF

bool pumpOn = false;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  digitalWrite(PUMP_RELAY, LOW); // adjust LOW/HIGH to match your relay's active state
}

void loop() {
  long distance = getDistanceCM();

  if (distance >= LOW_THRESHOLD_CM) {
    pumpOn = true;
  } else if (distance <= FULL_THRESHOLD_CM) {
    pumpOn = false;
  }
  // else: distance is between thresholds -> keep current pump state

  digitalWrite(PUMP_RELAY, pumpOn ? HIGH : LOW);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Pump: ");
  Serial.println(pumpOn ? "ON" : "OFF");

  delay(500);
}

long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration * 0.034 / 2;
}
