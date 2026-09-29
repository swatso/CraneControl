#include "CraneController.h"

CraneController* CraneController::interruptInstance_ = nullptr;

CraneController::CraneController()
    : gpioConfig_(),
      servo_(),
      command_(CraneCommand::DisableAllMovement),
      status_(CraneStatus::NotInitialised),
    targetPosition_(CraneTargetPosition::Home),
      targetPulseCount_(0),
      pulseCount_(0),
      maxPulseCount_(0),
      initialised_(false),
      errorCondition_(false),
      servoEnabled_(false),
      lastLightToggleMs_(0),
      lastPulseTimestampMs_(0) {
}

CraneController::CraneController(const GpioConfig& gpioConfig)
    : gpioConfig_(gpioConfig),
      servo_(),
      command_(CraneCommand::DisableAllMovement),
      status_(CraneStatus::NotInitialised),
    targetPosition_(CraneTargetPosition::Home),
      targetPulseCount_(0),
      pulseCount_(0),
      maxPulseCount_(0),
      initialised_(false),
      errorCondition_(false),
      servoEnabled_(false),
      lastLightToggleMs_(0),
      lastPulseTimestampMs_(0) {
}

void CraneController::begin() {
    interruptInstance_ = this;
    configurePins();
    command_ = CraneCommand::DisableAllMovement;
    status_ = CraneStatus::NotInitialised;
    pulseCount_ = 0;
    targetPosition_ = CraneTargetPosition::Home;
    targetPulseCount_ = 0;
    maxPulseCount_ = 0;
    errorCondition_ = false;
    initialised_ = false;
    lastLightToggleMs_ = millis();
}

void CraneController::update() {

    if (!initialised_) {
Serial.println("update::initialising power-up");
        initialisePowerUp();
        return;
    }

    if (errorCondition_) {
        updateLights();
        stopWinch();
        return;
    }

    if (command_ == CraneCommand::DisableAllMovement) {
        stopWinch();
        setStatus(CraneStatus::DisableAllMovement);
        updateLights();
        return;
    }

    if (upperLimitSwitchReached()) {
        pulseCount_ = 0;
        targetPulseCount_ = kTargetPulseCountUpperLimit;
        setStatus(CraneStatus::Home);
    }

    moveToDemand();
    updateLights();
}

void CraneController::setCommand(CraneCommand command) {
    command_ = command;
    resolveDemandFromCommand();
}

CraneCommand CraneController::command() const {
    return command_;
}

uint8_t CraneController::status() const {
    return static_cast<uint8_t>(status_);
}

// I think this can be removed....
void CraneController::setPulseCount(int32_t pulses) {
    pulseCount_ = constrain(pulses, 0, kMaxPulseCount);
    if (pulseCount_ <= 0) {
        pulseCount_ = 0;
        targetPosition_ = CraneTargetPosition::Home;
    }
}

int32_t CraneController::pulseCount() const {
    return pulseCount_;
}

void CraneController::setTargetPosition(CraneTargetPosition targetPosition) {
    targetPosition_ = targetPosition;
    switch (targetPosition_) {
    case CraneTargetPosition::Position1:
        targetPulseCount_ = kPosition1PulseCount;
        break;
    case CraneTargetPosition::Position2:
        targetPulseCount_ = kPosition2PulseCount;
        break;
    case CraneTargetPosition::Position3:
        targetPulseCount_ = kPosition3PulseCount;
        break;
    case CraneTargetPosition::Home:
        targetPulseCount_ = kTargetPulseCountUpperLimit;
        break;
    default:
        targetPulseCount_ = pulseCount_;        // If you dont recognise the target position then do nothing
        break;
    }
}

CraneStatus CraneController::evaluateStatus() const {
    if (errorCondition_) {
        return CraneStatus::ErrorCondition;
    }

    if (pulseCount_== kTargetPulseCountUpperLimit) {
        return CraneStatus::Home;
    }

    if (pulseCount_ == kPosition3PulseCount) {
        return CraneStatus::Position3;
    }

    if (pulseCount_ == kPosition2PulseCount) {
        return CraneStatus::Position2;
    }

    if (pulseCount_ == kPosition1PulseCount) {
        return CraneStatus::Position1;
    }

    return CraneStatus::Moving;
}

