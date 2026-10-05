#ifndef CRANE_CONTROLLER_H
#define CRANE_CONTROLLER_H

#include <Arduino.h>
#include <Servo.h>

enum class CraneCommand : uint8_t {
    DisableAllMovement = 0x00,
    MoveToPosition1 = 0x01,
    MoveToPosition2 = 0x02,
    MoveToPosition3 = 0x03,
    Home = 0x04
};

enum class CraneStatus : uint8_t {
    DisableAllMovement = 0x00,
    Position1 = 0x01,
    Position2 = 0x02,
    Position3 = 0x03,
    Home = 0x04,
    Moving = 0x05,
    NotInitialised = 0x06,
    ErrorCondition = 0x07
};

enum class CraneTargetPosition : uint8_t {
    Position1 = 0x01,
    Position2 = 0x02,
    Position3 = 0x03,
    Home = 0x00
};

struct GpioConfig {
    uint8_t upperLimitSwitch;
    uint8_t keyPhasor;
    uint8_t winchServo;
    uint8_t lights;
    uint8_t servoPower;
    uint8_t i2cAddress;

    // Create a default GPIO configuration for the crane controller.
    GpioConfig()
        : upperLimitSwitch(4),
          keyPhasor(3),
          winchServo(2),
          lights(6),
          servoPower(5),
          i2cAddress(0x40) {
    }

    GpioConfig(uint8_t upperLimitSwitchPin,
               uint8_t keyPhasorPin,
               uint8_t winchServoPin,
               uint8_t lightsPin,
               uint8_t servoPowerPin,
               uint8_t i2cAddressValue)
        : upperLimitSwitch(upperLimitSwitchPin),
          keyPhasor(keyPhasorPin),
          winchServo(winchServoPin),
          lights(lightsPin),
          servoPower(servoPowerPin),
          i2cAddress(i2cAddressValue) {
    }
};

class CraneController {
public:
    CraneController();
    explicit CraneController(const GpioConfig& gpioConfig);

    void begin();
    void update();

    void setCommand(CraneCommand command);
    CraneCommand command() const;
    uint8_t status() const;

    void setPulseCount(int32_t pulses);
    int32_t pulseCount() const;

    void setTargetPosition(CraneTargetPosition targetPosition);
    CraneStatus evaluateStatus() const;

    void handleI2CReceive(uint8_t* data, int length);
    uint8_t handleI2CRequest() const;

private:
    static CraneController* interruptInstance_;
    static constexpr int32_t kTargetPulseCountUpperLimit = 0;
    static constexpr int32_t kPosition1PulseCount = 10;
    static constexpr int32_t kPosition2PulseCount = 30;
    static constexpr int32_t kPosition3PulseCount = 43;
    static constexpr int32_t kMaxPulseCount = 300;
    static constexpr int32_t ktargetKeyPhasorGap =1400;
    static constexpr uint16_t kServoStopUs = 1515;
    static constexpr uint16_t kServoLowerUs = 1551;
    static constexpr uint16_t kServoRaiseUs = 1489;
    static constexpr uint32_t kBlinkPeriodMs = 5000U;
    static constexpr uint32_t kHighDutyMs = 250U;

    GpioConfig gpioConfig_;
    Servo servo_;

    CraneCommand command_;
    CraneStatus status_;
    CraneTargetPosition targetPosition_;
    int32_t pulseCount_;
    int32_t targetPulseCount_;
    int32_t maxPulseCount_;
    long durationSinceLastPulseMs_;
    bool initialised_;
    bool errorCondition_;
    bool servoEnabled_;
    uint32_t lastLightToggleMs_;
    uint32_t lastPulseTimestampMs_;
    int32_t servoDemandUs_;

    void configurePins();
    void setStatus(CraneStatus status);
    void stopWinch();
    void setServoPower(bool enabled);
    void setLights(bool active);
    void updateLights();
    void updateDebugLed();
    void initialisePowerUp();
    void moveToDemand();
    void resolveDemandFromCommand();
    static void keyPhasorIsr();
    void recordPulse();
    bool upperLimitSwitchReached() const;
    bool commandDisablesMovement() const;
};

#endif
