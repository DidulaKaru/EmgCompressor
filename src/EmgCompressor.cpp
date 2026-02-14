#include "EmgCompressor.h"
#include <queue>

// Arrays in C++ can't have negative indices. But since the EMG deltas might be negative, we'll add an offset.
const int DELTA_OFFSET = 32768;
// 16-bit delta symbols (-32768 to 32767)
const int MAX_SYMBOLS = 65536;

//Constructor and Destructor (Empty for now, but we might need them later)
EmgCompressor::EmgCompressor() {}
EmgCompressor::~EmgCompressor() {}

// HELPER FUNCTION: Delete Tree (Prevent Memory Leaks)
void EmgCompressor::deleteTree(HuffmanNode* root) {
    if (root == NULL) return; // Stop if we hit a dead end
    deleteTree(root->left); // recursively clean the left side
    deleteTree(root->right); // recursively clelan the right side
    delete root;
}

// HELPER FUNCTION: Recursive Code Generator
void EmgCompressor::generateCodes(HuffmanNode* root, const std::string& currentCode, std::vector<std::string>& codeTable) {
    if (root == NULL) return;

    // If leaf node, store the code
    if (!root->left && !root->right) {
        if (root->value >= -32768 && root->value <= 32767) {
            codeTable[root->value + DELTA_OFFSET] = currentCode;
        }
        return;
    }

    generateCodes(root->left, currentCode + "0", codeTable);
    generateCodes(root->right, currentCode + "1", codeTable);
}

// CORE ALGORITHM: Build Huffman Tree (Min-Heap)
struct NodeCompare {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        return a->freq > b->freq;
    }
};