void CraneController::handleI2CReceive(uint8_t* data, int length) {
    if (length <= 0 || data == nullptr) {
        return;
    }

Serial.print("Received I2C data: ");
for (int i = 0; i < length; ++i) {
    Serial.print(data[i], HEX);
    Serial.print(" ");
}
Serial.println();

    switch (static_cast<CraneCommand>(data[0])) {
    case CraneCommand::DisableAllMovement:
    case CraneCommand::MoveToPosition1:
    case CraneCommand::MoveToPosition2:
    case CraneCommand::MoveToPosition3:
    case CraneCommand::Home:
        setCommand(static_cast<CraneCommand>(data[0]));
        break;
    default:
        setCommand(CraneCommand::DisableAllMovement);
        break;
    }
}

uint8_t CraneController::handleI2CRequest() const {
    return static_cast<uint8_t>(status_);
}

void CraneController::configurePins() {
    pinMode(gpioConfig_.upperLimitSwitch, INPUT_PULLUP);
    pinMode(gpioConfig_.keyPhasor, INPUT_PULLUP);
    pinMode(gpioConfig_.lights, OUTPUT);
    pinMode(gpioConfig_.servoPower, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(gpioConfig_.keyPhasor), keyPhasorIsr, FALLING);
    digitalWrite(gpioConfig_.servoPower, LOW);
    digitalWrite(gpioConfig_.lights, HIGH);

    servo_.attach(gpioConfig_.winchServo);
    servo_.writeMicroseconds(kServoStopUs);
    servoEnabled_ = false;
    lastLightToggleMs_ = millis();
}

void CraneController::setStatus(CraneStatus status) {
    status_ = status;
}

void CraneController::keyPhasorIsr() {
    if (interruptInstance_ != nullptr) {
        interruptInstance_->recordPulse();
    }
}

void CraneController::stopWinch() {
    servoDemandUs_ = kServoStopUs;
    servo_.writeMicroseconds(servoDemandUs_);
    servoEnabled_ = false;
}

void CraneController::setServoPower(bool enabled) {
    servoEnabled_ = enabled;
    digitalWrite(gpioConfig_.servoPower, enabled ? HIGH : LOW);
    if (!enabled) {
        stopWinch();
    }
}

void CraneController::setLights(bool active) {
    digitalWrite(gpioConfig_.lights, active ? LOW : HIGH);
}

void CraneController::updateLights() {
    if (errorCondition_) {
        const bool errorPattern = ((millis() / 250U) % 2U) == 0U;
        setLights(errorPattern);
        return;
    }

    if (!initialised_) {
        setLights(false);
        return;
    }

    const uint32_t patternTime = millis() % kBlinkPeriodMs;
    const bool highDuty = patternTime < kHighDutyMs;
    setLights(highDuty);
}

void CraneController::initialisePowerUp() {
    setStatus(CraneStatus::NotInitialised);
    setServoPower(true);
    stopWinch();
    setLights(false);

    if (upperLimitSwitchReached()) {
        pulseCount_ = 0;
Serial.println("Initialised at upper limit");
        setStatus(CraneStatus::Home);
        targetPosition_ = CraneTargetPosition::Home;
        initialised_ = true;
        return;
    }

    // winch is not at home position, move it upwards until the upper limit switch is reached
    unsigned long startTime = millis();
    int pulseSnapShot = pulseCount_;
    servoDemandUs_ = kServoRaiseUs;

    while(!upperLimitSwitchReached()) {
        servo_.writeMicroseconds(servoDemandUs_);
        if(pulseCount_ == pulseSnapShot) {
            if (millis() - startTime >= 2000U) {
                Serial.println("Error: Motor timeout during power-up initialisation");
                break;
            }
        }
        startTime = millis();
        delay(1);
    }
    Serial.println("Winch initialised to upper limit");
    stopWinch();

    if (!upperLimitSwitchReached()) {
        errorCondition_ = true;
        Serial.println("Error: Failed to reach upper limit during power-up initialisation");
        stopWinch();
        setStatus(CraneStatus::ErrorCondition);
        initialised_ = true;
        return;
    }

    pulseCount_ = 0;
    setStatus(CraneStatus::Home);
    targetPosition_ = CraneTargetPosition::Home;
    initialised_ = true;
}

