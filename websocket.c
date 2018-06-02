#include "websocket.h"

int websocket_connect(char* ip, uint16_t port) {
    struct sockaddr_in addr;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("Error opening socket\n");
        return -1;
    }

    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_family = AF_INET;


    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 20*1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    if(connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) ) == -1)
    {
        printf("Error connecting socket\n");
        return -1;
    }

    printf("Successfully connected");
    return fd;
}

websocket_client websocket_init() {
    websocket_client client;
    client.on_message_received = NULL;
    client.on_connected = NULL;
    client.frame = NULL;
    return client;
}

void websocket_send_frame(websocket_client* client, websocket_frame* frame) {
    uint8_t header[4];

    header[0] = frame->fin << 7;
    header[0] |= frame->opcode;

    if (frame->payload_len < 125) {
        header[1] = frame->payload_len;
        send(client->socket, header, 2, 0);
    } else if (frame->payload_len < 0xFFFF) {
        header[1] = 126;
        header[2] = frame->payload_len >> 8;
        header[3] = frame->payload_len & 0xff;
        send(client->socket, header, 4, 0);
    }

    send(client->socket, frame->rawdata, frame->payload_len, 0);
}

void websocket_send_text(websocket_client* client, char* data, uint16_t len) {
    websocket_frame* frame = malloc(sizeof(websocket_frame));

    frame->fin = true;
    frame->opcode = WS_OPCODE_TEXT;
    frame->mask = false;
    frame->payload_len = len;
    frame->rawdata = malloc(len);

    memcpy(frame->rawdata, data, len);


    if (client->frame == NULL){
        frame->prev = NULL;
        client->frame = frame;
    } else {
        frame->prev = client->frame;
        client->frame = frame;
    }
}

void websocket_skip_frame(websocket_client* client, uint32_t data_to_skip) {
    uint8_t buffer[512];
    while (data_to_skip > 0) {
        int r;
        if (data_to_skip > sizeof(buffer)) {
            r = recv(client->socket, buffer, sizeof(buffer), 0);
        } else {
            r = recv(client->socket, buffer, data_to_skip, 0);
        }
        data_to_skip -= r;
    }
}

void* websocket_runner(void* arg) {
    websocket_client* client = arg;
    uint8_t buffer[512]; // TODO: reduce size

    while(1){
        size_t r = recv(client->socket, buffer, 2,0);
        if (r == 2){

            printf("r %d\n", r);

            uint32_t packet_size = buffer[1] & 0x7F;

            bool is_last = buffer[0] >> 7;

            if (packet_size == 126) {
                uint8_t size[2];
                recv(client->socket, buffer, 2,0);
                packet_size = buffer[0] << 8 | buffer[1];
            } else if (packet_size == 127) {
                uint8_t size[4];
                recv(client->socket, buffer, 4,0);
                packet_size = buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
                //handling frames bigger than 65k will lead to out of memory errors
            }

            if (packet_size < 0xFFFF) {
                websocket_frame frame;
                frame.payload_len = packet_size;
                frame.rawdata = malloc(packet_size);
                r = recv(client->socket, frame.rawdata, packet_size, 0);
                if (client->on_message_received != NULL){
                    client->on_message_received(frame.rawdata, packet_size);
                }
            } else {
                websocket_skip_frame(client, packet_size);
            }
        }

        if (client->frame != NULL) {
            websocket_frame* frame = client->frame;
            websocket_send_frame(client, frame);
            client->frame = frame->prev;

            free(frame->rawdata);
            free(frame);
        }
    }
}

bool websocket_handshake(websocket_client* client, char* ip, uint16_t port, char* path) {
    uint8_t buffer[512];
    int res;

    char request_headers[1024];

    char* websocket_key = "AQIDBAUGBwgJCgsMDQ4PEC=="; //TODO: randomize

    char request_host[100];
    if (port != 80) {
        snprintf(request_host, 255, "%s:%d", ip , port);
    } else {
        snprintf(request_host, 255, "%s", ip);
    }

    snprintf(request_headers, 1024, "GET %s HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nHost: %s\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n", path, request_host, websocket_key);

    int len = send(client->socket, request_headers, strlen(request_headers),0);

    size_t rc;
    while(1){
        rc = recv(client->socket, buffer,sizeof(buffer),0);
        if (rc >0){
            //TODO: add real handshake
            if (strncmp("HTTP/1.1 101 Switching Protocols", (char*)buffer, 32) != 0) {
                return false;
            }
            break;
        }
    }

    if (pthread_create(&client->thread, NULL, websocket_runner, client)) {
        printf("Error creating thread");
        return false;
    }

    if (client->on_connected != NULL) {
        client->on_connected();
    }

    return true;
}

bool websocket_open(websocket_client* client, char* ip, uint16_t port, char* path) {
    client->socket = websocket_connect(ip, port);
    if (client->socket == -1) {
        return false;
    } else {
        return websocket_handshake(client, ip, path, path);
    }
}

