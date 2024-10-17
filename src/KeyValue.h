
#include <vector>
using namespace std;
#ifndef MAPREDUCE_KEYVALUE_H
#define MAPREDUCE_KEYVALUE_H

#endif //MAPREDUCE_KEYVALUE_H
class KeyValue{
public:
    string key;
    string value;
    bool isMapTask;
} ;
struct TaskEqual{
    bool operator()(const KeyValue& k1, const KeyValue& k2) const{
        return k1.value==k2.value && k1.key==k2.key;
    }
};
struct TaskHash{
    std::size_t operator()(const KeyValue& kv) const {
        return hash<string>()(kv.key) ^ (hash<string>()(kv.value) << 1);
    }
};