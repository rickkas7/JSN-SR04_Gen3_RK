# JSN-SR04_Gen3_RK

*Library for JSN-SR04 ultrasonic distance sensor for Particle Gen 3 devices (Argon, Boron, B Series SoM, Tracker)*

The JSN-SR04 is an inexpensive ultrasonic distance sensor. There are many manufacturers, some are weather resistant, and they have somewhat different supported ranges, but usually in the range of a few centimeters to maybe a few meters. Also note: these sensors are generally not all that reliable. I would not use one in a life-safety situation and be sure to code defensively for the case when it fails.

Once characteristic of the sensor is that you need to measure the pulse width to within a microsecond or so, and measurement takes around 150 microseconds of setup time and up to 29 milliseconds of pulse width (at 5 meters). At a more reasonable 1 meter, it's still around 9 milliseconds.

The problem on Gen 3 devices (nRF52840) is that even if you use interrupts, the interrupt latency is high, and extremely variable. The radio stack runs at a high interrupt priority and will delay other interrupts until completion. Because of the long time to sample, you don't want to disable interrupts because it will adversely affect the BLE radio, and the reset of the system. But if you do not disable interrupts, the interrupt latency makes for very inaccurate readings.

The solution is the technique used in this library: The nRF52840 I2S, note this is I2S, as in sound, not I2C, peripheral is really just a high-speed digital input and output device with hardware DMA. The library is configured to use 32,000 samples per second, with 16 bits per sample, stereo. This works out to a bit clock of 1 MHz, or 1 µs per sample bit, which is perfect for the JSN-SR04.

The library uses the I2S peripheral to both generate the TRIG pulse and measure the resulting ECHO pulse. It does so without ever disabling interrupts, and the interrupt occurs after the entire maximum measurement is complete. A deferred interrupt does not affect the accuracy of the sensor!


### Level shifter required!

One caveat is the the JSN-SR04 sensors generally require 5V to operate properly.

Particle Gen 3 devices are not 5V tolerant because the nRF52840 MCU is not! Be sure to use a level shifter on the ECHO output from the JSN-SR04 to the GPIO on the Particle device, or you will likely permanently damage the pin on the Particle device.

Also note that since you need 5V you will not be able to power the sensor off the LiPo battery unless you use an external boost converter. If you are powered by USB, you can power the sensor from the VUSB bin.

Most JSN-SR04 sensors will trigger if TRIG only goes up to 3.3V so you generally don't need a level shifter on TRIG.

If you're looking for an inexpensive breadboard-friendly DIP package, the [74AHCT125](https://www.adafruit.com/product/1787) can be used. It's overkill in that it's a quad level-shifter, but it works. Connect VCC on the chip to 3V3 because you're using it to reduce the voltage!

