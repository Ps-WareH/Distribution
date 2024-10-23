//
// Created by 张博坤 on 2024/10/13.
//
#include <unordered_map>
#include <string>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <vector>
#include "KeyValue.h"
#include <unordered_set>
#include "../depends/buttonrpc.hpp"
using namespace std;
namespace fs = std::filesystem;


class Master {
private:
    vector<KeyValue> mapTasks;
    mutex mapTaskMtx;
    mutex reduceTaskMtx;
    vector<KeyValue> reduceTasks;
    mutex finishRMtx;
    unordered_set<int> finishedReduceTasks;
    mutex accessMapStatMtx;
    bool mapDone=false;
    unordered_set<string> mapWorkerIPs;
    string ipReformat;
    unordered_set<int> ihashValues;
    int finishedMapNum=0;
    unordered_set<KeyValue,TaskHash,TaskEqual> curRunningMaps;

    int mapTotalNum = 0;

public:
    struct ThreadParams {
        Master* masterPtr;  // 指向类实例的指针
        KeyValue temp;        // 正在被检查的task
    };
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
                    temp->isMapTask=true;
                    mapTasks.push_back(*temp);
                }
            }
        }
        mapTotalNum=mapTasks.size();
    }
    static void* waitRTime(void* arg){
        sleep(5);
    }
    static void* waitReduceTask(void* arg){
        ThreadParams* params = static_cast<ThreadParams*>(arg);
        pthread_t t;
        void* status;
        pthread_create(&t, NULL, waitRTime, NULL);
        pthread_join(t, &status);
        //如何防止重复执行任务捏？？？？？
        params->masterPtr->finishRMtx.lock();
        if(params->masterPtr->finishedReduceTasks.find(stoi(params->temp.value))==params->masterPtr->finishedReduceTasks.end()){
            params->masterPtr->reduceTaskMtx.lock();
            params->masterPtr->reduceTasks.push_back(params->temp);
            params->masterPtr->reduceTaskMtx.unlock();
        }
        params->masterPtr->finishRMtx.unlock();
        delete params;
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
        //another pthread to wait
        pthread_t tid;
        ThreadParams* params = new ThreadParams();
        params->masterPtr = this;  // 假设在类方法中使用
        params->temp = temp; // tempValue 是你要传递的额外参数
        pthread_create(&tid, NULL, waitReduceTask, params);
        //创建一个用于回收计时线程及处理超时逻辑的线程，检查running task里面，没有我们的temp->value(就是ihash）就算完成啦
        pthread_detach(tid);
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
            accessMapStatMtx.lock();
            curRunningMaps.insert(temp);
            accessMapStatMtx.unlock();
        }
        mapTaskMtx.unlock();
        //another pthread to wait
        pthread_t tid;
        ThreadParams* params = new ThreadParams();
        params->masterPtr = this;  // 假设在类方法中使用
        params->temp = temp; // tempValue 是你要传递的额外参数
        pthread_create(&tid, NULL, waitMapTask, params);
        //创建一个用于回收计时线程及处理超时逻辑的线程，检查running task里面，没有我们的temp就算完成啦
        pthread_detach(tid);
        return temp;
        //filename,start/end
    }
    static void* waitTime(void* arg){
        sleep(3);
    }
    static void* waitMapTask(void* arg){
        ThreadParams* params = static_cast<ThreadParams*>(arg);
        pthread_t t;
        void* status;
        pthread_create(&t, NULL, waitTime, NULL);
        pthread_join(t, &status);
        //如何防止重复执行任务捏？？？？？
        params->masterPtr->accessMapStatMtx.lock();
        if(params->masterPtr->curRunningMaps.find(params->temp)!=params->masterPtr->curRunningMaps.end()){
            params->masterPtr->mapTaskMtx.lock();
            params->masterPtr->mapTasks.push_back(params->temp);
            params->masterPtr->mapTaskMtx.unlock();
        }
        params->masterPtr->accessMapStatMtx.unlock();
        delete params;
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
    void setAReduceTaskDone(int iHashValue){
        finishRMtx.lock();
        finishedReduceTasks.insert(iHashValue);
        finishRMtx.unlock();
    }
    void setAMapTaskDone(const std::string& workerIp,  vector<int> ihashValue,KeyValue kv){
        accessMapStatMtx.lock();
        finishedMapNum+=1;
        mapWorkerIPs.insert(workerIp);
        curRunningMaps.erase(kv);
        for(auto& q: ihashValue)this->ihashValues.insert(q);
        //这个if只运行一次
        //all map task done
        if(finishedMapNum==mapTotalNum){
            mapDone=true;//下次assignTask就是reduce了！
            ipReformat="";
            for(auto& s:mapWorkerIPs)ipReformat=ipReformat+s+'/';
            makeReduceTask();
        }
        accessMapStatMtx.unlock();
    }

};

int main(int argc, char* argv[]){
    Master* master = new Master();
    size_t temp = stoi(argv[1]);
    master ->loadTasks(argv[0],temp);

    buttonrpc server;
    server.as_server(5555);
    //多个线程还是queue？
    server.bind("assignTasks", &Master::assignTasks, master);
    server.bind("setAMapTaskDone", &Master::setAMapTaskDone, master);
    server.bind("setAReduceTaskDone",&Master::setAReduceTaskDone,master);
    server.run();
}