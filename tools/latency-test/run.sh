#!/bin/bash
# Misst die TATSAECHLICHE Latenz des LCR-Extraktors und vergleicht sie mit der,
# die er meldet. Weichen die beiden ab, ist das Plugin im Host falsch
# ausgerichtet UND der Focus-Filter erzeugt eine hoerbare Dopplung (genau der
# Bug aus Runde 28). Braucht kein JUCE - juceshim.h ersetzt die FFT durch eine
# naive DFT mit identischer Mathematik.
set -e
cd "$(dirname "$0")"
sed 's|#include <juce_dsp/juce_dsp.h>|#include "juceshim.h"|' ../../Source/DSP/LCRExtractor.h > LCRExtractor_local.h
g++ -std=c++17 -O2 -o latency-test test.cpp
./latency-test
