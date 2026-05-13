#ifndef EMG_COMPRESSOR_H 
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
    
    HuffmanNode(int v, uint32_t f) : value(v), freq(f), left(nullptr), right(nullptr) {}
};

class EmgCompressor {
    private:
        void generateCodes(HuffmanNode* root, const std::string& currentCode, std::vector<std::string>& codeTable);
        void deleteTree(HuffmanNode* root);
        HuffmanNode* buildTreeFromFrequencies(const std::vector<uint32_t>& frequencies);

    public:
        EmgCompressor();
        ~EmgCompressor();

        std::vector<uint8_t> compress(const std::vector<int>& input);
        std::vector<int> decompress(const std::vector<uint8_t>& input);
};

#endif