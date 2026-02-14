/*
  Example: compress a small batch of EMG samples and print sizes to Serial.
  Target: ESP32 (Arduino IDE)
*/

#include <vector>
#include "EmgCompressor.h"

EmgCompressor codec;

void setup() {
  Serial.begin(115200);
  delay(1000);

  std::vector<int> samples;
  samples.reserve(64);
  for (int i = 0; i < 64; ++i) {
    int v = (i < 32) ? (1000 + i) : (1032 - (i - 32));
    samples.push_back(v);
  }

  std::vector<uint8_t> compressed = codec.compress(samples);

  Serial.println("EmgCompressor demo");
  Serial.print("Samples: ");
  Serial.println(samples.size());
  Serial.print("Compressed bytes: ");
  Serial.println(compressed.size());
}

void loop() {
  delay(2000);
}

