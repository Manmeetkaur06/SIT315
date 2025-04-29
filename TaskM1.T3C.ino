// Define pins
#define PIR_SENSOR_PIN 2  // PIR sensor digital output pin
#define TEMP_SENSOR_PIN A0  // TMP36 analog output pin
#define LED_PIN 13  // LED connected to Pin 13

// Temperature threshold for triggering action (in °C)
#define TEMP_THRESHOLD 30.0  

// Volatile variable for interrupt handling
volatile bool motionDetected = false;

void setup() {
    // Configure pin modes
    pinMode(PIR_SENSOR_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);

    // Start Serial Monitor
    Serial.begin(9600);

    // Attach interrupt for the PIR sensor
    attachInterrupt(digitalPinToInterrupt(PIR_SENSOR_PIN), pirISR, RISING);

    // Debug message
    Serial.println("Multiple Inputs Board Initialized...");
}

void loop() {
    // Read temperature from TMP36
    int tempRaw = analogRead(TEMP_SENSOR_PIN);
    float voltage = tempRaw * (5.0 / 1023.0);  // Convert raw value to voltage
    float temperature = (voltage - 0.5) * 100.0;  // Convert voltage to Celsius

    // Check if motion is detected
    if (motionDetected) {
        Serial.println("Motion detected! Turning LED ON...");
        digitalWrite(LED_PIN, HIGH);  // Turn LED on
        delay(1000);                  // Keep LED on for 500ms
        digitalWrite(LED_PIN, LOW);  // Turn LED off
        motionDetected = false;      // Reset motion detected flag
    }

    // Check if temperature exceeds threshold
    if (temperature > TEMP_THRESHOLD) {
        Serial.print("High Temperature Detected: ");
        Serial.print(temperature);
        Serial.println(" °C. Turning LED ON...");
        digitalWrite(LED_PIN, HIGH);  // Turn LED on
        delay(1000);                   // Keep LED on for 500ms
        digitalWrite(LED_PIN, LOW);   // Turn LED off
    } else {
        Serial.print("Temperature is Normal: ");
        Serial.print(temperature);
        Serial.println(" °C");
    }

    delay(1000);  // Delay for stability
}

// Interrupt Service Routine (ISR) for PIR sensor
void pirISR() {
    motionDetected = true;  // Set flag when motion is detected
}
