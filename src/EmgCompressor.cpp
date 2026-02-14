#include "EmgCompressor.h"

// Offset to handle negative deltas in array index (e.g., -50 becomes index 78)
const int DELTA_OFFSET = 128; 
const int MAX_SYMBOLS = 256;

EmgCompressor::EmgCompressor() {}
EmgCompressor::~EmgCompressor() {}

// -----------------------------------------------------------
// HELPER: Delete Tree (Prevent Memory Leaks)
// -----------------------------------------------------------
void EmgCompressor::deleteTree(HuffmanNode* root) {
    if (root == NULL) return;
    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}

// -----------------------------------------------------------
// HELPER: Recursive Code Generator
// Traverses tree to assign "0" for left and "1" for right
// -----------------------------------------------------------
void EmgCompressor::generateCodes(HuffmanNode* root, std::string currentCode, std::string* codeTable) {
    if (root == NULL) return;

    // If leaf node, store the code
    if (!root->left && !root->right) {
        if (root->value >= -128 && root->value <= 127) {
            codeTable[root->value + DELTA_OFFSET] = currentCode;
        }
        return;
    }

    generateCodes(root->left, currentCode + "0", codeTable);
    generateCodes(root->right, currentCode + "1", codeTable);
}

// -----------------------------------------------------------
// CORE ALGORITHM: Build Huffman Tree (O(N^2) Array Method)
// -----------------------------------------------------------
HuffmanNode* EmgCompressor::buildTreeFromFrequencies(int* frequencies) {
    std::vector<HuffmanNode*> nodes;

    // 1. Create Leaf Nodes
    for (int i = 0; i < MAX_SYMBOLS; i++) {
        if (frequencies[i] > 0) {
            nodes.push_back(new HuffmanNode(i - DELTA_OFFSET, frequencies[i]));
        }
    }

    // Edge case: Empty data
    if (nodes.empty()) return NULL;
    
    // Edge case: Only 1 symbol (e.g., all zeros)
    // We add a dummy node to make the tree logic work
    if (nodes.size() == 1) {
        nodes.push_back(new HuffmanNode(0, 0)); 
    }

    // 2. Build Tree by merging smallest nodes
    while (nodes.size() > 1) {
        int min1 = -1, min2 = -1;

        // Find smallest and second smallest
        for (size_t i = 0; i < nodes.size(); i++) {
            if (min1 == -1 || nodes[i]->freq < nodes[min1]->freq) {
                min2 = min1;
                min1 = i;
            } else if (min2 == -1 || nodes[i]->freq < nodes[min2]->freq) {
                min2 = i;
            }
        }

        // Merge them
        HuffmanNode* left = nodes[min1];
        HuffmanNode* right = nodes[min2];
        
        HuffmanNode* parent = new HuffmanNode(0, left->freq + right->freq);
        parent->left = left;
        parent->right = right;

        // Remove old nodes from list and add parent
        // (Swap-and-pop is faster, but order matters for determinism, 
        // so we just erase carefully. Since N is small (<50), erase is fine).
        if (min1 > min2) { // Erase larger index first to avoid shifting
            nodes.erase(nodes.begin() + min1);
            nodes.erase(nodes.begin() + min2);
        } else {
            nodes.erase(nodes.begin() + min2);
            nodes.erase(nodes.begin() + min1);
        }
        nodes.push_back(parent);
    }

    return nodes[0]; // The Root
}

// -----------------------------------------------------------
// MAIN: COMPRESS
// -----------------------------------------------------------
std::vector<uint8_t> EmgCompressor::compress(const std::vector<int>& input) {
    std::vector<uint8_t> output;
    if (input.empty()) return output;

    // STEP 1: Delta Encoding
    std::vector<int> deltas;
    deltas.reserve(input.size());
    deltas.push_back(input[0]); // First value is absolute
    for (size_t i = 1; i < input.size(); i++) {
        int diff = input[i] - input[i-1];
        // Clamp to prevent crash if noise is huge (optional safety)
        if (diff > 127) diff = 127;
        if (diff < -128) diff = -128;
        deltas.push_back(diff);
    }

    // STEP 2: Count Frequencies
    int frequencies[MAX_SYMBOLS] = {0};
    for (int d : deltas) {
        frequencies[d + DELTA_OFFSET]++;
    }

    // STEP 3: Build Tree & Codes
    HuffmanNode* root = buildTreeFromFrequencies(frequencies);
    std::string codeTable[MAX_SYMBOLS];
    generateCodes(root, "", codeTable);

    // STEP 4: Write Header (So Decompressor knows the Tree)
    // Format: [NumSymbols] -> Pairs of [Value, Frequency]
    int uniqueSymbols = 0;
    for(int i=0; i<MAX_SYMBOLS; i++) if(frequencies[i] > 0) uniqueSymbols++;
    
    output.push_back((uint8_t)uniqueSymbols);
    for(int i=0; i<MAX_SYMBOLS; i++) {
        if(frequencies[i] > 0) {
            output.push_back((uint8_t)i); // The symbol index (0-255)
            // Clamp frequency to 255 to fit in 1 byte (Loss of efficiency, not data)
            int f = frequencies[i];
            if (f > 255) f = 255; 
            output.push_back((uint8_t)f);
        }
    }
    
    // Also save original size (2 bytes) to know when to stop
    uint16_t count = (uint16_t)input.size();
    output.push_back((uint8_t)(count >> 8));
    output.push_back((uint8_t)(count & 0xFF));

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

    deleteTree(root);
    return output;
}

// -----------------------------------------------------------
// MAIN: DECOMPRESS
// -----------------------------------------------------------
std::vector<int> EmgCompressor::decompress(const std::vector<uint8_t>& input) {
    std::vector<int> output;
    if (input.empty()) return output;

    size_t cursor = 0;

    // STEP 1: Read Header & Rebuild Tree
    int uniqueSymbols = input[cursor++];
    int frequencies[MAX_SYMBOLS] = {0};

    for(int i=0; i<uniqueSymbols; i++) {
        uint8_t symbolIndex = input[cursor++];
        uint8_t symbolFreq = input[cursor++];
        frequencies[symbolIndex] = symbolFreq;
    }

    // Read Original Item Count (2 bytes)
    uint16_t originalCount = (input[cursor] << 8) | input[cursor+1];
    cursor += 2;

    HuffmanNode* root = buildTreeFromFrequencies(frequencies);
    HuffmanNode* current = root;

    // STEP 2: Traverse Tree using Bits
    std::vector<int> deltas;
    while (deltas.size() < originalCount && cursor < input.size()) {
        uint8_t byte = input[cursor++];
        
        for (int i = 0; i < 8; i++) {
            // Check bit at index i
            bool isSet = (byte >> (7 - i)) & 1;

            if (isSet) current = current->right;
            else       current = current->left;

            // Found a leaf?
            if (!current->left && !current->right) {
                deltas.push_back(current->value);
                current = root; // Reset to top
                
                if (deltas.size() == originalCount) break;
            }
        }
    }

    // STEP 3: Reverse Delta Encoding (Prefix Sum)
    if (!deltas.empty()) {
        int val = deltas[0];
        output.push_back(val);
        for(size_t i=1; i<deltas.size(); i++) {
            val = val + deltas[i];
            output.push_back(val);
        }
    }

    deleteTree(root);
    return output;
}