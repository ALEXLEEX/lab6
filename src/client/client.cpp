#include <iostream>
#include <string>
#include <cstring> // For memset
#include <unistd.h> // For close
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h> // For inet_addr
#include <glog/logging.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include "../../include/packet.h"
// 定义服务器端口和地址
#define SERVER_ADDRESS "127.10.0.1"
#define SERVER_PORT 5346
using namespace std;
atomic<bool> exit_flag(false);  // 控制子线程退出
mutex mtx;                      // 互斥锁保护队列
condition_variable response;    // 条件变量用于通知主线程
queue<Packet> msgQueue;         // 消息队列
PacketID packetID = 0;          // 消息ID

class GlogWrapper{ // 封装Glog
public:
    GlogWrapper(char* program) {
        google::InitGoogleLogging(program);
        FLAGS_log_dir="/root/Downloads/cnlab/lab6/logs/"; //设置log文件保存路径及前缀
        FLAGS_alsologtostderr = true; //设置日志消息除了日志文件之外是否去标准输出
        FLAGS_log_prefix = "client";
        FLAGS_colorlogtostderr = true; //设置记录到标准输出的颜色消息（如果终端支持）
        FLAGS_stop_logging_if_full_disk = true;   //设置是否在磁盘已满时避免日志记录到磁盘
        // FLAGS_stderrthreshold=google::WARNING; //指定仅输出特定级别或以上的日志
        google::InstallFailureSignalHandler();
    }
    ~GlogWrapper() { google::ShutdownGoogleLogging(); }
};

void mainInterface()
{    
    cout << "\n=============== MENU ================" << endl;
    
    cout << "1. Connect to Server" << endl;
    cout << "2. Disconnet" << endl;
    cout << "3. Get Time" << endl;
    cout << "4. Get Name" << endl;
    cout << "5. Get Client List" << endl;
    cout << "6. Send Message" << endl;
    cout << "7. Exit" << endl;
    
    cout << "=====================================" << endl;
}

void connectInterface()
{
    cout << "\n=============== MENU ================" << endl;
    cout << "1. Connect to Server" << endl;
    cout << "2. Exit" << endl;
    cout << "=====================================" << endl;
}

void handle(int clientSocket) {
    char buffer[1024] = {0};
    while (!exit_flag)
    {
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            string message(buffer);
            // LOG(INFO) << "[Info] Received message from server: " << buffer;

            // 加锁
            {
                lock_guard<mutex> lock(mtx);
                Packet pktIn;
                pktIn.decode(message);
                msgQueue.push(pktIn);
            }
            // 通知主线程
            response.notify_one();
        } else if (bytesReceived == 0) {
            LOG(INFO) << "[Info] Server closed connection";
            exit_flag = true;
            break;
        } else {
            LOG(ERROR) << "[Error] Failed to receive message from server";
            exit_flag = true;
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    // 初始化 glog
    auto glog = GlogWrapper(argv[0]);
    int clientSocket;
    struct sockaddr_in serverAddress;
    
    // 创建socket(int domain, int type, int protocol)
    // AF_INET: IPv4
    // SOCK_STREAM: TCP
    // 创建套接字 客户端和服务端各要创建一个
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    LOG_IF(FATAL, clientSocket < 0) << "[Error] Failed to create socket";

    // 设置服务器地址信息
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

    string user_input;
    connectInterface();
    while (getline(cin, user_input)) {
        if (user_input == "1") {
            break;
        } else if (user_input == "2") {
            cout << "[Info] Thanks for using, bye!" << endl;
            return 0;
        } else {
            cout << "[Info] Invalid input, please try again." << endl;
        }
        connectInterface();
    }

    // 连接服务器
    LOG_IF(FATAL, connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
        << "[Error] Connection failed";
    
    LOG(INFO) << "[Info] Connected to server at " << SERVER_ADDRESS << ":" << SERVER_PORT;

    vector<thread> recv_threads;
    recv_threads.emplace_back(handle, clientSocket);

    try
    {
        while (!exit_flag){
            unique_lock<mutex> msgLock(mtx);
            while (msgQueue.empty()) {
                response.wait(msgLock);
            }

            while (!msgQueue.empty()) {
                Packet pkt = msgQueue.front();
                msgQueue.pop();

                LOG(INFO) << "[Info] Received message from server: ";
                pkt.print();
            }
            msgLock.unlock();

            // 主线程发送消息到服务器
            mainInterface();
            // getline(cin, user_input);
            while (getline(cin, user_input)) {
                if (user_input == "1") {
                    if (exit_flag)
                    {
                        LOG_IF(FATAL, connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) 
                            << "[Error] Connection failed";
                        LOG(INFO) << "[Info] Connected to server at " << SERVER_ADDRESS << ":" << SERVER_PORT;
                        exit_flag = false;
                        recv_threads.emplace_back(handle, clientSocket);
                    } else {
                        LOG(INFO) << "[Info] Already connected to server";
                    }
                    break;
                } else if (user_input == "2") {
                    Packet pkt(PacketType::REQUEST, ContentType::RequestCloseConnection, packetID++);
                    string message = pkt.encode();
                    send(clientSocket, message.c_str(), message.length(), 0);
                    // exit_flag = true;
                    close(clientSocket);
                    break;
                } else if (user_input == "3") {
                    Packet pkt(PacketType::REQUEST, ContentType::RequestTime, packetID++);
                    string message = pkt.encode();
                    send(clientSocket, message.c_str(), message.length(), 0);
                    break;
                } else if (user_input == "4") {
                    Packet pkt(PacketType::REQUEST, ContentType::RequestName, packetID++);
                    string message = pkt.encode();
                    send(clientSocket, message.c_str(), message.length(), 0);
                    break;
                } else if (user_input == "5") {
                    Packet pkt(PacketType::REQUEST, ContentType::RequestClientList, packetID++);
                    string message = pkt.encode();
                    send(clientSocket, message.c_str(), message.length(), 0);
                    break;
                } else if (user_input == "6") {
                    cout << "[Info] Please enter the id you want to send message to: ";
                    getline(cin, user_input);
                    Packet pkt(PacketType::REQUEST, ContentType::RequestSendMessage, packetID++);
                    pkt.addArg(user_input);

                    cout << "[Info] Please enter the message you want to send: ";
                    getline(cin, user_input);
                    pkt.addArg(user_input);
                    string message = pkt.encode();
                    send(clientSocket, message.c_str(), message.length(), 0);
                    break;
                } else if (user_input == "7") {
                    if (exit_flag)
                    {
                        LOG(INFO) << "[Info] Thanks for using, bye!";
                        break;
                    } else {
                        Packet pkt(PacketType::REQUEST, ContentType::RequestCloseConnection, packetID++);
                        string message = pkt.encode();
                        send(clientSocket, message.c_str(), message.length(), 0);
                        exit_flag = true;
                    }
                    break;
                } else {
                    cout << "[Info] Invalid input, please try again." << endl;
                }
                mainInterface();
            }
            
        }
    }
    catch(...)
    {
        LOG(FATAL) << "[Fatal] 主线程运行发生异常";
        exit_flag = true;
    }
    
    // 关闭socket
    close(clientSocket);

    return 0;
}