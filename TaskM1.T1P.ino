#define MOTION_SENSOR_PIN 5 // Motion sensor connected to digital pin 5
#define LED_PIN 9         // LED connected to pin 9

void setup() {
  pinMode(MOTION_SENSOR_PIN, INPUT); // Set motion sensor pin as input
  pinMode(LED_PIN, OUTPUT);          // Set LED pin as output
  Serial.begin(9600);                // Start serial communication at 9600 baud rate
  Serial.println("System Initialized");
}

void loop() {
  int motionDetected = digitalRead(MOTION_SENSOR_PIN); // Read motion sensor data

  if (motionDetected == HIGH) {
    digitalWrite(LED_PIN, HIGH); // Turn LED ON
    Serial.println("Motion detected: LED ON");
  } else {
    digitalWrite(LED_PIN, LOW); // Turn LED OFF
    Serial.println("No motion: LED OFF");
  }

  delay(500); // Delay to stabilize readings
}
