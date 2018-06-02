#include "websocket.h"
#include <cjson/cJSON.h>

websocket_client client;

const char* AUTH_TOKEN= "2f6df06e-f634-4219-b609-5e4fbbe82c6a";

void send_message(const char *name, const cJSON *payload);

void on_data_received(uint8_t* data, uint16_t len) {
    fprintf(stderr, "on_data %s\n", data);

    cJSON *message = cJSON_Parse(data);
    if (message == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }

        return;
    }
    const cJSON *resolution = cJSON_GetObjectItem(message, "name");

    printf("msg name:\"%s\"\n", resolution->valuestring);

    if (strcmp(resolution->valuestring, "RequestGetDevices") == 0) {
        send_device_list();
    }

    cJSON_Delete(message);
}

void authorize(){
    cJSON *payload = cJSON_CreateObject();
    cJSON_AddStringToObject(payload, "token", AUTH_TOKEN);
    cJSON_AddStringToObject(payload, "name", "HUB C");
    cJSON_AddStringToObject(payload, "uuid", "2f6df06e-f634-4249-b609-5e4fbbe82c6a");

    send_message("RequestAuthorize", payload);
}

void send_device_list(){
    cJSON *payload = cJSON_CreateObject();

    char devices[] = "[{\"id\": \"0685B960-736F-46F7-BEC0-9E6CBD61HDC1\", \"name\": \"Fake C Button\", \"variables\": [  {\"href\": \"\/light\",\"if\": \"oic.if.rw\",\"n\": \"Switch\",\"rt\": \"oic.r.switch.binary\",\"values\": {\"rt\": \"oic.r.switch.binary\", \"value\": false}  } ]}]";
    cJSON_AddRawToObject(payload, "devices", devices);

    send_message("EventDeviceListUpdate", payload);
}

void send_message(const char *name, const cJSON *payload) {
    cJSON *message = cJSON_CreateObject();

    cJSON_AddStringToObject(message, "name", name);
    cJSON_AddItemToObject(message, "payload", payload);

    char* text = cJSON_PrintUnformatted(message);
    cJSON_Delete(message);
    websocket_send_text(&client, text, strlen(text));
    free(text);
}

void on_connected() {
    printf("on_connected");
    authorize();
}

void on_disconnected() {
    printf("on_disconnected");
}

int main(int argc, const char * argv[]) {

    client = websocket_init();
    client.on_message_received = on_data_received;
    client.on_connected = on_connected;
    client.on_disconnected = on_disconnected;

    if (websocket_open(&client, "127.0.0.1", 12345, "/connect")) {
        pthread_join(client.thread, NULL);
    }

    return 0;
}
