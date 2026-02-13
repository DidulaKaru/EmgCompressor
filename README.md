# EmgCompressor

A lightweight C++ library for lossless compression of Myoelectric (EMG) sensor data. Designed for ESP32-based robotic arms to minimize wireless transmission latency.

## Features
* **Delta Encoding:** Exploits the continuous nature of bio-signals.
* **Huffman Compression:** dynamic bit-packing for maximum space savings.
* **Embedded Friendly:** written in standard C++ for portability.

## Installation
1.  Download this repository as a ZIP file.
2.  Open Arduino IDE.
3.  Go to `Sketch` -> `Include Library` -> `Add .ZIP Library...`
4.  Select the downloaded file.

## Usage
```cpp
#include <EmgCompressor.h>

EmgCompressor codec;
std::vector<int> data = {0, 0, 5, 12, 10, 2}; // Your EMG data

void setup() {
  std::vector<uint8_t> compressed = codec.compress(data);
  // Send 'compressed' over Wi-Fi/ESP-NOW...
}