HuffmanNode* EmgCompressor::buildTreeFromFrequencies(const std::vector<uint32_t>& frequencies) {
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, NodeCompare> heap;

    for (int i = 0; i < MAX_SYMBOLS; i++) {
        if (frequencies[i] > 0) {
            heap.push(new HuffmanNode(i - DELTA_OFFSET, frequencies[i]));
        }
    }

    if (heap.empty()) return NULL;

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

// MAIN: Compress Function. This is what we call from the code outside
std::vector<uint8_t> EmgCompressor::compress(const std::vector<int>& input) {
    std::vector<uint8_t> output;
    if (input.empty()) return output;
    if (input.size() > 0xFFFFFFFFu) return output;

    // STEP 1: Delta Encoding
    std::vector<int> deltas;
    deltas.reserve(input.size() - 1);
    for (size_t i = 1; i < input.size(); i++) {
        int diff = input[i] - input[i-1]; // Take the difference
        // Clamp to 16-bit delta range
        if (diff > 32767) diff = 32767;
        if (diff < -32768) diff = -32768;
        deltas.push_back(diff);
    }

    // STEP 2: Count Frequencies & Normalize
    std::vector<uint32_t> frequencies(MAX_SYMBOLS, 0);
    uint32_t maxFreq = 0;

    // A. Count
    for (int d : deltas) {
        int idx = d + DELTA_OFFSET;
        frequencies[idx]++;
        if (frequencies[idx] > maxFreq) maxFreq = frequencies[idx];
    }

    // B. Normalize (Scale down to 0-255 if needed)
    if (maxFreq > 65535) {
        for (int i = 0; i < MAX_SYMBOLS; i++) {
            if (frequencies[i] > 0) {
                uint32_t scaled = static_cast<uint32_t>((static_cast<uint64_t>(frequencies[i]) * 65535) / maxFreq);
                
                // Safety: Ensure rare symbols don't become 0 (which would delete them)
                if (scaled == 0) scaled = 1; 
                
                frequencies[i] = scaled;
            }
        }
    }

    // STEP 3: Build Tree & Codes
    HuffmanNode* root = NULL;
    std::vector<std::string> codeTable(MAX_SYMBOLS);
    if (!deltas.empty()) {
        root = buildTreeFromFrequencies(frequencies);
        generateCodes(root, "", codeTable);
    }

    // STEP 4: Write Header so that decompression can rebuild the same tree
    uint32_t uniqueSymbols = 0;
    for (int i = 0; i < MAX_SYMBOLS; i++) if (frequencies[i] > 0) uniqueSymbols++;
    
    output.push_back((uint8_t)((uniqueSymbols >> 24) & 0xFF));
    output.push_back((uint8_t)((uniqueSymbols >> 16) & 0xFF));
    output.push_back((uint8_t)((uniqueSymbols >> 8) & 0xFF));
    output.push_back((uint8_t)(uniqueSymbols & 0xFF));

    for (int i = 0; i < MAX_SYMBOLS; i++) {
        if (frequencies[i] > 0) {
            uint16_t symbolIndex = static_cast<uint16_t>(i);
            uint16_t symbolFreq = static_cast<uint16_t>(frequencies[i]);
            output.push_back((uint8_t)((symbolIndex >> 8) & 0xFF));
            output.push_back((uint8_t)(symbolIndex & 0xFF));
            output.push_back((uint8_t)((symbolFreq >> 8) & 0xFF));
            output.push_back((uint8_t)(symbolFreq & 0xFF));
        }
    }
    
    // Write original count (4 bytes)
    uint32_t count = (uint32_t)input.size();
    output.push_back((uint8_t)((count >> 24) & 0xFF));
    output.push_back((uint8_t)((count >> 16) & 0xFF));
    output.push_back((uint8_t)((count >> 8) & 0xFF));
    output.push_back((uint8_t)(count & 0xFF));

    // Write initial sample (4 bytes)
    int32_t firstSample = static_cast<int32_t>(input[0]);
    output.push_back((uint8_t)((firstSample >> 24) & 0xFF));
    output.push_back((uint8_t)((firstSample >> 16) & 0xFF));
    output.push_back((uint8_t)((firstSample >> 8) & 0xFF));
    output.push_back((uint8_t)(firstSample & 0xFF));

    // STEP 5: Bit Packing (The actual Compression)
    uint8_t currentByte = 0;
    int bitCount = 0;

    for (int d : deltas) {
        std::string code = codeTable[d + DELTA_OFFSET];
        for (char bit : code) {
            if (bit == '1') {
                currentByte |= (1 << (7 - bitCount));
            }
            bitCount++;
            
            if (bitCount == 8) {
                output.push_back(currentByte);
                currentByte = 0;
                bitCount = 0;
            }
        }
    }
    
    // Push remaining bits if any
    if (bitCount > 0) {
        output.push_back(currentByte);
    }

    if (root != NULL) deleteTree(root);
    return output;
}

// MAIN: Decompress Function

std::vector<int> EmgCompressor::decompress(const std::vector<uint8_t>& input) {
    std::vector<int> output;
    if (input.empty()) return output;

    size_t cursor = 0;

    // STEP 1: Read Header & Rebuild Tree
    if (input.size() < 4) return output;
    uint32_t uniqueSymbols = (static_cast<uint32_t>(input[cursor]) << 24) |
                             (static_cast<uint32_t>(input[cursor + 1]) << 16) |
                             (static_cast<uint32_t>(input[cursor + 2]) << 8) |
                             (static_cast<uint32_t>(input[cursor + 3]));
    cursor += 4;
    if (uniqueSymbols > static_cast<uint32_t>(MAX_SYMBOLS)) return output;

    std::vector<uint32_t> frequencies(MAX_SYMBOLS, 0);

    if (input.size() < cursor + static_cast<size_t>(uniqueSymbols) * 4 + 8) return output;

    for (uint32_t i = 0; i < uniqueSymbols; i++) {
        uint16_t symbolIndex = (static_cast<uint16_t>(input[cursor]) << 8) |
                               static_cast<uint16_t>(input[cursor + 1]);
        uint16_t symbolFreq = (static_cast<uint16_t>(input[cursor + 2]) << 8) |
                              static_cast<uint16_t>(input[cursor + 3]);
        cursor += 4;
        if (symbolIndex >= MAX_SYMBOLS) return output;
        frequencies[symbolIndex] = symbolFreq;
    }

    // Read Original Item Count (4 bytes)
    uint32_t originalCount = (static_cast<uint32_t>(input[cursor]) << 24) |
                             (static_cast<uint32_t>(input[cursor + 1]) << 16) |
                             (static_cast<uint32_t>(input[cursor + 2]) << 8) |
                             (static_cast<uint32_t>(input[cursor + 3]));
    cursor += 4;

    // Read initial sample (4 bytes)
    int32_t firstSample = (static_cast<int32_t>(input[cursor]) << 24) |
                          (static_cast<int32_t>(input[cursor + 1]) << 16) |
                          (static_cast<int32_t>(input[cursor + 2]) << 8) |
                          (static_cast<int32_t>(input[cursor + 3]));
    cursor += 4;

    if (originalCount == 0) return output;

    if (uniqueSymbols == 0) {
        if (originalCount == 1) {
            output.push_back(static_cast<int>(firstSample));
        }
        return output;
    }

    HuffmanNode* root = buildTreeFromFrequencies(frequencies);
    if (root == NULL) return output;
    HuffmanNode* current = root;

    // STEP 2: Traverse Tree using Bits
    std::vector<int> deltas;
    while (deltas.size() < (originalCount - 1) && cursor < input.size()) {
        uint8_t byte = input[cursor++];
        
        for (int i = 0; i < 8; i++) {
            // Check bit at index i
            bool isSet = (byte >> (7 - i)) & 1;

            if (isSet) current = current->right;
            else       current = current->left;

            if (current == NULL) {
                deleteTree(root);
                return output;
            }

            // Found a leaf?
            if (!current->left && !current->right) {
                deltas.push_back(current->value);
                current = root; // Reset to top
                
                if (deltas.size() == (originalCount - 1)) break;
            }
        }
    }

    // STEP 3: Reverse Delta Encoding (Prefix Sum)
    output.push_back(static_cast<int>(firstSample));
    int val = static_cast<int>(firstSample);
    for (size_t i = 0; i < deltas.size(); i++) {
        val = val + deltas[i];
        output.push_back(val);
    }

    deleteTree(root);
    return output;
}