#ifndef EMG_SIMULATION_H
#define EMG_SIMULATION_H

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <ctime>

inline void initRandom(){
    std::srand(static_cast<unsigned int>(std::time(NULL)));
}

inline double getEMGSample(double time) {
    double contraction = 0.0;

    double period = 2.0;
    double duration = 0.5;
    double t_mod = std::fmod(time, period);

    if(t_mod < duration) {
        contraction = 1.0; 
    } else {
        contraction = 0.2; 
    }

    double baseline = (std::rand() % 200 - 100) / 1000.0;

    double spike = 0.0;
    int threshold = static_cast<int>(1000 * contraction);
    if ((std::rand() % 1000) < threshold) {
        spike = ((std::rand() % 500) / 1000.0) * contraction;
    }

    return baseline + spike;
}

inline int maptoADC(double sample){
    return static_cast<int>((sample + 1.0) * 32767); 
}

#endif