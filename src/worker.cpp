#include <iostream>
#include <string>
#include <pthread.h>
#include <ifaddrs.h>
#include <iostream>
#include <netdb.h>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include "../tempRPC/buttonrpc.hpp"
#include <dlfcn.h>  // For dynamic library functions
#include "KeyValue.h"
using namespace std;

class Worker {
public:
    vector<string> otherIps;
    typedef vector<string> (*MapFunction)(KeyValue);
    typedef vector<string> (*ReduceFunction)(KeyValue);
    unordered_map<int, vector<string>> recordMidWork;
    MapFunction mapF;
    ReduceFunction rF;
    buttonrpc work_client;
    int initialLib(){
        void* handle = dlopen("../lib/libmrfunc.so", RTLD_LAZY);
        if (!handle) {
            std::cerr << "Cannot open library: " << dlerror() << '\n';
            return 1;
        }

        // 获取函数的指针
        dlerror();  // 清除现有的错误
        this->mapF= (MapFunction)dlsym(handle, "mapF");
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Cannot load symbol 'mapF': " << dlsym_error << '\n';
            dlclose(handle);
            return 1;
        }
    }

    Worker(){
        initialLib();
    }
    int ihash(const std::string& key) {
        int hash = 0;
        for (char c : key) {
            // 使用31作为简单质数
            hash = (hash * 31 + c) & 0x7fffffff;
        }
        cout<<"hash for "<<key<<": "<<hash<<endl;
        return hash;
    }
    string getLocalIP() {
        struct ifaddrs *addresses;
        if (getifaddrs(&addresses) == -1) {
            std::cerr << "Failed to get network addresses.\n";
            return "Error";
        }
        std::string ipAddress;
        struct ifaddrs *address = addresses;
        while (address) {
            int family = address->ifa_addr->sa_family;
            if (family == AF_INET) { // Check for IPv4 addresses only
                char ap[100];
                const int family_size = sizeof(struct sockaddr_in);
                getnameinfo(address->ifa_addr, family_size, ap, sizeof(ap), nullptr, 0, NI_NUMERICHOST);
                ipAddress = ap;
                break; // Break after the first IP address is found
            }
            address = address->ifa_next;
        }
        freeifaddrs(addresses);
        return ipAddress.empty() ? "No IP Found" : ipAddress;
    }
    void executeTasks() {
        KeyValue task = work_client.call<KeyValue>("assignTasks").val();
        if(task.key=="empty")return;
        string ip = getLocalIP();

        if(task.isMapTask){
            vector<string> res = mapF(task);
            //先记录在buffer里面吧
            vector<int> ihashVs;
            for(auto& t:res){
                int temp = ihash(t);
                if(recordMidWork.find(temp)==recordMidWork.end()){
                    ihashVs.push_back(temp);
                    recordMidWork[temp]={t};
                }else recordMidWork[temp].push_back(t);
            }
            work_client.call<void>("setAMapTaskDone", ip, ihashVs,task);
        }else{
            //reduce
            //temp->key=ipReformat;
            //temp->value=to_string(i);
            if(otherIps.empty())otherIps=getOtherIps(task.key);
            unordered_map<string,int> record;
            for(auto& ip:otherIps){
                this->work_client.as_client(ip,5554);
                this->work_client.set_timeout(5000);
                vector<string> temp= this->work_client.call<vector<string>>("getDataForHash",stoi(task.value)).val();
                shuffle(record,temp);
            }
            writeFile(record,stoi(task.value));
            this->work_client.as_client("127.0.0.1", 5555);
            this->work_client.set_timeout(5000);
            this->work_client.call<void>("setAReduceTaskDone",task.value);
        }
    }
    void writeFile(unordered_map<string,int>& record,int hashValue){
        std::ofstream outFile("records_"+to_string(hashValue)+".txt");
        if (outFile.is_open()) {
            // 遍历map中的每个元素并写入文件
            for (const auto& pair : record) {
                outFile << pair.first << ": " << to_string(pair.second) << std::endl;
            }
            outFile.close(); // 关闭文件流
            std::cout << "Records written to file successfully." << std::endl;
        } else {
            std::cout << "Unable to open file." << std::endl;
        }

    }
    void shuffle(unordered_map<string,int>& record,vector<string>& temp){
        for(auto& s:temp) {
            if (record.find(s) == record.end()) {
                record[s] = 1;
            } else { record[s] += 1; }
        }
    }
    vector<string> getOtherIps(string s){
        vector<string> allIps;
        string res = "";
        int count = 0;
        while(count<s.size()){
            if(s[count]=='/'){
                allIps.push_back(res);
                res="";
            }else{
                res+=s[count];
            }
            count+=1;
        }
        return allIps;
    }
    vector<string> getDataForHash(const int& hashKey) {
       return recordMidWork[hashKey];
    }
    static void* startServer(void* arg) {
        Worker* worker = static_cast<Worker*>(arg);
        buttonrpc server;
        server.as_server(5554);
        server.bind("getDataForHash",&Worker::getDataForHash,worker);
        server.run();
    }
};

int main() {
    //假设每个节点都先是单线程
    Worker* worker = new Worker();
    worker->work_client.as_client("127.0.0.1", 5555);
    worker->work_client.set_timeout(5000);
    pthread_t serverThread;
    pthread_create(&serverThread,NULL,Worker::startServer,worker);
    pthread_detach(serverThread);
    //待办：改成while（1）循环，
    while(1) {
        worker->executeTasks();
    }
}
