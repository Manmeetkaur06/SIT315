// Define pin numbers for PIR motion sensor and LED
#define MOTION_SENSOR_PIN 2  // Interrupt-capable pin connected to the PIR sensor
#define LED_PIN 9            // Digital pin connected to the LED

// Global flag to track if motion is detected
volatile bool motionDetected = false;

// Interrupt Service Routine (ISR) for handling motion detection
void motionISR() {
  motionDetected = true; // Set the motionDetected flag when motion is detected
}

void setup() {
  // Configure PIR motion sensor pin as input with an internal pull-up resistor
  pinMode(MOTION_SENSOR_PIN, INPUT_PULLUP); 
  
  // Configure LED pin as output
  pinMode(LED_PIN, OUTPUT);  
  
  // Initialize serial communication for debugging and monitoring
  Serial.begin(9600);  
  Serial.println("System Initialized");  // Print system initialization message

  // Attach interrupt to the motion sensor pin
  // Trigger ISR (motionISR) on a FALLING edge (motion detected)
  attachInterrupt(digitalPinToInterrupt(MOTION_SENSOR_PIN), motionISR, FALLING);
}

void loop() {
  // Check if motionDetected flag is set
  if (motionDetected) {
    // Print message to Serial Monitor
    Serial.println("Motion detected: LED ON");

    // Turn the LED ON to indicate motion
    digitalWrite(LED_PIN, HIGH); 
    
    // Wait for 1000 ms (1 second)
    delay(1000);  
    
    // Turn the LED OFF
    digitalWrite(LED_PIN, LOW);  
    
    // Print message to Serial Monitor
    Serial.println("LED OFF");

    // Reset the motionDetected flag to wait for the next motion
    motionDetected = false;  
  }
}
