//
// Created by 张博坤 on 2024/10/13.
//
#include <unordered_map>
#include <string>
#include "../buttonrpc_cpp14/buttonrpc.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include "worker.cpp"
#include <unordered_set>
using namespace std;
namespace fs = std::filesystem;
class Master {
private:
    vector<KeyValue> mapTasks;
    mutex mapTaskMtx;
    mutex reduceTaskMtx;
    vector<KeyValue> reduceTasks;
    mutex accessMapStatMtx;
    bool mapDone=false;
    unordered_set<string> mapWorkerIPs;
    string ipReformat;
    unordered_set<int> ihashValues;
    int finishedMapNum=0;
    int mapTotalNum = 0;

public:
    void loadTasks(const string& path,size_t partSize) {
        // 遍历指定路径下的所有文件
        for (const auto &entry: fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                ifstream file(entry.path(), std::ios::binary);

                if (!file) {
                    std::cerr << "Failed to open " << entry.path() << '\n';
                    continue;
                }

                size_t fileSize = file.tellg(); // 获取文件大小
                file.close();

                size_t parts = fileSize / partSize + (fileSize % partSize != 0);
                for (size_t i = 0; i < parts; ++i) {
                    size_t start = i * partSize;
                    size_t end = (i + 1) * partSize - 1;
                    if (end >= fileSize) end = fileSize - 1;
                    KeyValue* temp = new KeyValue();
                    temp->key= entry.path().string();
                    temp->value= to_string(start)+'/'+ to_string(end);
                    mapTasks.push_back(*temp);
                }
            }
        }
        mapTotalNum=mapTasks.size();
    }
    KeyValue assignReduceTask(){
        //要告诉你位置
        //The locations of these buffered pairs on the local disk are
        // passed back to the master,
        // who is responsible for forwarding these locations to the reduce workers.
        //每个worker要去所有ip，得到某个特定ihash，然后写在文件
        KeyValue temp;
        reduceTaskMtx.lock();
        if(reduceTasks.empty()){
            temp = * new KeyValue();
            temp.key="empty";
        }else{
            temp=reduceTasks.back();
            reduceTasks.pop_back();
        }
        reduceTaskMtx.unlock();
        return temp;
    }
    KeyValue assignMapTask(){
        KeyValue temp;
        mapTaskMtx.lock();
        //empty也不代表map完了，还有可能在等别的挂掉的任务，这是map还没结束，还是会走到这
        if(mapTasks.empty()){
            temp = * new KeyValue();
            temp.key="empty";
        }
        else {
           //这个在server的线程需要等待任务结束
            temp = mapTasks.back();
            mapTasks.pop_back();
        }
        mapTaskMtx.unlock();
        //filename,start/end
        return temp;
    }

    KeyValue assignTasks() {
        // 分配任务给每个 Worker
        accessMapStatMtx.lock();
        if(mapDone){//reduce
            accessMapStatMtx.unlock();
            return assignReduceTask();
        }else{//map
            accessMapStatMtx.unlock();
            return assignMapTask();
        }

    }
    void makeReduceTask(){
        for(auto& i:ihashValues){
            KeyValue* temp = new KeyValue();
            temp->key=ipReformat;
            temp->value=to_string(i);
            temp->isMapTask=false;
            reduceTasks.push_back(*temp);
        }
    }
    void setAMapTaskDone(const std::string& workerIp){
        accessMapStatMtx.lock();
        finishedMapNum+=1;
        mapWorkerIPs.insert(workerIp);
        //这个if只运行一次
        if(finishedMapNum==mapTotalNum){
            mapDone=true;//下次assignTask就是reduce了！
            ipReformat="";
            for(auto& s:mapWorkerIPs)ipReformat=ipReformat+s+'/';
            makeReduceTask();
        }
        accessMapStatMtx.unlock();
    }

    void waitAll() {
        // 等待所有 Worker 完成或超时
        // 检查哪些任务完成，哪些需要重分配
    }
};

int main(int argc, char* argv[]){
    Master* master = new Master();
    size_t temp = stoi(argv[1]);
    master ->loadTasks(argv[0],temp);

    buttonrpc server;
    server.as_server(5555);

    server.bind("assignTasks", &Master::assignTasks, master);
    server.bind("setAMapTaskDone", &Master::setAMapTaskDone, master);
    server.run();
}