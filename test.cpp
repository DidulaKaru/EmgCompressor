#include <iostream>
#include <fstream>
#include "EMGSimulation.h"  

using namespace std;

int main() {
    initRandom();

    const int sampleRate = 1000;
    const int numSamples = 1000;

    ofstream file("emg_samples1.csv");
    file << "time,sample\n";

    double t = 0.0;
    double dt = 1.0 / sampleRate;

    for(int i = 0; i < numSamples; ++i) {
        double sample = getEMGSample(t);
        file << t << "," << sample << "\n";
        t += dt;
    }

    file.close();
    cout << "EMG samples saved to emg_samples1.csv" << endl;
    return 0;
}