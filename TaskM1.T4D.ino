#include <avr/interrupt.h>
#include <avr/io.h>

// -------------------- PIN ASSIGNMENTS --------------------
#define PIR_SENSOR_PIN    2   // PIR Sensor (PCINT)
#define PUSH_BUTTON_PIN   3   // Push Button (PCINT)
#define TMP36_PIN         A0  // Temperature Sensor (Analog)
#define PING_SENSOR_PIN   5   // Parallax PING))) (Manually triggered in loop)
#define LED_SENSOR        9   // LED for sensor events
#define LED_TIMER         10  // LED we blink in loop (no hardware timer)

// -------------------- THRESHOLD VALUES --------------------
const float TEMP_THRESHOLD  = 30.0; // °C
const float DIST_THRESHOLD  = 10.0; // cm

// -------------------- TIMING SETTINGS --------------------
const unsigned long LED_TIMER_ON_MS   = 2000; // LED_TIMER ON duration
const unsigned long LED_TIMER_OFF_MS  = 2000; // LED_TIMER OFF duration
const unsigned long SENSOR_LED_ON_MS  = 2000; // LED_SENSOR stays ON after sensor triggers
const unsigned long READ_INTERVAL_MS  = 1000; // Read temperature & distance every 1 second

// -------------------- GLOBAL VARIABLES --------------------
// For PIR & Button edge detection
static bool lastPirState    = false;
static bool lastButtonState = true;  // Button is INPUT_PULLUP (normally HIGH)

// PCINT-set flags
volatile bool pirTriggered  = false;
volatile bool buttonPressed = false;

// Temperature & Distance states
static bool lastTempAbove   = false;
static bool lastDistBelow   = false;
bool tempAbove     = false;
bool distBelow     = false;
bool tempChanged   = false;
bool distChanged   = false;

// LED timeouts
unsigned long ledOnUntil    = 0;   // for LED_SENSOR
unsigned long ledTimerUntil = 0;   // for LED_TIMER blinking
bool ledTimerState          = false; 

// For sensor reading in loop
unsigned long lastReadTime  = 0;

// -------------------- setup() --------------------
void setup() {
    Serial.begin(9600);

    // Configure digital pins
    pinMode(PIR_SENSOR_PIN,    INPUT);
    pinMode(PUSH_BUTTON_PIN,   INPUT_PULLUP);
    pinMode(PING_SENSOR_PIN,   OUTPUT); // We'll toggle to INPUT when measuring echo
    pinMode(TMP36_PIN,         INPUT);
    pinMode(LED_SENSOR,        OUTPUT);
    pinMode(LED_TIMER,         OUTPUT);

    // Start with LED_TIMER OFF
    digitalWrite(LED_TIMER, LOW);

    // Enable PCINT for PIR & Button on Port D (D2-D7)
    PCICR  |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT18);  // PIR (D2)
    PCMSK2 |= (1 << PCINT19);  // Button (D3)

    lastPirState    = digitalRead(PIR_SENSOR_PIN);
    lastButtonState = digitalRead(PUSH_BUTTON_PIN);
    
    Serial.println("=== System Initialized ===");
}

// -------------------- PCINT2_vect: Handles PIR & Button --------------------
ISR(PCINT2_vect) {
    // PIR Rising Edge
    bool currentPir = digitalRead(PIR_SENSOR_PIN);
    if (currentPir && !lastPirState) {
        pirTriggered = true;
    }
    lastPirState = currentPir;

    // Button Falling Edge
    bool currentButton = digitalRead(PUSH_BUTTON_PIN);
    if (!currentButton && lastButtonState) {
        buttonPressed = true;
    }
    lastButtonState = currentButton;
}

// -------------------- loop() --------------------
void loop() {
    unsigned long now = millis();

    // 1) Blink LED_TIMER in loop (No hardware timer)
    if (now >= ledTimerUntil) {
        ledTimerState = !ledTimerState;
        digitalWrite(LED_TIMER, (ledTimerState) ? HIGH : LOW);

        // Determine next duration
        ledTimerUntil = now + (ledTimerState ? LED_TIMER_ON_MS : LED_TIMER_OFF_MS);
    }

    // 2) Read Temperature & Distance every 1 second
    if (now - lastReadTime >= READ_INTERVAL_MS) {
        lastReadTime = now;
        
        // Check Temperature
        float temperature = readTMP36();
        bool nowTempAbove = (temperature >= TEMP_THRESHOLD);
        if (nowTempAbove != lastTempAbove) {
            tempAbove = nowTempAbove;
            tempChanged = true;
            lastTempAbove = nowTempAbove;
        }

        // Check Distance
        float distance = measurePing();
        bool nowDistBelow = (distance > 0 && distance < DIST_THRESHOLD);
        if (nowDistBelow != lastDistBelow) {
            distBelow = nowDistBelow;
            distChanged = true;
            lastDistBelow = nowDistBelow;
        }
    }

    // 3) Handle PIR
    bool sensorEvent = false; 
    if (pirTriggered) {
        Serial.println("[PIR] Motion Detected!");
        sensorEvent    = true;
        pirTriggered   = false;
    }

    // 4) Handle Button
    if (buttonPressed) {
        Serial.println("[BUTTON] Pressed!");
        sensorEvent     = true;
        buttonPressed   = false;
    }

    // 5) Handle Temperature changes
    if (tempChanged) {
        if (tempAbove) {
            Serial.println("[TEMP] Above 30°C - LED ON");
            sensorEvent = true;
        } else {
            Serial.println("[TEMP] Below 30°C - LED OFF");
        }
        tempChanged = false;
    }

    // 6) Handle Distance changes
    if (distChanged) {
        if (distBelow) {
            Serial.println("[PING] Distance < 10cm - LED ON");
            sensorEvent = true;
        } else {
            Serial.println("[PING] Distance > 10cm - LED OFF");
        }
        distChanged = false;
    }

    // 7) LED_SENSOR ON for 2 seconds if any sensor triggered
    if (sensorEvent) {
        ledOnUntil = now + SENSOR_LED_ON_MS;
    }
    digitalWrite(LED_SENSOR, (now < ledOnUntil) ? HIGH : LOW);
}

// -------------------- measurePing: Trigger & Read PING))) --------------------
float measurePing() {
    // Trigger Pulse
    pinMode(PING_SENSOR_PIN, OUTPUT);
    digitalWrite(PING_SENSOR_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(PING_SENSOR_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(PING_SENSOR_PIN, LOW);

    // Read Echo
    pinMode(PING_SENSOR_PIN, INPUT);
    long duration = pulseIn(PING_SENSOR_PIN, HIGH, 20000UL);  // 20ms timeout
    if (duration == 0) return -1.0;  // Handle timeout case
    return duration * 0.0343f / 2.0f;
}

// -------------------- readTMP36: Returns Temperature in °C --------------------
float readTMP36() {
    int adcVal = analogRead(TMP36_PIN);
    float voltage = (adcVal * 5.0f) / 1023.0f;
    return (voltage - 0.5f) * 100.0f;
}
