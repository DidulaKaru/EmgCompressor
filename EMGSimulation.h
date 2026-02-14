#ifndef EMG_SIMULATION_H
#define EMG_SIMULATION_H

#include <iostream>
#include <cstlib> //for rand() and srand()
#include <cmath> //for sin() and M_PI
#include <ctime> //for time() to seed random number generator

using namespace std;

// Initializes the random number generator with the current time as the seed
inline void initRandom(){
    srand(time(0));
}

//Generates a simulated EMG signal based on a sine wave with added random noise
inline double generateEMG(double time, double frequency = 5.0){
    double amplitude = sin(2 * M_PI * frequency * time); 
    double noise = ((double)rand()%200-100)/1000.0; //Random noise between -0.1 and 0.1
    return amplitude + noise;
}

//Maps the simulated EMG signal to a 16-bit ADC range (0 to 65535)
inline int maptoADC(double sample){
    return static_cast<int>((sample + 1.0) * 32767); //Map -1.0 to 1.0 range to 0 to 65535 for 16-bit ADC
}