// Host stand-in for the few Arduino/Teensy pieces DisplaySerial.ino and
// KalmanFilter.cpp use, so both compile unchanged on a desktop compiler.
#ifndef HOST_ARDUINO_H
#define HOST_ARDUINO_H

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>

typedef unsigned char byte;

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

inline long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

unsigned long millis();

class String {
public:
    String() {}
    String(const char *s) : s_(s) {}
    bool operator==(const char *o) const { return s_ == o; }
    bool operator!=(const char *o) const { return s_ != o; }
private:
    std::string s_;
};

// Captures one line per println() so the harness reads exactly what went on the wire.
class HostSerial {
public:
    void begin(long) { open_ = true; }
    explicit operator bool() const { return open_; }
    void print(const char *s) { line_ += s; }
    void printf(const char *fmt, ...)
    {
        char buf[128];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        line_ += buf;
    }
    void println() { last_ = line_; line_.clear(); lines_++; }
    const std::string &last() const { return last_; }
    unsigned long lines() const { return lines_; }
private:
    bool open_ = false;
    std::string line_, last_;
    unsigned long lines_ = 0;
};

extern HostSerial Serial1, Serial3, Serial5;

#endif
