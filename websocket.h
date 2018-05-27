#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

enum WS_OPCODE {
    WS_OPCODE_CONTINUATION = 0x0, //denotes a continuation frame
    WS_OPCODE_TEXT = 0x1, // %x1 denotes a text frame
    WS_OPCODE_BINARY = 0x2, // %x2 denotes a binary frame
    WS_OPCODE_CON_CLOSE = 0x8, //*  %x8 denotes a connection close
    WS_OPCODE_PING = 0x9, //       *  %x9 denotes a ping
    WS_OPCODE_PONG = 0xA // %xA denotes a pong
};


typedef struct  {
    bool fin;
    uint8_t opcode;
    bool mask;
    uint32_t payload_len;
    uint8_t* rawdata;
    void* prev;
}websocket_frame;


typedef void (*websocket_on_message_received)(uint8_t*, uint16_t);
typedef struct {
    int socket;
    struct pollfd pfd;
    websocket_on_message_received on_message_received;
    pthread_t thread;
    websocket_frame* frame;
} websocket_client;

websocket_client websocket_init();
bool websocket_open(websocket_client* client, char* ip, uint16_t port, char* path);

#endif // WEBSOCKET_H
