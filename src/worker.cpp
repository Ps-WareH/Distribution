#include <iostream>
#include <string>
#include <pthread.h>
#include <ifaddrs.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

#include "../buttonrpc_cpp14/buttonrpc.hpp"
using namespace std;

class KeyValue{
public:
    string key;
    string value;
    bool isMapTask;
} ;

class Worker {

public:
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
    void executeTasks(KeyValue task) {

    }

    void reportCompletedTasks() {
        // 向 Master 报告已完成的任务编号
    }
};

int main() {
    //假设每个节点都先是单线程
    Worker* worker = new Worker();
    buttonrpc work_client;
    string ip = worker->getLocalIP();
    work_client.as_client("127.0.0.1", 5555);
    work_client.set_timeout(5000);
    //待办：改成while（1）循环，
    KeyValue task = work_client.call<KeyValue>("assignTasks").val();
    worker->executeTasks(task);
    //任务结束了
    work_client.call<void>("setAMapTaskDone",  ip);

}

    return 0;
}
