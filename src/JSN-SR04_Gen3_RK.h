#ifndef __JSN_SR04_GEN3_RK_H
#define __JSN_SR04_GEN3_RK_H

#include "Particle.h"

// Repository: https://github.com/rickkas7/JSN-SR04_Gen3_RK
// License: MIT


class JSN_SR04_Gen3 {
public:
    class DistanceResult {
    public:
        enum class Status {
            SUCCESS = 0,		//!< Success (including valid checksum)
            ERROR,				//!< An internal error (problem with the I2S peripheral, etc.)
            TOO_MANY_RETRIES,	//!< After the specified number of retries, could not get a valid result
            BUSY				//!< Called getSample() before the previous call completed            
        };

        Status getStatus() const { return status; };
        double getDistanceM() const { return distanceM; };

        double cm() const { return distanceM * 100.0; };
        double mm() const { return distanceM * 1000.0; };

        double inch() const { return distanceM * 39.3701; };


        Status status = Status::ERROR;
        double distanceM = 0.0;
    };

    JSN_SR04_Gen3();
    virtual ~JSN_SR04_Gen3();

    JSN_SR04_Gen3 &withTrigPin(pin_t trigPin) { this->trigPin = trigPin; return *this; };
    JSN_SR04_Gen3 &withEchoPin(pin_t echoPin) { this->echoPin = echoPin; return *this; };
    JSN_SR04_Gen3 &withUnusedPins(pin_t unusedPin1, pin_t unusedPin2) { this->unusedPin1 = unusedPin1; this->unusedPin2 = unusedPin2; return *this; };

    JSN_SR04_Gen3 &withMaxLengthMeters(float maxLengthM) { this->maxLengthM = maxLengthM; return *this; };

    JSN_SR04_Gen3 &withCallback(std::function<void(DistanceResult)> callback) { this->callback = callback; return *this; };

    JSN_SR04_Gen3 &withSamplePeriodic(std::chrono::milliseconds period) { samplePeriodic = period.count(); return *this; };
    JSN_SR04_Gen3 &withSamplePeriodicMs(unsigned long periodMs) { samplePeriodic = periodMs; return *this; };

    bool setup();

    void loop();

    bool sampleOnce();


    DistanceResult getLastResult() const { return lastResult; };

    void setResult(DistanceResult::Status status, double distanceM = 0.0);

    static int countOneBits(uint16_t sample);

protected:
    void idleState();
    void startState();
    void sampleState();

    pin_t trigPin = PIN_INVALID;
    pin_t echoPin = PIN_INVALID;
    pin_t unusedPin1 = PIN_INVALID;
    pin_t unusedPin2 = PIN_INVALID;
    float maxLengthM = 2.0;
    std::function<void(DistanceResult)> callback = nullptr;
    size_t numSamples = 0;
    uint16_t *rxBuffer = nullptr;
    uint16_t *txBuffer = nullptr;
    bool isIdle = true;
    DistanceResult lastResult;
    unsigned long samplePeriodic = 0;

    size_t leadingOverhead = 152; // Must be a multiple of 4
    unsigned long safetyTimeoutMs = 30;

    unsigned long sampleTime = 0;
    std::function<void(JSN_SR04_Gen3&)> stateHandler = &JSN_SR04_Gen3::idleState;
};

#endif /* __JSR_SR04_GEN3_RK_H */
