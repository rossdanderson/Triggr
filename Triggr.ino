#include <stdlib.h>
#include <string.h>

// Constants
const int irPin = 10;
const int errorPin = 12;
const int onPin = 8;
const int ledDelay = 100;
const char* delimiter = " ";

// Setup variables
double halfPulseLength = 0.0;
unsigned int burstPairSequence1Length = 0;
unsigned int burstPairSequence2Length = 0;
unsigned int* burstPairSequence1 = NULL;
unsigned int* burstPairSequence2 = NULL;
boolean isOk = true;

// Variables
char* prontoCode = "0000 0067 0069 0000 005e 001a 002f 0019 0017 0019 002f 0019 002f 0019 0017 0019 002f 0019 0017 001a 0017 0019 002f 0019 0017 0019 002f 0019 002f 0019 002f 0019 0017 0019 0017 001a 0017 0019 002f 0019 002f 0019 002f 0019 002f 01bb 005e 0019 002f 0019 0017 0019 002f 0019 002f 0019 0017 0019 002f 0019 0017 001a 0017 0019 002f 0019 0017 0019 002f 0019 002f 0019 002f 0019 0017 0019 0017 001a 0017 0019 002f 0019 002f 0019 002f 0019 002f 01bb 005e 0019 002f 0019 0017 0019 002f 0019 002f 0019 0017 0019 002f 0019 0017 001a 0017 0019 002f 0019 0017 0019 002f 0019 002f 0019 002c 001c 0017 0019 0017 0019 0017 0019 0017 0031 0014 0034 0011 0037 000f 01db 000c 006b 0008 0040 0007 0029 0007 0041 0007 0041 0006 002a 0006 0042 0005 002c 0006 002a 0005 0043 0005 002b 0005 0043 0004 0044 0004 0044 0004 002c 0004 002d 0004 002c 0004 0044 0004 0044 0003 0045 0003 01e7 0006 0072 0003 0045 0003 002d 0003 0045 0003 0045 0003 002d 0003 0045 0003 002d 0004 002c 0004 0044 0004 002c 0004 0044 0004 0044 0004 0044 0004 002c 0004 002d 0004 002c 0004 0044 0004 0044 0004 0044 0004 001a";

void setup() {

    delay(1000);

    Serial.begin(9600);

    // Ignore first code
    char* hexCode = strtok(prontoCode, delimiter);
    isOk = hexCode != NULL;
    
    double pulseLength = 0.0;
    if (isOk) {
        //Second code is frequency
        hexCode = strtok(NULL, delimiter);
        if (hexCode != NULL) {
            double kHz = 1000.0 / (strtol(hexCode, NULL, 16) * 0.241246);
            pulseLength = 1000.0 / kHz;
            halfPulseLength = pulseLength / 2.0;
            Serial.print("Frequency: ");
            Serial.print(kHz);
            Serial.print("kHz - Pulse width: ");
            Serial.print(pulseLength);
            Serial.println(" microseconds");
            if (pulseLength <= 0.0) {
                isOk = false;
            }
        } else {
            isOk = false;
        }
    }
  
    if (isOk) {
        //Third code is burst pair sequence 1 length
        hexCode = strtok(NULL, delimiter);
        if (hexCode != NULL) {
            unsigned int length = strtol(hexCode, NULL, 16);
            Serial.print("Burst pair sequence 1 length: ");
            Serial.println(length);
            if (length > 0) {
                burstPairSequence1Length = length;
                burstPairSequence1 = (unsigned int*) malloc(sizeof(unsigned int) * length * 2);
            }
        } else {
            isOk = false;
        }
    }

    if (isOk) {
        //Fourth code is burst pair sequence 2 length
        hexCode = strtok(NULL, delimiter);
        if (hexCode != NULL) {
            unsigned int length = strtol(hexCode, NULL, 16);
            Serial.print("Burst pair sequence 2 length: ");
            Serial.println(length);
            if (length > 0) {
                burstPairSequence2Length = length;
                burstPairSequence2 = (unsigned int*) malloc(sizeof(unsigned int) * length * 2);
            }
        }
    }

    isOk &= (burstPairSequence1Length > 0 && burstPairSequence1Length % 2 == 0) ||
            (burstPairSequence2Length > 0 && burstPairSequence2Length % 2 == 0);
  
    if (isOk) {
        unsigned int** burstPairSequence = &burstPairSequence1;
        unsigned int* burstPairSequenceLength = &burstPairSequence1Length;

        if (burstPairSequence1Length == 0) {
            burstPairSequence = &burstPairSequence2;
            burstPairSequenceLength = &burstPairSequence2Length;
	    }

        for (unsigned int i = 0; i < *burstPairSequenceLength && isOk; i++) {
            Serial.print(i);
            Serial.print(": ");

            for (int j = 0; j < 2; j++) {
                hexCode = strtok(NULL, delimiter);
                if (hexCode != NULL) {
                    unsigned int intCode = strtol(hexCode, NULL, 16);
                    (*burstPairSequence)[i*2+j] = intCode * pulseLength;
                    if (j == 0) {
                        Serial.print("On: ");
                    } else {
                        Serial.print(" Off: ");
                    }
                    Serial.print((*burstPairSequence)[i*2+j]);
                } else {
                    isOk = false;
                    break;
                }
            }
            Serial.println();

            if (i == *burstPairSequenceLength-1 &&
                burstPairSequence != &burstPairSequence2 &&
                burstPairSequence2Length > 0 &&
                isOk) {
                i = 0;
                burstPairSequence = &burstPairSequence2;
                burstPairSequenceLength = &burstPairSequence2Length;
            }
        }
    }
  
    isOk &= strtok(NULL, delimiter) == NULL;
  
    pinMode(irPin, OUTPUT);
    pinMode(onPin, OUTPUT);
}

void loop() {
    if (isOk) {
        digitalWrite(onPin, HIGH);
        delay(ledDelay);
        digitalWrite(onPin, LOW);
        trigger();
        delay(60000);
    } else {
        digitalWrite(errorPin, HIGH);
        delay(ledDelay);
        digitalWrite(errorPin, LOW);
    }
}

void pulseOn(unsigned int length) {
    unsigned long endPulse = micros() + length;
    while(micros() < endPulse) {
        digitalWrite(irPin, HIGH);
        delayMicroseconds(halfPulseLength);
        digitalWrite(irPin, LOW);
        delayMicroseconds(halfPulseLength);
    }
}

void pulseOff(unsigned int length) {
    unsigned long endDelay = micros() + length;
    while(micros() < endDelay);
}

void trigger() {
    unsigned int** burstPairSequence = NULL;
    unsigned int* burstPairSequenceLength = NULL;

    if (burstPairSequence1 != NULL) {
        burstPairSequence = &burstPairSequence1;
        burstPairSequenceLength = &burstPairSequence1Length;
    } else {
        burstPairSequence = &burstPairSequence2;
        burstPairSequenceLength = &burstPairSequence2Length;
    }

    for (int i=0; i < 2; i++) {
        for (unsigned int j=0; j < *burstPairSequenceLength; j+=2) {
            pulseOn((*burstPairSequence)[j]);
            pulseOff((*burstPairSequence)[j+1]);
        }
        if (burstPairSequence2 != NULL) {
            burstPairSequence = &burstPairSequence2;
            burstPairSequenceLength = &burstPairSequence2Length;
        }
    }
}
