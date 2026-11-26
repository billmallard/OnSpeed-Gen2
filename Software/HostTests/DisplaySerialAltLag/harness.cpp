// Drives the firmware's own KalmanFilter.cpp and DisplaySerial.ino on a desktop,
// through a synthetic climb, and records the altitude each serial sentence carries.
//
// The IMU/baro step runs at 208 Hz (the ISM330DHCX rate) and calls kalman.Update()
// exactly as AHRS.ino does; writeSerialData() is called after every step, as the
// main loop does, so it sends whenever its own 100 ms timer fires.
//
// Build: see run.sh. DISPLAY_SERIAL names the DisplaySerial.ino under test.
//
// Usage: harness <G3X|ONSPEED> > out.csv

#include "Arduino.h"
#include "KalmanFilter.h"

#include <cstdlib>
#include <cstring>
#include <random>

#define G2MPS   9.80665
#define FT2M    0.3048
#define M2FT    3.28084
#define MPS2FPM 196.85

HostSerial Serial1, Serial3, Serial5;

static unsigned long hostMillis = 0;
unsigned long millis() { return hostMillis; }

// Firmware globals the writer reads, declared as in OnSpeedTeensy_AHRS.ino.
String serialOutPort;
String serialOutFormat;
byte serialCRC;
int muteAudioUnderIAS = 30;
unsigned long serialoutLastUpdate = 0;
const int serialDisplaySmoothingLat = 50;
const int serialDisplaySmoothingVert = 20;
volatile float aVertCorr = 1.0;
volatile float aLatCorr = 0.0;
volatile int dataMark = 0;
volatile float kalmanAlt = 0;
volatile float kalmanVSI = 0;
volatile float smoothedPitch = 0;
volatile float smoothedRoll = 0;
volatile float efisIAS = 0;
volatile float AOA = 5.0;
volatile float flightPath = 0;
volatile float LDmaxAOA = 6.0;
volatile float onSpeedAOAfast = 8.0;
volatile float onSpeedAOAslow = 10.0;
volatile float stallWarningAOA = 14.0;
volatile float IAS = 120.0;
volatile float LateralGSmoothed = 0.0;
volatile float VerticalGSmoothed = 1.0;
volatile float PaltSmoothed = 0.0;
volatile float gYaw = 0;
volatile int flapsPos = 0;

#include DISPLAY_SERIAL

// Truth: level at 3000 ft for 60 s, climb at 1000 fpm for 60 s, level for 60 s.
// Vertical speed ramps over 3 s at each end, so the accelerometer sees a real pull-up.
static const double kT0 = 60.0, kT1 = 120.0, kRamp = 3.0, kEnd = 180.0;
static const double kVs = 1000.0 / 60.0 * FT2M; // m/s

static double vsAt(double t)
{
    if (t < kT0 || t > kT1 + kRamp) return 0.0;
    if (t < kT0 + kRamp) return kVs * (t - kT0) / kRamp;
    if (t < kT1) return kVs;
    return kVs * (1.0 - (t - kT1) / kRamp);
}

static double accelAt(double t)
{
    if (t >= kT0 && t < kT0 + kRamp) return kVs / kRamp;
    if (t >= kT1 && t < kT1 + kRamp) return -kVs / kRamp;
    return 0.0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <G3X|ONSPEED>\n", argv[0]);
        return 2;
    }
    const bool onspeed = strcmp(argv[1], "ONSPEED") == 0;
    serialOutFormat = onspeed ? "ONSPEED" : "G3X";
    serialOutPort = "Serial5";
    const int paltAt = onspeed ? 15 : 27; // byte offset of the 6-char PALT field

    const double imuRate = 208.0;
    const double dt = 1.0 / imuRate;
    std::mt19937 rng(6400);
    std::normal_distribution<double> baroNoise(0.0, 3.0);   // ft, ~ sqrt(zVariance) of the filter
    std::normal_distribution<double> accelNoise(0.0, 0.02); // g

    double altM = 3000.0 * FT2M;
    KalmanFilter kalman;
    kalman.Configure(0.79078, 26.0638, 1e-11, altM, 0.00, 0.00); // as OnSpeedTeensy_AHRS.ino

    printf("t_s,truth_ft,kalman_ft,sent_ft,sentence\n");
    unsigned long seen = 0;
    for (long i = 0; i * dt <= kEnd; i++) {
        const double t = i * dt;
        altM += vsAt(t) * dt;
        const float Palt = float(altM * M2FT + baroNoise(rng));
        const float earthVertG = float(accelAt(t) / G2MPS + accelNoise(rng));
        kalman.Update(Palt * FT2M, earthVertG * G2MPS, float(1 / imuRate), &kalmanAlt, &kalmanVSI);

        hostMillis = (unsigned long)(t * 1000.0);
        writeSerialData();
        if (Serial5.lines() != seen) {
            seen = Serial5.lines();
            const std::string &s = Serial5.last();
            const int sent = atoi(s.substr(paltAt, 6).c_str());
            printf("%.3f,%.2f,%.2f,%d,%s\n", t, altM * M2FT, kalmanAlt * M2FT, sent, s.c_str());
        }
    }
    return 0;
}
