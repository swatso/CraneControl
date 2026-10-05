#include <Arduino.h>
#include <Wire.h>

#include "CraneController.h"

// Instantiate the crane controller with the specified GPIO configuration.
namespace {
/*const GpioConfig kCraneGpioConfig = GpioConfig{
    4U,
    3U,
    2U,
    6U,
    5U,
    0x40U
};
*/
const GpioConfig kCraneGpioConfig = GpioConfig();

CraneController craneController(kCraneGpioConfig);

void onI2cReceive(int bytes) {
    uint8_t buffer[8] = {0};
    int index = 0;

    while (Wire.available() && index < 8) {
        buffer[index++] = static_cast<uint8_t>(Wire.read());
    }

    craneController.handleI2CReceive(buffer, index);
    (void)bytes;
}

void onI2cRequest() {
    Wire.write(craneController.handleI2CRequest());
}
}  // namespace

void setup() {
    Serial.begin(115200);
    Wire.begin(kCraneGpioConfig.i2cAddress);
    Wire.onReceive(onI2cReceive);
    Wire.onRequest(onI2cRequest);
    Serial.println("Wire interface configured.");
    craneController.begin();
    Serial.println("Setup complete.");

    craneController.setCommand(CraneCommand::MoveToPosition1);

}

void loop() {
    craneController.update();

    /*
    if (craneController.evaluateStatus() == CraneStatus::Position1) {
        Serial.println("Crane has reached Position 1.");
        craneController.setCommand(CraneCommand::MoveToPosition2);
    }
    if (craneController.evaluateStatus() == CraneStatus::Position2) {
        Serial.println("Crane has reached Position 2.");
        craneController.setCommand(CraneCommand::MoveToPosition3);
    }
    if (craneController.evaluateStatus() == CraneStatus::Position3) {
        Serial.println("Crane has reached Position 3.");
        craneController.setCommand(CraneCommand::Home);
    }
    if (craneController.evaluateStatus() == CraneStatus::Home) {
        Serial.println("Crane has reached the Upper Limit.");
        craneController.setCommand(CraneCommand::MoveToPosition1);
    }

    delay(1000);
    */
}
