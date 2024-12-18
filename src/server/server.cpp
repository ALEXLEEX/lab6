// 引入一些必要的头文件
#include <iostream>
#include <string>
#include <cstring> // For memset
#include <unistd.h> // For close
#include <sys/socket.h> // For socket functions
#include <netinet/in.h> // For sockaddr_in
#include <arpa/inet.h> // For inet_ntoa
#include <glog/logging.h>

#include <vector>
#include <thread> // For threading
#include <iomanip>
#include <sstream>
#include <ctime>
#include <atomic>
#include <mutex>
#include <algorithm>

#include "../../include/packet.h"
using namespace std;

#define MAX_CLIENT_QUEUE 20
#define SERVER_ADDRESS "127.10.0.1"
#define SERVER_PORT 5346

vector<int> clientSockets;

class GlogWrapper{ // 封装Glog
public:
    GlogWrapper(char* program) {
        google::InitGoogleLogging(program);
        FLAGS_log_dir="/root/Downloads/cnlab/lab6/logs/"; //设置log文件保存路径及前缀
        FLAGS_alsologtostderr = true; //设置日志消息除了日志文件之外是否去标准输出
        FLAGS_log_prefix = "/root/Downloads/cnlab/lab6/logs/server";
        FLAGS_colorlogtostderr = true; //设置记录到标准输出的颜色消息（如果终端支持）
        FLAGS_stop_logging_if_full_disk = true;   //设置是否在磁盘已满时避免日志记录到磁盘
        // FLAGS_stderrthreshold=google::WARNING; //指定仅输出特定级别或以上的日志
        google::InstallFailureSignalHandler();
    }
    ~GlogWrapper() { google::ShutdownGoogleLogging(); }
};

void handleTimeRequest(int clientSocket, PacketID id) {
    // 获取当前时间
    time_t now = time(nullptr);
    tm *localTime = localtime(&now); // 转换为本地时间

    // 格式化时间为字符串
    stringstream timeStream;
    timeStream << put_time(localTime, "Current time is %Y-%m-%d %H:%M:%S");
    string formattedTime = timeStream.str();

    // 构建响应数据包
    Packet pkt(PacketType::RESPONSE, ContentType::ResponseTime, id);
    pkt.addArg(formattedTime);
    string message = pkt.encode();

    // 发送响应数据
    send(clientSocket, message.c_str(), message.length(), 0);
}

void handleNameRequest(int clientSocket, PacketID id) {
    // 构建响应数据包
    Packet pkt(PacketType::RESPONSE, ContentType::ResponseName, id);
    char hostname[1024];
    gethostname(hostname, 1024);
    pkt.addArg("Server Name: " + string(hostname));
    string message = pkt.encode();

    // 发送响应数据
    send(clientSocket, message.c_str(), message.length(), 0);
}

void handleClientListRequest(int clientSocket, PacketID id) {
    // 构建响应数据包
    Packet pkt(PacketType::RESPONSE, ContentType::ResponseClientList, id);
    for (int i = 0; i < clientSockets.size(); i++) {
        pkt.addArg("Client ID: "+ to_string(i+1));
    }
    string message = pkt.encode();

    // 发送响应数据
    send(clientSocket, message.c_str(), message.length(), 0);
}

void handleSendMessageRequest(int clientSocket, PacketID id, string receiverID, string message) {
    
    if (stoi(receiverID)-1 >= clientSockets.size())
    {
        LOG(ERROR) << "[Error] Invalid client ID";
        Packet pkt(PacketType::RESPONSE, ContentType::ResponseSendMessage, id);
        pkt.addArg("Invalid client ID");
        string response = pkt.encode();
        send(clientSocket, response.c_str(), response.length(), 0);
        return;
    }

    // 构建响应数据包
    Packet pkt(PacketType::ASSIGNMENT, ContentType::AssignmentSendMessage, id);
    pkt.addArg("Message from client: ");
    pkt.addArg(message);
    string messageOut = pkt.encode();
    // 发送指示数据
    send(clientSockets[stoi(receiverID)-1], messageOut.c_str(), messageOut.length(), 0);
    
    // 向源客户端发送消息
    Packet pktOut(PacketType::RESPONSE, ContentType::ResponseSendMessage, id);
    pktOut.addArg("Send message to client " + receiverID + " successfully");
    string response = pktOut.encode();
    send(clientSocket, response.c_str(), response.length(), 0);
}

