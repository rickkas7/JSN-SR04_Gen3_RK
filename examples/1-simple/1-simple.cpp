#include "JSN-SR04_Gen3_RK.h"


SerialLogHandler logHandler;

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

JSN_SR04_Gen3 distanceSensor;

void distanceCallback(JSN_SR04_Gen3::DistanceResult result) {
    Log.info("status=%d cm=%lf inch=%lf", result.status, result.cm(), result.inch());
}

void setup() {

    distanceSensor
        .withTrigPin(D3)
        .withEchoPin(D4)
        .withUnusedPins(A0, A1)
        .withCallback(distanceCallback)
        .withSamplePeriodic(500ms)
        .setup();

    Particle.connect();
}

void loop() {
    distanceSensor.loop();
}
