#include <MCP4151.h>

// SPI Pins
#define CS     10
#define MOSI   11
#define MISO   12
#define SCK    13

MCP4151 pot(CS, MOSI, MISO, SCK);  // Initialize the MCP4151 device

#define PULSE_PIN 2                // Pin connected to the pulse signal

// Pulse timing constants (based on your pulse characteristics)
#define PULSE_MIN 460              // Minimum pulse duration in microseconds
#define PULSE_MAX 520              // Maximum pulse duration in microseconds
#define END_NUMBER 1900            // Minimum duration to indicate the end of a number
#define END_SEQUENCE 10000         // Duration indicating the end of a sequence (10 ms)

#define MAX_NUMBERS 6              // Maximum numbers in a decoded sequence

int numbers[MAX_NUMBERS];          // Array to store the decoded numbers
int numberCount = 0;               // Current pulse count in a number
int sequenceCount = 0;             // Number of numbers in the current sequence
unsigned long lastPulseTime = 0;   // Tracks the last time a pulse occurred
bool isPulse = false;              // Flag to check pulse state

unsigned long lastActivityTime = 0;                 // Timestamp of last activity
const unsigned long inactivityThreshold = 100000;   // Reset after 100ms of inactivity

// For repeated presses tracking
String lastValidSequence = "";                      // Stores the last valid sequence
unsigned long repeatTimeout = 300000;               // Timeout (300ms) between repeat presses
unsigned long lastValidSequenceTime = 0;            // Tracks time of last valid sequence

// Define lookup table for sequence mappings (adjust according to your sequences)
struct SequenceMapping {
    String name;
    int sequence[MAX_NUMBERS];
    int length;
};

// Define your sequences and names (modify as per your requirements)
SequenceMapping lookupTable[] = {
    {"CHG",      {4, 3, 1, 1, 2}, 5},
    {"MODE",     {5, 2, 1, 1, 2}, 5},
    {"VOLDOWN",  {4, 3, 4},       3},
    {"VOLUP",    {7, 4},          2},
    {"SKIPBACK", {7, 1, 1, 2},    4},
    {"SKIPFWD",  {4, 1, 2, 1, 1, 2}, 6}
};

const int lookupTableSize = sizeof(lookupTable) / sizeof(SequenceMapping); // Auto-calculate table size

void setup() {
    pinMode(PULSE_PIN, INPUT_PULLUP);  // Initialize the pulse pin
    pot.writeValue(0);                 // Reset the potentiometer to zero at the start
    resetSequence();                   // Initialize sequence storage
}

void loop() {
    int pinState = digitalRead(PULSE_PIN);
    unsigned long currentTime = micros();
    unsigned long interval = currentTime - lastPulseTime;

    // Handle inactivity reset (reset potentiometer after long inactivity)
    if (currentTime - lastActivityTime > inactivityThreshold) {
        pot.writeValue(0);  // Reset potentiometer to zero
        lastActivityTime = currentTime;  // Avoid continuous reset
    }

    // Signal pickup: Check if a pulse is active
    if (pinState == LOW && !isPulse) {
        // Pulse start
        isPulse = true;
        lastPulseTime = currentTime;
        lastActivityTime = currentTime;

        if (interval >= END_NUMBER && interval < END_SEQUENCE) {
            // End of a valid number
            if (sequenceCount < MAX_NUMBERS) {
                numbers[sequenceCount++] = numberCount;  // Store pulse count as a number
                numberCount = 0;  // Reset pulse count for next number
            }
        } else if (interval >= END_SEQUENCE) {
            // End of a sequence
            if (numberCount > 0 && sequenceCount < MAX_NUMBERS) {
                numbers[sequenceCount++] = numberCount;  // Store the last number
            }
            printSequence();  // Process the detected sequence
            resetSequence();  // Reset for the next sequence
        }
    } else if (pinState == HIGH && isPulse) {
        // Pulse end
        lastActivityTime = currentTime;
        isPulse = false;

        if (interval >= PULSE_MIN && interval <= PULSE_MAX) {
            // Valid pulse detected
            numberCount++;  // Increment pulse count for the current number
        }
    }
}

void printSequence() {
    // Reject short sequences that are likely noise (fewer than 2 pulses)
    if (sequenceCount < 2) {
        resetSequence();  // Reset sequence and return if it's too short
        return;
    }

    // Translate the sequence
    String sequenceName = translateSequence();

    // Allowing repeated presses and holds via 'repeatTimeout'
    unsigned long currentTime = micros();
    if (sequenceName != lastValidSequence || (currentTime - lastValidSequenceTime > repeatTimeout)) {
        // Process the new sequence or allow repetition after timeout
        setResistanceForSequence(sequenceName);    // Trigger the action for the sequence
        lastValidSequence = sequenceName;          // Update the last valid sequence
        lastValidSequenceTime = currentTime;       // Update the time of the last valid sequence
    }

    lastActivityTime = currentTime;  // Mark this as valid activity
}

void setResistanceForSequence(String sequenceName) {
    // Apply corresponding resistance values based on the sequence name
    if (sequenceName == "CHG") {
        pot.writeValue(55);  // Minimum working value
    } else if (sequenceName == "MODE") {
        pot.writeValue(80);
    } else if (sequenceName == "VOLDOWN") {
        pot.writeValue(110);
    } else if (sequenceName == "VOLUP") {
        pot.writeValue(148);
    } else if (sequenceName == "SKIPBACK") {
        pot.writeValue(185);
    } else if (sequenceName == "SKIPFWD") {
        pot.writeValue(215);
    } else {
        pot.writeValue(0);  // Default for unknown sequence
    }
}

void resetSequence() {
    // Reset the sequence state to prepare for the next input
    numberCount = 0;
    sequenceCount = 0;
    for (int i = 0; i < MAX_NUMBERS; i++) {
        numbers[i] = 0;
    }
}

String translateSequence() {
    // Match the detected sequence with the lookup table
    for (int i = 0; i < lookupTableSize; i++) {
        if (sequenceCount == lookupTable[i].length) {
            bool match = true;
            for (int j = 0; j < sequenceCount; j++) {
                if (numbers[j] != lookupTable[i].sequence[j]) {
                    match = false;  // Mismatch found, not a valid sequence
                    break;
                }
            }
            if (match) {
                return lookupTable[i].name;  // Return valid sequence name
            }
        }
    }
    return "UNKNOWN";  // If no match, return "UNKNOWN"
}
