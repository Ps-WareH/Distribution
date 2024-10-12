//
// Created by 张博坤 on 2024/10/12.
//
#include <sstream>
#include <vector>
#include <string>
extern "C" std::vector<std::string> mapF(const std::string& articleContents) {
    std::istringstream stream( articleContents);
    std::vector<std::string> words;
    std::string word;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}

extern "C" std::string reduceF(const std::vector<std::string>& tobeReduced) {
    return std::to_string(tobeReduced.size());
}