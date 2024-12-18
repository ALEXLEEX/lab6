#ifndef _PACKET_UTILS_H
#define _PACKET_UTILS_H

/* Packet types */
enum class PacketType {
    INVALID,    // Invalid packet.
    REQUEST,    // Request packet. 请求
    RESPONSE,   // Response packet. 响应
    ASSIGNMENT  // Assignment packet. 指示 指示服务器给别的客户发消息
};

/* Content types */
enum class ContentType {
    None,                   // No content.

    RequestTime,            // Request time.
    RequestName,            // Request name.
    RequestClientList,      // Request client list.
    RequestSendMessage,     // Request message.
    RequestMakeConnection,  // Request make connection.
    RequestCloseConnection, // Request close connection.

    ResponseTime,           // Response time.
    ResponseName,           // Response name.
    ResponseClientList,     // Response client list.
    ResponseSendMessage,    // Response message.
    ResponseMakeConnection, // Response make connection.
    ResponseCloseConnection,// Response close connection.
    ResponseUnknown,        // Response unknown request.

    AssignmentSendMessage,  // Assignment send message.
    
};

/* Packet id */
using PacketID = unsigned char;

/* Packet length */
using PacketLength = unsigned short;

#endif /* _PACKET_UTILS_H */