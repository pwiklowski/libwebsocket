#include "websocket.h"

websocket_client client;

void on_data_received(uint8_t* data, uint16_t len) {
    fprintf(stderr, "on_data %d\n", len);
    fprintf(stderr, "on_data %s\n", data);
}

void sender(){
    while(1){
        sleep(1);
        websocket_send_text(&client, "test", 4);
    }
}

int main(int argc, const char * argv[]) {

    client = websocket_init();
    client.on_message_received = on_data_received;

    if (websocket_open(&client, "127.0.0.1", 8080, "/connect")) {
        pthread_t t;
        pthread_create(&t, 0, sender, 0);
        pthread_join(&t, NULL);
        pthread_join(&client.thread, NULL);
    }

    return 0;
}