void CraneController::moveToDemand() {
    switch (command_) {
    case CraneCommand::Home:
        // moves the crane to the home position (upper limit) and relies on the upper limit switch to establish the reference position
        targetPosition_ = CraneTargetPosition::Home;
        targetPulseCount_ = kTargetPulseCountUpperLimit;
        if (upperLimitSwitchReached()) {
            pulseCount_ = 0;
            stopWinch();
            setStatus(CraneStatus::Home);
            return;
        }
        servo_.writeMicroseconds(servoDemandUs_);
        setStatus(CraneStatus::Moving);
        break;

    case CraneCommand::MoveToPosition1:
        targetPosition_ = CraneTargetPosition::Position1;
        targetPulseCount_ = kPosition1PulseCount;
        if (pulseCount_ == kPosition1PulseCount) {
            stopWinch();
            setStatus(CraneStatus::Position1);
            return;
        }
        servo_.writeMicroseconds(servoDemandUs_);
        setStatus(CraneStatus::Moving);
        break;

    case CraneCommand::MoveToPosition2:
        targetPosition_ = CraneTargetPosition::Position2;
        targetPulseCount_ = kPosition2PulseCount;
        if (pulseCount_ == kPosition2PulseCount) {
            stopWinch();
            setStatus(CraneStatus::Position2);
            return;
        }
        servo_.writeMicroseconds(servoDemandUs_);
        setStatus(CraneStatus::Moving);
        break;

    case CraneCommand::MoveToPosition3:
        targetPosition_ = CraneTargetPosition::Position3;
        targetPulseCount_ = kPosition3PulseCount;
        if (pulseCount_ == kPosition3PulseCount) {
            stopWinch();
            setStatus(CraneStatus::Position3);
            return;
        }
        servo_.writeMicroseconds(servoDemandUs_);
        setStatus(CraneStatus::Moving);
        break;
    default:
        stopWinch();
        setStatus(CraneStatus::DisableAllMovement);
        break;
    }
}

void CraneController::resolveDemandFromCommand() {
    switch (command_) {
    case CraneCommand::DisableAllMovement:
        targetPosition_ = CraneTargetPosition::Home;
        stopWinch();
        setStatus(CraneStatus::DisableAllMovement);
        break;

    case CraneCommand::MoveToPosition1:
        targetPosition_ = CraneTargetPosition::Position1;
        setStatus(CraneStatus::Moving);
        if(pulseCount_ < kPosition1PulseCount) servoDemandUs_ = kServoLowerUs;
        else servoDemandUs_ = kServoRaiseUs;
        break;

    case CraneCommand::MoveToPosition2:
        targetPosition_ = CraneTargetPosition::Position2;
        setStatus(CraneStatus::Moving);
        if(pulseCount_ < kPosition2PulseCount) servoDemandUs_ = kServoLowerUs;
        else servoDemandUs_ = kServoRaiseUs;
        break;

    case CraneCommand::MoveToPosition3:
        targetPosition_ = CraneTargetPosition::Position3;
        setStatus(CraneStatus::Moving);
        if(pulseCount_ < kPosition3PulseCount) servoDemandUs_ = kServoLowerUs;
        else servoDemandUs_ = kServoRaiseUs;
        break;

    case CraneCommand::Home:
        targetPosition_ = CraneTargetPosition::Home;
        setStatus(CraneStatus::Moving);
        servoDemandUs_ = kServoRaiseUs;
        break;

    default:
        targetPosition_ = CraneTargetPosition::Home;
        setStatus(CraneStatus::DisableAllMovement);
        break;
    }
}

void CraneController::recordPulse() {
    if (millis() - lastPulseTimestampMs_ < 10U) {
        return;
    }
    durationSinceLastPulseMs_ = abs( millis() - lastPulseTimestampMs_);
    if(targetPulseCount_ > pulseCount_) {
        // Heading downwards to target position
        pulseCount_ = constrain(pulseCount_ + 1, 0, kMaxPulseCount);
        if (durationSinceLastPulseMs_ > ktargetKeyPhasorGap) {
            // turning too slowly
            servoDemandUs_++;
        }
        else
        {
            // turning too fast
            servoDemandUs_--;
        }
    }
    else{
        // Heading upwards to target position
        pulseCount_ = constrain(pulseCount_ - 1, 0, kMaxPulseCount);
        if (durationSinceLastPulseMs_ > ktargetKeyPhasorGap) {
            // turning too slowly
            servoDemandUs_--;
        }
        else
        {
            // rising too fast
            servoDemandUs_++;
        }
    }
    lastPulseTimestampMs_ = millis();
    Serial.println(servoDemandUs_);
}

bool CraneController::upperLimitSwitchReached() const {
    return digitalRead(gpioConfig_.upperLimitSwitch) == LOW;
}

bool CraneController::commandDisablesMovement() const {
    return command_ == CraneCommand::DisableAllMovement;
}
