#include <iostream>
#include <string>
#include <pthread.h>
#include "../buttonrpc_cpp14/buttonrpc.hpp"
using namespace std;

int ihash(const std::string& key) {
    int hash = 0;
    for (char c : key) {
        // 使用31作为简单质数
        hash = (hash * 31 + c) & 0x7fffffff;
    }
    cout<<"hash for "<<key<<": "<<hash<<endl;
    return hash;
}

typedef struct{
    string key;
    string value;
} KeyValue;

int main() {

    return 0;
}
