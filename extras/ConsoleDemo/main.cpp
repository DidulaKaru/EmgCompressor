/*
  Console demo: generate EMG samples, compress, decompress, and verify round-trip.
  Build example (from repo root):
  g++ -std=c++17 extras/ConsoleDemo/main.cpp src/EmgCompressor.cpp -o emg_demo
*/

#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdint>
#include "../../EMGSimulation.h"
#include "../../src/EmgCompressor.h"

int main() {
  initRandom();

  const int sampleRate = 1000;
  const int numSamples = 1000;

  std::vector<int> samples;
  samples.reserve(numSamples);

  double t = 0.0;
  double dt = 1.0 / sampleRate;

  for (int i = 0; i < numSamples; ++i) {
    double s = getEMGSample(t);
    samples.push_back(maptoADC(s));
    t += dt;
  }

  EmgCompressor codec;
  std::vector<uint8_t> compressed = codec.compress(samples);
  std::vector<int> restored = codec.decompress(compressed);

  size_t mismatches = 0;
  size_t count = std::min(samples.size(), restored.size());
  for (size_t i = 0; i < count; ++i) {
    if (samples[i] != restored[i]) {
      mismatches++;
    }
  }

  std::cout << "Samples: " << samples.size() << "\n";
  std::cout << "Compressed bytes: " << compressed.size() << "\n";
  std::cout << "Restored samples: " << restored.size() << "\n";
  std::cout << "Mismatches: " << mismatches << "\n";

  size_t rawBytes = samples.size() * sizeof(int);
  if (rawBytes > 0) {
    double ratio = static_cast<double>(compressed.size()) / static_cast<double>(rawBytes);
    std::cout << "Compression ratio (compressed/raw): " << ratio << "\n";
  }

  return 0;
}