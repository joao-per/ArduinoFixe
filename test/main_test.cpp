/* MINIMAL TEST - Sistema de Arrefecimento */
#include <Arduino.h>

int counter = 0;

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(2000); // Wait for serial to initialize
    
    // Print startup messages
    Serial.println("========================================");
    Serial.println("   MINIMAL TEST - STM32L476RG");
    Serial.println("========================================");
    Serial.println("Serial communication working!");
    Serial.println("Starting main loop...");
    
    // Initialize built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    // Blink LED and print counter
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("Counter: ");
    Serial.println(counter++);
    delay(500);
    
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED OFF");
    delay(500);
    
    // Print status every 10 loops
    if (counter % 10 == 0) {
        Serial.println("--- System running normally ---");
    }
}