void handleCloseConnectionRequest(int clientSocket, PacketID id) {
    Packet pkt(PacketType::RESPONSE, ContentType::ResponseCloseConnection, id);
    pkt.addArg("Connection closed");
    string message = pkt.encode();
    send(clientSocket, message.c_str(), message.length(), 0);
}

void handleClient(int clientSocket) {
    atomic<bool> exit_flag(true);  // 控制子线程退出

    Packet pkt(PacketType::INVALID, ContentType::None, 0);
    pkt.addArg("Hello, client!");
    string message = pkt.encode();
    send(clientSocket, message.c_str(), message.length(), 0);

    char buffer[1024] = {0};
    while (exit_flag) {
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            LOG(INFO) << "[Info] Received message from client: ";
            string message(buffer);
            Packet pktIn;
            pktIn.decode(message);
            pktIn.print();

            switch (pktIn.getContentType()) 
            {
            case ContentType::RequestTime:
                handleTimeRequest(clientSocket, pktIn.getID());
                break;
            case ContentType::RequestName:
                handleNameRequest(clientSocket, pktIn.getID());
                break;
            case ContentType::RequestClientList:
                handleClientListRequest(clientSocket, pktIn.getID());
                break;
            case ContentType::RequestSendMessage:
                handleSendMessageRequest(clientSocket, pktIn.getID(), pktIn.getArgs()[0], pktIn.getArgs()[1]);
                break;
            case ContentType::RequestCloseConnection:
                exit_flag = false;
                break;
            default:
                break;
            }
        
        } else {
            LOG(ERROR) << "[Error] Failed to receive message from client";
        }
    }
    
    LOG(INFO) << "[Info] Client requested to close connection.";
    close(clientSocket);
    
    auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
    if (it != clientSockets.end()) {
        clientSockets.erase(it);
        LOG(INFO) << "[Info] Removed client socket.";
    }
    
    return;
}

int main(int argc, char* argv[]) {
    // 初始化 glog
    auto glog = GlogWrapper(argv[0]);

    int serverSocket;
    
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    // 创建socket 用于监听客户端连接的端口 socket是一个文件描述符 可以说是服务器与os之间的通信接口
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    LOG_IF(FATAL, serverSocket < 0) << "[Error] Failed to create socket";

    // 设置服务器地址信息 操作系统可以将数据路由到正确的进程和程序上。
    // 绑定的目的是让操作系统知道哪个程序（通过套接字）负责监听某个IP和端口上的数据请求。
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    // serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_addr.s_addr = inet_addr("127.10.0.1");
    serverAddress.sin_port = htons(SERVER_PORT);

    // 绑定socket到本地地址
    LOG_IF(FATAL, bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) << "[Error] Binding failed";

    // 开始监听客户端的连接请求
    LOG_IF(FATAL, listen(serverSocket, MAX_CLIENT_QUEUE) < 0) << "[Error] Listening failed";

    std::vector<std::thread> threads; // 用于存放线程对象
    LOG(INFO) << "[Info] Server is listening on port " << SERVER_PORT << "...";
    
    while (true)
    {
        // 接受新的连接请求
        // 会返回一个新的套接字clientSocket这个套接字用于与客户端的通信
        // 原来的 serverSocket 继续在该端口上监听新的连接请求
        // clientSocket 是在 accept() 过程中由操作系统分配的 标识了与某个特定客户端的连接
        // clientSockets[i] = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        clientSockets.push_back(accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength));
        LOG_IF(ERROR, clientSockets[clientSockets.size()-1] < 0) << "[Error] A client failed to connect";
        LOG_IF(INFO, clientSockets[clientSockets.size()-1] >= 0) << "[Info] A client has connected at " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port);

        // 创建一个新的线程处理客户端请求
        threads.emplace_back(handleClient, clientSockets[clientSockets.size()-1]);
    }

    // 关闭sockets
    close(serverSocket);

    return 0;
}