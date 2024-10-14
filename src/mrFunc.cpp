//
// Created by 张博坤 on 2024/10/12.
//
#include <sstream>
#include <iostream>
#include <fstream>
#include "worker.cpp"
using namespace std;
string path ="../inputArticles/";
string pathout = "../outputArticles/out.txt";
vector<size_t> splitStartEnd(string s){
    string res1 = "";
    vector<size_t > res;
    for(int i = 0;i<s.size();i++){
        if(s[i]=='/'){
            res.push_back(stoi(res1));
            res1="";
        }else res1+=s[i];
    }
    return res;
}

extern "C" vector<string> mapF(KeyValue mapTask) {
    string fullPath = path+mapTask.key;
    vector<string> res;
    ifstream file(fullPath);
    if (!file) {
        std::cerr << "Unable to open file: " << fullPath << std::endl;
        //?????是不是应该立刻让这个线程挂掉呢
        return res;  // Return empty vector if file can't be opened
    }
    vector<size_t> temp = splitStartEnd(mapTask.value);
    size_t start = temp[0];
    size_t end = temp[1];
    file.seekg(start);
    if (file.fail()) {
        std::cerr << "Failed to seek to position " << start << std::endl;
        return res;
    }
    // Read data from start to end
    char c;
    std::stringstream buffer;
    int currentPosition = start;
    while (currentPosition <= end && file.get(c)) {
        buffer << c;
        currentPosition++;
    }
    string word;
    while (buffer >> word) {
        res.push_back(word);
    }
    return res;
}
vector<string> getAllIPs(string& s){

}
//和别的worker通讯，获取制定ihash对应的单词们的所有的出现次数
extern "C" string reduceF(KeyValue reduceTask,  buttonrpc work_client) {
    vector<string> ip = getAllIPs(reduceTask.key);
    for(auto& i: ip){
        work_client.as_client(i,5555);
        work_client.set_timeout(2000);
        vector<string> data = work_client.call<vector<string>>("getDataForHash", reduceTask.value).val();
        unordered_map<string, int> count;
        for(auto& i: data){
            if(count.find(i)!=count.end()){
                count[i]=1;
            }else count[i]+=1;
        }
        std::ofstream outFile(pathout);
        if (!outFile) {
            std::cerr << "Unable to open file for writing: " << path << std::endl;
            return "Unable to open file for writing: ";
        }

        for (const auto& pair : count) {
            outFile << pair.first << ": " << pair.second << std::endl;
        }

        outFile.close();
    }

}