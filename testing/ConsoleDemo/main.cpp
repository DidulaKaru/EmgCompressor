/*
  Console demo: generate EMG samples, compress, decompress, and verify round-trip.
  Build example (from repo root):
  g++ -std=c++17 testing/ConsoleDemo/main.cpp src/EmgCompressor.cpp -o testing/ConsoleDemo/emg_demo
*/

#include <algorithm>
#include <iostream>
#include <vector>
#include <cstdint>
#include <queue>
#include <string>
#include "../../EMGSimulation.h"
#include "../../src/EmgCompressor.h"

namespace {
constexpr int DELTA_OFFSET = 32768;
constexpr int MAX_SYMBOLS = 65536;

struct NodeCompare {
  bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
    return a->freq > b->freq;
  }
};

void deleteTree(HuffmanNode* root) {
  if (root == nullptr) {
    return;
  }
  deleteTree(root->left);
  deleteTree(root->right);
  delete root;
}

HuffmanNode* buildTreeFromFrequencies(const std::vector<uint32_t>& frequencies) {
  std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, NodeCompare> heap;

  for (int i = 0; i < MAX_SYMBOLS; ++i) {
    if (frequencies[i] > 0) {
      heap.push(new HuffmanNode(i - DELTA_OFFSET, frequencies[i]));
    }
  }

  if (heap.empty()) {
    return nullptr;
  }

  if (heap.size() == 1) {
    heap.push(new HuffmanNode(0, 0));
  }

  while (heap.size() > 1) {
    HuffmanNode* left = heap.top();
    heap.pop();
    HuffmanNode* right = heap.top();
    heap.pop();

    HuffmanNode* parent = new HuffmanNode(0, left->freq + right->freq);
    parent->left = left;
    parent->right = right;
    heap.push(parent);
  }

  return heap.top();
}

void printArray(const std::string& title, const std::vector<int>& values, int perLine = 16) {
  std::cout << title << " (size=" << values.size() << "): [\n";
  for (size_t i = 0; i < values.size(); ++i) {
    std::cout << values[i];
    if (i + 1 != values.size()) {
      std::cout << ", ";
    }

    if ((i + 1) % static_cast<size_t>(perLine) == 0 && i + 1 != values.size()) {
      std::cout << "\n";
    }
  }
  std::cout << "\n]\n";
}

void printTree(const HuffmanNode* node, const std::string& prefix = "", bool isLeft = true) {
  if (node == nullptr) {
    return;
  }

  std::cout << prefix;
  std::cout << (isLeft ? "|-" : "`-");
  if (!node->left && !node->right) {
    std::cout << "leaf(value=" << node->value << ", freq=" << node->freq << ")\n";
  } else {
    std::cout << "node(freq=" << node->freq << ")\n";
  }

  const std::string childPrefix = prefix + (isLeft ? "| " : "  ");
  printTree(node->left, childPrefix, true);
  printTree(node->right, childPrefix, false);
}

bool extractFrequenciesFromCompressed(
    const std::vector<uint8_t>& compressed,
    std::vector<uint32_t>& frequencies,
    uint32_t& uniqueSymbols) {
  if (compressed.size() < 4) {
    return false;
  }

  size_t cursor = 0;
  uniqueSymbols = (static_cast<uint32_t>(compressed[cursor]) << 24) |
                  (static_cast<uint32_t>(compressed[cursor + 1]) << 16) |
                  (static_cast<uint32_t>(compressed[cursor + 2]) << 8) |
                  (static_cast<uint32_t>(compressed[cursor + 3]));
  cursor += 4;

  if (uniqueSymbols > static_cast<uint32_t>(MAX_SYMBOLS)) {
    return false;
  }

  if (compressed.size() < cursor + static_cast<size_t>(uniqueSymbols) * 4) {
    return false;
  }

  frequencies.assign(MAX_SYMBOLS, 0);
  for (uint32_t i = 0; i < uniqueSymbols; ++i) {
    uint16_t symbolIndex = (static_cast<uint16_t>(compressed[cursor]) << 8) |
                           static_cast<uint16_t>(compressed[cursor + 1]);
    uint16_t symbolFreq = (static_cast<uint16_t>(compressed[cursor + 2]) << 8) |
                          static_cast<uint16_t>(compressed[cursor + 3]);
    cursor += 4;

    if (symbolIndex >= MAX_SYMBOLS) {
      return false;
    }
    frequencies[symbolIndex] = symbolFreq;
  }

  return true;
}
}  // namespace

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

  printArray("Input array", samples);

  std::vector<uint32_t> frequencies;
  uint32_t uniqueSymbols = 0;
  if (extractFrequenciesFromCompressed(compressed, frequencies, uniqueSymbols) && uniqueSymbols > 0) {
    HuffmanNode* root = buildTreeFromFrequencies(frequencies);
    std::cout << "Huffman tree (from compressed header):\n";
    printTree(root, "", false);
    deleteTree(root);
  } else {
    std::cout << "Huffman tree: not available (empty/invalid compressed stream).\n";
  }

  printArray("Decoded array", restored);

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
