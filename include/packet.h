#ifndef _PACKET_H
#define _PACKET_H

#include "utils.h"
#include <string>
#include <vector>

/* Main class */

/**
 * @brief Packet class defines how data is transmitted between two endpoints.
 * @note Data between server and client will be stored in packets, 
 * and each packet will be encoded into std::string then transmitted. 
 * After receiving, strings will be decoded into packets and processed by the server or client.
 */
class Packet {
public:

    /**
     * @brief Constructor for creating a packet.
     * @param packetType Packet type.
     * @param contentType Content type.
     * @param id Packet id.
     * @note Packet content should be set according to the packet type.
     */
    Packet(PacketType packetType = PacketType::INVALID, ContentType contentType = ContentType::None, PacketID id = PacketID(0));

    /**
     * @brief Destructor.
     */
    ~Packet();

    /**
     * @brief Add 1 packet argument.
     * @param arg Argument to be added. 作为请用户输入客户端的列表编号
     */
    void addArg(std::string arg);

    /**
     * @brief Get packet arguments.
     * @return Packet arguments.
     */
    std::vector<std::string> getArgs() const;

    /**
     * @brief Encode packet into string for transmission.
     * @return Encoded string.
     */
    std::string encode();

    /**
     * @brief Decode string to packet data.
     * @param data Data to be decoded.
     * @return True if decoding is successful, false otherwise.
     */
    bool decode(std::string data);

    /**
     * @brief Get packet type.
     * @return Packet type.
     */
    PacketType getPacketType() const;

    /**
     * @brief Get packet id.
     * @return Packet id.
     */
    PacketID getID() const;

    /**
     * #@brief Set packet content.
     * @param contentType Packet content.
     */
    void setContentType(ContentType contentType);

    /**
     * @brief Get packet content.
     * @return Packet content.
     */
    ContentType getContentType() const;

    /**
     * @brief Print packet information.
     * @note For debugging.
     */
    void print();

private:

    /* Packet header. */
    PacketType packetType;               // Packet type.
    ContentType contentType;           // Packet content type.
    PacketID id;                   // Packet id.
    PacketLength length;           // Packet total length.

    std::vector<std::string> args; // Packet arguments.要交付的用户和消息
    
    /* Packet checksum. */
    std::string checksum;          // Packet checksum.

    /* Calculate MD5 checksum of the packet. */
    std::string getChecksum(std::string origin);

    /* Validate the packet checksum. */
    bool validateChecksum(std::string data);

};

#endif /* _PACKET_H */