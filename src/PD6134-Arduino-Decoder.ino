
#include <MCP4151.h>

#define CS     10
#define MOSI   11
#define MISO   12
#define SCK    13

MCP4151 pot(CS, MOSI, MISO, SCK);
// MCP4151 pot(CS, MOSI, MISO, SCK, 4000000, 250000, SPI_MODE0);

#define PULSE_PIN 2 // Pin connected to the pulse signal
// #define PWM_PIN 3 // PWM capable pin for controlling the voltage
// Timing constants (in microseconds)
#define PULSE_MIN 460      // Minimum pulse duration
#define PULSE_MAX 520      // Maximum pulse duration
#define GAP_MIN 680        // Minimum gap duration
#define GAP_MAX 730        // Maximum gap duration
#define END_NUMBER 1900    // Minimum duration to indicate end of a number
#define END_SEQUENCE 10000 // Duration indicating the end of a sequence (10 milliseconds)

#define MAX_NUMBERS 6      // Maximum numbers in a sequence

int numbers[MAX_NUMBERS];  // Array to store the numbers in a sequence
int numberCount = 0;       // Number of pulses in the current number.
int sequenceCount = 0;     // Number of numbers in the current sequence
unsigned long lastPulseTime = 0;
bool isPulse = false;      // Flag to track if we are in a pulse

unsigned long lastActivityTime = 0;
const unsigned long inactivityThreshold = 100000; // 100 milleseconds, adjust as needed


struct SequenceMapping {
    String name;
    int sequence[MAX_NUMBERS];
    int length;
};

SequenceMapping lookupTable[] = {
    {"CHG", {4, 3, 1, 1, 2}, 5},
    {"MODE", {5, 2, 1, 1, 2}, 5},
    {"VOLDOWN", {4, 3, 4}, 3},
    {"VOLUP", {7, 4}, 2},
    {"SKIPBACK", {7, 1, 1, 2}, 4},
    {"SKIPFWD", {4, 1, 2, 1, 1, 2}, 6}
};
const int lookupTableSize = sizeof(lookupTable) / sizeof(SequenceMapping);


void setup() {
    // Serial.begin(9600);
    pinMode(PULSE_PIN, INPUT_PULLUP);
    pot.writeValue(0); // turn the pot off
    resetSequence();
}

void loop() {
    int pinState = digitalRead(PULSE_PIN);
    unsigned long currentTime = micros();
    unsigned long interval = currentTime - lastPulseTime;
    if (micros() - lastActivityTime > inactivityThreshold) {
        pot.writeValue(0); // Reset potentiometer
        lastActivityTime = micros(); // Reset the activity timer to avoid continuous resets
    }

    if (pinState == LOW && !isPulse) {
        // Start of a pulse
        isPulse = true;
        lastPulseTime = currentTime;
        lastActivityTime = currentTime;
        if (interval >= END_NUMBER && interval < END_SEQUENCE) {
            // End of a number
            if (sequenceCount < MAX_NUMBERS) {
                numbers[sequenceCount++] = numberCount;
                numberCount = 0;  // Reset to 0 for the next number
            }
        } else if (interval >= END_SEQUENCE) {
            // End of a sequence
            if (numberCount > 0 && sequenceCount < MAX_NUMBERS) {
                // Store the last number if it hasn't been stored
                numbers[sequenceCount++] = numberCount;
            }
            printSequence();
            resetSequence();
        }
    } else if (pinState == HIGH && isPulse) {
        // End of a pulse
        lastActivityTime = currentTime;
        isPulse = false;
        if (interval >= PULSE_MIN && interval <= PULSE_MAX) {
            // Valid pulse, increment pulse count
            numberCount++;
        }
    }
}

void resetSequence() {
    numberCount = 0;  // Initialize to 0
    sequenceCount = 0;
    for (int i = 0; i < MAX_NUMBERS; i++) {
        numbers[i] = 0;
    }
}

void printSequence() {
    String sequenceName = translateSequence();
    setResistanceForSequence(sequenceName);
    // Serial.print("Button Pressed: ");
    // Serial.println(sequenceName);
    lastActivityTime = micros(); // Update last activity time after processing a sequence
}

void setResistanceForSequence(String sequenceName) {
    if (sequenceName == "CHG") pot.writeValue(55); // Minimum value known to work     
    else if (sequenceName == "MODE") pot.writeValue(80); 
    else if (sequenceName == "VOLDOWN") pot.writeValue(110);
    else if (sequenceName == "VOLUP") pot.writeValue(148);
    else if (sequenceName == "SKIPBACK") pot.writeValue(185);
    else if (sequenceName == "SKIPFWD") pot.writeValue(215);
    else pot.writeValue(0); // Default or unknown sequence
}

String translateSequence() {
    for (int i = 0; i < lookupTableSize; i++) {
        if (sequenceCount == lookupTable[i].length) {
            bool match = true;
            for (int j = 0; j < sequenceCount; j++) {
                if (numbers[j] != lookupTable[i].sequence[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return lookupTable[i].name;
            }
        }
    }
    return "UNKNOWN";
}


