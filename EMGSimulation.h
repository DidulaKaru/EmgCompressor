#ifndef EMG_SIMULATION_H
#define EMG_SIMULATION_H

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>

constexpr int ADC_MIN = 0;
constexpr int ADC_MAX = 4096;

inline void initRandom(){
    std::srand(static_cast<unsigned int>(std::time(NULL)));
}

inline double getEMGSample(double time) {
    constexpr double PI = 3.14159265358979323846;
    const double period = 2.0;
    const double burstDuration = 0.5;
    const double tMod = std::fmod(time, period);

    const double contraction = (tMod < burstDuration) ? 1.0 : 0.2;

    // Noise floor around a small positive baseline to mimic single-ended ADC input.
    const double baseline = 0.08 + ((std::rand() % 2001) - 1000) / 100000.0;

    const double burstA = std::fabs(std::sin(2.0 * PI * 90.0 * time));
    const double burstB = std::fabs(std::sin(2.0 * PI * 140.0 * time));
    const double activity = contraction * (0.22 * burstA + 0.16 * burstB);

    double spike = 0.0;
    const int threshold = static_cast<int>(300 * contraction);
    if ((std::rand() % 1000) < threshold) {
        spike = contraction * ((std::rand() % 240) / 1000.0);
    }

    const double sample = baseline + activity + spike;
    if (sample < 0.0) return 0.0;
    if (sample > 1.0) return 1.0;
    return sample;
}

inline int maptoADC(double sample){
    if (sample < 0.0) sample = 0.0;
    if (sample > 1.0) sample = 1.0;
    const int adc = static_cast<int>(std::round(sample * ADC_MAX));
    if (adc < ADC_MIN) return ADC_MIN;
    if (adc > ADC_MAX) return ADC_MAX;
    return adc;
}

#endif