For custom boards I often use a [Texas Instruments SN74LVC1T45](https://www.digikey.com/product-detail/en/texas-instruments/SN74LVC1T45QDCKRQ1/296-39212-1-ND/5143211) which comes in a tiny SC70-7 SMD package and is even less expensive.


### Lots of GPIO required

Another caveat is that this library requires a lot of GPIO: four GPIO.

Of course there are the two standard JSN-SR04 pins, TRIG (output) and ECHO (input).

The unfortunate thing is that you also need to dedicate two other pins, unusedPin1 and unusedPin2. These must not be the same pin, and can't be used for anything else for all practical purposes. This is due to how the I2S peripheral works. You have to assign the I2S LRCK and SCK to pins or the nRF52 I2S peripheral won't initialize. You won't need these outputs for anything, but they do need to be assigned real GPIOs. The signals happen to be 32 kHz for the LRCK and 1 MHz for SCK while getting a distance sample. 

### Memory requirements

The DMA buffers require 2,144 of RAM at the default maximum measurement distance of 1 meter. The library can be configured for larger or smaller distances, which affect both RAM usage and the amount of time to measure a distance.

## Modes of operation

The library essentially only works with one sensor. You could reinitialize it on different pins for multiple sensors, but you can't measure more than one sensor at the same time because the I2S peripheral has a single bit digital capture.


### Periodic with callback

The recommended mode of operation is to assign a callback function, and set the sensor to periodically sample. The minimum period is around 10 milliseconds with a maximum distance of 1 meter. Ideally you should set to to be 300 milliseconds or larger to account for timeout situations.

The callback is called with a DistanceResult object that contains the distance which you can retrieve in meters, centimeters, or inches. It will also include a status code, such as SUCCESS, RANGE_ERROR (too close or too far), BUSY, or other errors.

See the example examples/1-simple for how to use this mode.

### Single sample with callback

If you only need a single sample, you can do that as well.

### Single sample synchronous

This is not recommended, because it will block the current thread for up to the safety timer period, which is 300 milliseconds. Using the callback or polling is preferable. 

### Alarm mode

Alarm mode calls the callback when the alarm condition is entered or exited. The alarm condition is a distance, along with an less than or greater than test, and a hysteresis value. It can be used to alert when you are too close or too far away from the thing being measured.

If you want to measure for change in distance, use the periodic measurement mode directly instead.

See the example examples/2-alarm for how to use this mode.

## Usage

- Include the library, such as by using `Particle: Install Library` in Particle Workbench, or by adding the **JSN-SR04_Gen3_RK** library in the Web IDE.

- Add the necessary header file:

```cpp
#include "JSN-SR04_Gen3_RK.h"
```

- Declare one of these as a global variable to manage the JSN-SR04 sensor:

```cpp
JSN_SR04_Gen3 distanceSensor;
```

- In setup(), initialize the sensor pins and settings

```cpp
void setup() {

    // Initialize the sensor configuration from setup()
    distanceSensor
        .withTrigPin(D3)
        .withEchoPin(D4)
        .withUnusedPins(A0, A1)
        .setup();
}
```

- Be sure to call the loop() method from loop()!

```cpp
void loop() {
    // You must call this frequently from loop(), preferable on every execution
    distanceSensor.loop();
}
```

- If you are using a callback (recommended), the function might look something like this:

```cpp
void distanceCallback(JSN_SR04_Gen3::DistanceResult result) {
    switch(result.status) {
        case JSN_SR04_Gen3::DistanceResult::Status::SUCCESS:
            Log.info("cm=%lf inch=%lf", result.cm(), result.inch());
            break;

        case JSN_SR04_Gen3::DistanceResult::Status::RANGE_ERROR:
            Log.info("distance range error");
            break;

        default:
            Log.info("distance error %d", result.status);
            break;
    }
}
```

## JSN_SR04_Gen3 API

The full browseable API can be found [online](https://rickkas7.github.io/JSN-SR04_Gen3_RK/) or in the docs folder docs/index.html offline.

Class for a JSN-SR04 ultrasonic distance sensor.

Note: You can effectively only have one instance of this class per device, because there is only one I2S peripheral, which is what is used to implement the device driver.


---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withTrigPin(pin_t trigPin) 

Specifies the TRIG pin (OUTPUT)

```
JSN_SR04_Gen3 & withTrigPin(pin_t trigPin)
```

#### Parameters
* `trigPin` A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc.

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withEchoPin(pin_t echoPin) 

Specifies the ECHO pin (INPUT)

```
JSN_SR04_Gen3 & withEchoPin(pin_t echoPin)
```

#### Parameters
* `echoPin` A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc.

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

The ECHO pin is typically a 5V logic level. You MUST add a level shifter before connecting it to a Particle Gen 3 device GPIO because GPIO are not 5V tolerant on the nRF52!

The ECHO pin on a JSN-SR04 is driven by a push-pull driver so you can only connect a single sensor to a single GPIO.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withUnusedPins(pin_t unusedPin1, pin_t unusedPin2) 

You must specify two GPIO that are not otherwise used that will be used as outputs by this library.

```
JSN_SR04_Gen3 & withUnusedPins(pin_t unusedPin1, pin_t unusedPin2)
```

#### Parameters
* `unusedPin1` A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc. 

* `unusedPin2` A pin, such as D2 or A0, or another port that is not being used, such as TX, RX, DAC, etc. Must be different than unusedPin1. 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

You need to dedicate two other pins, unusedPin1 and unusedPin2. These must not be the same pin, and can't be used for anything else for all practical purposes. This is due to how the I2S peripheral works. You have to assign the I2S LRCK and SCK to pins or the nRF52 I2S peripheral won't initialize. You won't need these outputs for anything, but they do need to be assigned GPIOs. The signals happen to be 32 kHz for the LRCK and 1 MHz for SCK while getting a distance sample.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withMaxLengthMeters(float maxLengthM) 

The maximum length that can be measured in meters. Default: 1 meter.

```
JSN_SR04_Gen3 & withMaxLengthMeters(float maxLengthM)
```

#### Parameters
* `maxLengthM` Distance in meters 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

This affects the amount of memory that is used, and also the amount of time a sampling takes. See the README for more information.

At the default is 1 meter, the memory is 2,080 bytes and the time is 9 milliseconds.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withCallback(std::function< void(DistanceResult)> callback) 

Specify a callback function to be called on samples, errors, and alarm conditions.

```
JSN_SR04_Gen3 & withCallback(std::function< void(DistanceResult)> callback)
```

#### Parameters
* `callback` The callback function 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

The callback function has the prototype;

void callback(DistanceResult distanceResult)

It can be a C++11 lambda, if desired, to call a class member function.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withSamplePeriodic(std::chrono::milliseconds period) 

Enabling periodic sampling mode.

```
JSN_SR04_Gen3 & withSamplePeriodic(std::chrono::milliseconds period)
```

#### Parameters
* `period` The sampling period as a chrono literal, such as 500ms, 10s, etc. 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

It's recommended to specify a sampling period greater than safetyTimeoutMs milliseconds (currently 300). However, in practice you can specify a much faster sampling period, as low as getSampleTimeMs() milliseconds. The latter varies depending on the max length meters. At the default value of 1 meter, you can use a periodic sample rate at low as 10 milliseconds, however you might not get every sample. The sensor may not always reset in time an the BUSY error will be called on the callback.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withSamplePeriodicMs(unsigned long periodMs) 

Enabling periodic sampling mode.

```
JSN_SR04_Gen3 & withSamplePeriodicMs(unsigned long periodMs)
```

#### Parameters
* `periodMs` The sampling period in milliseconds 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

It's recommended to specify a sampling period greater than safetyTimeoutMs milliseconds (currently 300). However, in practice you can specify a much faster sampling period, as low as getSampleTimeMs() milliseconds. The latter varies depending on the max length meters. At the default value of 1 meter, you can use a periodic sample rate at low as 10 milliseconds, however you might not get every sample. The sensor may not always reset in time an the BUSY error will be called on the callback.

---

### JSN_SR04_Gen3 & JSN_SR04_Gen3::withDistanceAlarm(DistanceAlarm distanceAlarm) 

Enable distance alarm mode.

```
JSN_SR04_Gen3 & withDistanceAlarm(DistanceAlarm distanceAlarm)
```

#### Parameters
* `distanceAlarm` The distance alarm configuration 

#### Returns
JSN_SR04_Gen3& This object, for chaining options, fluent-style

---

### bool JSN_SR04_Gen3::setup() 

You typically call this from setup() after setting all of the configuration parameters using the withXXX() methods.

```
bool setup()
```

You must call setup() before any samples can be taken.

---

### void JSN_SR04_Gen3::loop() 

You must call this from loop()

```
void loop()
```

---

### bool JSN_SR04_Gen3::sampleOnce() 

Get a sample from the sensor.

```
bool sampleOnce()
```

#### Returns
true 

#### Returns
false

You still must configure the pins and call the setup() method. You must also call the loop() method from loop().

---

### DistanceResult JSN_SR04_Gen3::sampleOnceSync() 

Synchronous version of sampleOnce - not recommended.

```
DistanceResult sampleOnceSync()
```

#### Returns
DistanceResult

Using the asynchronous callback is preferable, but this call is provided to make it somewhat easier to convert code from other libraries.

---

### DistanceResult JSN_SR04_Gen3::getLastResult() const 

Gets the last result code.

```
DistanceResult getLastResult() const
```

#### Returns
DistanceResult

Normally you should use the callback, but if you want to poll instead of using a callback you can use this function for periodic, sample once, and alarm modes.

---

### unsigned long JSN_SR04_Gen3::getSampleTimeMs() const 

Returns the number of milliseconds it takes to process a sample.

```
unsigned long getSampleTimeMs() const
```

#### Returns
unsigned long number of milliseconds

In practice it might take a few milliseconds longer because of the delays in dispatching loop(). The value is calculated from the maxLengthM and leadingOverhead values.

With the default value of 1 meter maximum and 152 leadingOverhead, this returns 9 milliseconds.

In theory you could sample at around every 9 milliseconds, maybe 10, but it's probably best to limit it to 100 milliseconds, or even 500 milliseconds, to be safe. If you sample frequently, be sure to handle the case where BUSY status is returned. This means that that sensor has not yet reset the ECHO output low and a sample cannot be taken yet.

Note that the callback will not be called for at least this long, regardless of distance! The reason is that the sample buffer is not processed until the DMA engine stops writing to the entire buffer, and then it waits until in loop context again.

## JSN_SR04_Gen3::DistanceResult API

```
class JSN_SR04_Gen3::DistanceResult
  : public JSN_SR04_Gen3::Distance
```  

Structure passed to the callback when the distance has been retrieved.

This includes a Status enum for the result status, and optionally a distance as this class is derived from class Distance. Thus you can use the inherited methods such as cm(), mm(), and inch() to get the distance in centimeters, millimeters, or inches, for example.

### Status status 

Current status value.

```
Status status
```

---

### Status JSN_SR04_Gen3::DistanceResult::getStatus() const 

Get the Status value for this result.

```
Status getStatus() const
```

#### Returns
Status

---

### bool JSN_SR04_Gen3::DistanceResult::success() const 

Helper function to return true if the Status is SUCCESS.

```
bool success() const
```

#### Returns
true 

#### Returns
false

---

### enum Status 

Status of the call.

```
enum Status
```

 Values                         | Descriptions                                
--------------------------------|---------------------------------------------
SUCCESS            | Success, got a valid looking measurement.
ERROR            | An internal error (problem with the I2S peripheral, etc.)
RANGE_ERROR            | Too close or too far away to detect.
BUSY            | Called before the previous call completed.
IN_PROGRESS            | Call is in progress (getting sample from sensor)
ENTER_ALARM            | When using distance alarm, entering alarm state.
EXIT_ALARM            | When using distance alarm, exiting alarm state.


# class JSN_SR04_Gen3::Distance 

Utility class for holding a distance.

The storage value is in meters, but there are accessors for cm, mm, and inches. This is used for both getting distances (the sensor value) as well as setting distances (for distance alarm mode).

The JSN_SR04_Gen3::DistanceResult class is derived from this class, so the methods here can be used with a DistanceResult that is passed to the callback.

### double distanceM 

The value of the distance in meters.

```
double distanceM
```

---

###  JSN_SR04_Gen3::Distance::Distance() 

Construct a new Distance object with a distance of 0.

```
 Distance()
```

---

###  JSN_SR04_Gen3::Distance::Distance(double valueM) 

Construct a new Distance object with a distance in meters.

```
 Distance(double valueM)
```

#### Parameters
* `valueM` distance in meters (double floating point)

---

###  JSN_SR04_Gen3::Distance::Distance(const Distance & value) 

Construct a new Distance object from another Distance object.

```
 Distance(const Distance & value)
```

#### Parameters
* `value` The object to copy the distance from

---

### Distance & JSN_SR04_Gen3::Distance::operator=(const Distance & value) 

Copy the distance value from another Distance object.

```
Distance & operator=(const Distance & value)
```

#### Parameters
* `value` The object to copy the distance from

#### Returns
Distance& Return this object, so you can chain multiple assignments

---

### void JSN_SR04_Gen3::Distance::setDistanceM(double distanceM) 

Set the Distance in meters.

```
void setDistanceM(double distanceM)
```

#### Parameters
* `distanceM` the distance in meters to set

---

### double JSN_SR04_Gen3::Distance::getDistanceM() const 

Get the Distance in meters.

```
double getDistanceM() const
```

#### Returns
double Distance in meters

---

### void JSN_SR04_Gen3::Distance::cm(double cm) 

Set the distance in centimeters.

```
void cm(double cm)
```

#### Parameters
* `cm` Distance in centimeters (double floating point)

Internally, the distance is stored in meters, but this sets the value in centimeters. You can mix-and-match, for example you can get the distance in inches after setting it in centimeters.

---

### double JSN_SR04_Gen3::Distance::cm() const 

Get the value of the Distance in centimeters.

```
double cm() const
```

#### Returns
double Distance in centimeters

---

### void JSN_SR04_Gen3::Distance::mm(double mm) 

Set the distance in millimeter.

```
void mm(double mm)
```

#### Parameters
* `mm` Distance in millimeters (double floating point)

Internally, the distance is stored in meters, but this sets the value in millimeters. You can mix-and-match, for example you can get the distance in inches after setting it in millimeters.

---

### double JSN_SR04_Gen3::Distance::mm() const 

Get the value of the Distance in millimeters.

```
double mm() const
```

#### Returns
double Distance in millimeters

---

### void JSN_SR04_Gen3::Distance::inch(double inch) 

Set the distance in inches.

```
void inch(double inch)
```

#### Parameters
* `inch` Distance in inches (double floating point)

Internally, the distance is stored in meters, but this sets the value in inches. You can mix-and-match, for example you can get the distance in centimeters after setting it in inches.

---

### double JSN_SR04_Gen3::Distance::inch() const 

Get the value of the Distance in inches.

```
double inch() const
```

#### Returns
double Distance in inches


## Calculations

The TRIG pin is normally low, you pulse it high for 10 µS to begin measurement.

The ECHO pin is normally low, it will go high during the measurement, and the length of the high pulse determines the distance:

Distance = (T × C) / 2

where:

- T is the time of the pulse in seconds  
- C is the speed of sound (340 meters/sec)

The detection range of the sensor I got is 2 cm (0.02 m) to 500 cm (5 m). This means the time of the pulse ranges from:

- Min: 0.02 m = 0.00011765 sec = 0.1176 ms = 117 µs
- Max: 5 m = 0.0294 sec = 29 ms


The accuracy of the sensor I got is claimed to be 0.3 cm, or 0.0003 meters. 

- 0.0003 meters = 0.00000186 sec = 0.00176 milliseconds = 1.76 microseconds
- 568,181 Hz

We want to sample at roughly twice that frequency, if possible, or even higher.

The I2S peripheral has many possible configurations:

![](images/i2sconfig.png)

Note that the sample rate (LRCK), is the number of 16-bit stereo samples. Each bit of our input is one of those bits, so a sample rate of 32000 Hz is actually samples at 1,024,000 Hz, which is close to our target. Perfect!

In other words, with a 1 MHz sample rate, we can measure the width of a pulse with a resolution if 1 µs (1 microsecond), without touching interrupts and with no interrupt latency, because the I2S peripheral does not require bit-level interrupts. 

The total buffer length is the length of the TRIG pulse (10 µS) + setup time + maximum detection pulse (up to 29 ms). 

The TRIG pulse output should be 10 µs. This is a minimum of 10 bits of output 1. We use 16 bits because it's really convenient with 16-bit samples.

The setup time is not in the datasheet, but dumping the ECHO pin output from the sensor looks like this:

```
0000005026 [app] INFO: 0090: 0000 0000 0000 0000
0000005026 [app] INFO: 0094: 03ff ffff ffff ffff
0000005027 [app] INFO: 0098: ffff ffff ffff ffff
0000005027 [app] INFO: 009c: ffff ffff ffff ffff
0000005028 [app] INFO: 00a0: ffff ffff ffff ffff
0000005028 [app] INFO: 00a4: ffff ffff ffff ffff
0000005029 [app] INFO: 00a8: ffff ffff ffff ffff
0000005029 [app] INFO: 00ac: ffff ffff ffff ffff
0000005030 [app] INFO: 00b0: ffff ffff ffff ffff
0000005030 [app] INFO: 00b4: ffff ffff ffff ffff
0000005031 [app] INFO: 00b8: ffff ffff fffe 0000
0000005031 [app] INFO: 00bc: 0000 0000 0000 0000
0000005032 [app] INFO: 00c0: 0000 0000 0000 0000
```

- The time from the falling TRIG pulse to ECHO going high is 0x93 16-bit samples (147), or 2.35 ms.

- In other words, the leading overhead should be assumed to be around 150 16-bit samples.

- When the signal goes high the sample has a value of 0x03ff. The leftmost (MSB) is the first, chronologically.

- When the signal goes low the sample has a value of 0xfff3. The rightmost (LSB) is the last, chronologically.

I set the leadingOverhead to 152 16-bit samples, or 2432 µs.

Even though this sensor is theoretically able to measure up to 5 meters, I've never had a sensor work at that distance reliably. To save RAM, I set the maximum distance to 1 meter. You could set it shorter or a longer if desired.

- D = 1 meters
- C = 340 meters/sec
- T = (2 × D) / C
- T = 0.005882 sec = 5.882 ms = 5882 µs = 5,882 1-bit samples = 368 16-bit samples

Adding in the 152 16-bit setup time results in 520 16-bit samples. This is 1,040 bytes, however we need both transmit and receive DMA buffers, so that's a total overhead of 2,080 bytes for transmit and receive buffers at 1 meter.

If you wanted a 2 meter range and had a sensor that worked acceptable at that range, it would require 3,552 bytes of buffer. The range is floating point, so you can specify fractions of a meter as well.

At a maximum distance of 5 meters, T = (2 × D) / C = 0.294 seconds = 294 milliseconds. This is where the 300 millisecond safety timer is derived from. If the sensor fails to respond in this amount of time, something has gone wrong.

The datasheet is silent as to what happens if you pulse TRIG high while ECHO is still high. The library returns a BUSY error if you attempt this since the behavior in unspecified.

The getSampleTimeMs() method returns the number of milliseconds it takes to process a sample. In practice it might take a few milliseconds longer because of the delays in dispatching loop().

The formula is:

- T = (2 × D) / C
- leadingOverheadMs = leadingOverhead * 16 / 1000 = 152 * 16 / 1000 = 2.432 ms
- Cms = 0.340 meters/millisecond
- Tms = (2 × D) / Cms + leadingOverheadMs

For the default of D = 1 meter:

- D = 1 meter
- Tms = (2 × D) / Cms + leadingOverheadMs
- Tms = 2 / 0.340 + 2.432
- Tms = 8.31 ms (rounded up to 9)

Thus in theory you could sample at around every 9 milliseconds, maybe 10, but it's probably best to limit it to 100 milliseconds, or even 500 milliseconds, to be safe. If you sample frequently, be sure to handle the case where BUSY status is returned. This means that that sensor has not yet reset the ECHO output low and a sample cannot be taken yet.

## Version History

### 0.0.3 (2025-03-16)

- Fix compatibility with Device OS 5.0.0 and later with HAL_Pin_Map().

### 0.0.2 (2021-12-15)

- Fixed a bug that caused busy errors

### 0.0.1 (2021-12-15)

- Initial version
