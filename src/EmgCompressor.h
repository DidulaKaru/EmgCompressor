#ifndef EMG_COMPRESSOR_H // Prevents the code from breaking if the library is included twice
#define EMG_COMPRESSOR_H

#include <vector> // We'll use vectors rather than arrays because vectors can grow them dynamically
#include <string> // For std::string
#include <stdint.h> // For uint8_t
#include <cstddef> // For size_t and NULL

struct HuffmanNode {
    int value; //The actual data          
    int freq;  //The frequency of the data           
    HuffmanNode *left; // if you go left, you add 0
    HuffmanNode *right; // if you go right, you add 1
    
    // Constructor for the HuffmanNode struct
    HuffmanNode(int v, int f) : value(v), freq(f), left(NULL), right(NULL) {}
};

class EmgCompressor {
    private:
        // This walks down the tree from the root to leaves. When it goes left, it adds a "0" to the current code, and when it goes right, it adds a "1". When it reaches a leaf node, it stores the current code in the code table at the index corresponding to the value of that leaf node. 
        void generateCodes(HuffmanNode* root, std::string currentCode, std::string* codeTable);
        
        // Free the memory allocated for the Huffman tree. 
        void deleteTree(HuffmanNode* root);

        // Finds the two smallest nodes and merges them into a new node until only one node (the root) remains.
        HuffmanNode* buildTreeFromFrequencies(int* frequencies);

    public:
        // This shit is the API (Application Programming Interface)
        EmgCompressor();
        ~EmgCompressor();

        // Input-raw EMG signal. Output-uint8_t 
        std::vector<uint8_t> compress(const std::vector<int>& input);

        // the reverse of the above
        std::vector<int> decompress(const std::vector<uint8_t>& input);
};

#endif