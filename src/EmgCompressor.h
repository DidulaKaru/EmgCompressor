#ifndef EMG_COMPRESSOR_H // Prevents the code from breaking if the library is included twice
#define EMG_COMPRESSOR_H

#include <vector> 
#include <string> 
#include <cstdint> 
#include <cstddef> 

struct HuffmanNode {
    int value;          
    uint32_t freq;             
    HuffmanNode *left; 
    HuffmanNode *right; 
    
    // Constructor for the HuffmanNode struct
    HuffmanNode(int v, uint32_t f) : value(v), freq(f), left(nullptr), right(nullptr) {}
};

class EmgCompressor {
    private:
        // This walks down the tree from the root to leaves. When it goes left, it adds a "0" to the current code, and when it goes right, it adds a "1". When it reaches a leaf node, it stores the current code in the code table at the index corresponding to the value of that leaf node. 
        void generateCodes(HuffmanNode* root, const std::string& currentCode, std::vector<std::string>& codeTable);
        
        // Free the memory allocated for the Huffman tree. 
        void deleteTree(HuffmanNode* root);

        // Finds the two smallest nodes and merges them into a new node until only one node (the root) remains.
        HuffmanNode* buildTreeFromFrequencies(const std::vector<uint32_t>& frequencies);

    public:
        EmgCompressor();
        ~EmgCompressor();

        // Input-raw EMG signal. Output-uint8_t 
        std::vector<uint8_t> compress(const std::vector<int>& input);

        // the reverse of the above
        std::vector<int> decompress(const std::vector<uint8_t>& input);
};

#endif