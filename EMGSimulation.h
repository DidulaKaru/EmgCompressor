#ifndef EMG_SIMULATION_H
#define EMG_SIMULATION_H

#include <iostream>
#include <cstdlib> //for rand() and srand()
#include <cmath> //for sin() and M_PI
#include <ctime> //for time() to seed random number generator

using namespace std;

// Initializes the random number generator with the current time as the seed
inline void initRandom(){
    srand(time(0));
}

//Generates a simulated EMG signal based on a sine wave with added random noise
inline double getEMGSample(double time) {
    // Simulate a contraction envelope: 0 = rest, 1 = full contraction
    double contraction = 0.0;

    // Example: periodic contraction every 2 seconds for 0.5s
    double period = 2.0;      // contraction every 2s
    double duration = 0.5;    // contraction lasts 0.5s
    double t_mod = fmod(time, period);

    if(t_mod < duration) {
        contraction = 1.0;    // full contraction
    } else {
        contraction = 0.2;    // rest baseline
    }

    // Base EMG jitter
    double baseline = (rand() % 200 - 100) / 1000.0;  // small noise

    // Random spike probability increases with contraction
    double spike = 0.0;
    int threshold = static_cast<int>(1000 * contraction);  // more spikes in contraction
    if ((rand() % 1000) < threshold) {
        spike = ((rand() % 500) / 1000.0) * contraction; // spike amplitude scales
    }

    return baseline + spike;
}

//Maps the simulated EMG signal to a 16-bit ADC range (0 to 65535)
inline int maptoADC(double sample){
    return static_cast<int>((sample + 1.0) * 32767); //Map -1.0 to 1.0 range to 0 to 65535 for 16-bit ADC
}

#endif