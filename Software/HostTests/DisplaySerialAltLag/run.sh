#!/bin/sh
# Builds the harness twice -- against DisplaySerial.ino at BASE (default be1481c,
# before the change) and against the working tree -- runs both serial formats,
# and writes the traces to out/. Needs a C++17 compiler; CXX defaults to c++
# (any g++/clang++ works; `zig c++` was used for the committed results).
set -eu
here=$(cd "$(dirname "$0")" && pwd)
fw="$here/../../OnSpeedTeensy_AHRS"
base=${BASE:-be1481c}
cxx=${CXX:-c++}
out="$here/out"
mkdir -p "$out/build"

git -C "$fw" show "$base:Software/OnSpeedTeensy_AHRS/DisplaySerial.ino" > "$out/build/DisplaySerial_base.ino"

build() { # $1 = name, $2 = DisplaySerial.ino path
    $cxx -std=c++17 -O1 -ffp-contract=off -I"$here/stub" -I"$fw" \
        -DDISPLAY_SERIAL="\"$2\"" \
        "$here/harness.cpp" "$fw/KalmanFilter.cpp" -o "$out/build/harness_$1"
}
build base "$out/build/DisplaySerial_base.ino"
build new "$fw/DisplaySerial.ino"

for v in base new; do
    for f in ONSPEED G3X; do
        "$out/build/harness_$v" "$f" > "$out/${v}_${f}.csv"
    done
done
python3 "$here/analyze.py" "$out"
