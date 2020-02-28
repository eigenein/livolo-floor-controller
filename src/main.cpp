#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 pins.
constexpr uint8_t dallasDataPin = 2;
constexpr uint8_t dallasPullUpPin = 3;
constexpr uint8_t dallasVccPin = 4;

// Livolo thermostat relay transistor base pin.
constexpr uint8_t cooldownPin = 20;

// Hardcoded floor temperature limitations.
constexpr float enableCooldownTemp = 31;
constexpr float disableCooldownTemp = 29;

// Peripherals.
OneWire oneWire(dallasDataPin);
DallasTemperature sensors(&oneWire);
DeviceAddress dallasAddress;

bool isCooldownEnabled = false;

void setupSerial() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Starting up...");
}

void setupDallas() {
    // I'm NOT using an external pull-up resistor.
    // Instead, the data pin is connected to the pull up pin which is using the internal pull-up.
    pinMode(dallasPullUpPin, INPUT_PULLUP);

    // Power up and start.
    pinMode(dallasVccPin, OUTPUT);
    digitalWrite(dallasVccPin, HIGH);
    sensors.begin();

    // Locate the sensor.
    while (!sensors.getAddress(dallasAddress, 0)) {
        Serial.println("Locating the sensor...");
        delay(1000);
    }
    sensors.setResolution(dallasAddress, 12, true);
}

bool disableCooldown() {
    isCooldownEnabled = false;
    // Putting the pin to hi-Z state allowing the thermostat to do its job.
    pinMode(cooldownPin, INPUT);
}

void setup() {
    disableCooldown();
    setupSerial();
    setupDallas();

    Serial.println("Started up.");
}

void enableCooldown() {
    isCooldownEnabled = true;
    // Close the relay transistor effectively turning off heating.
    pinMode(cooldownPin, OUTPUT);
    digitalWrite(cooldownPin, LOW);
}

void printCurrentState(const float temperature) {
    Serial.print(millis());
    Serial.print(" ms | ");
    Serial.print(temperature);
    Serial.print(" °C | ");
    if (!isCooldownEnabled) {
        // Since there's the 2kOhm resistor in front of the pin,
        // I'm using a small "magic" threshold number.
        Serial.println(analogRead(cooldownPin) > 10 ? "ON" : "OFF");
    } else {
        Serial.println("COOLDOWN");
    }
}

void loop() {
    sensors.requestTemperatures();
    const float temperature = sensors.getTempC(dallasAddress);

    if (!isCooldownEnabled && (temperature > enableCooldownTemp)) {
        // Cooldown is disabled, but the floor temperature is too high.
        enableCooldown();
    } else if (isCooldownEnabled && (temperature < disableCooldownTemp)) {
        // Cooldown is enabled, but the floor is already cooled down.
        disableCooldown();
    }

    printCurrentState(temperature);
}
