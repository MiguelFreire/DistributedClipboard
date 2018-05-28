#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include "clipboard.h"
#include "utils.h"
#include "cbmessage.pb-c.h"

bool validate_region(int region) {
    return (region > 0 || region < NUM_REGIONS);
}

packed_message new_message(message_type type, message_method method, int region, void *data, size_t count, bool has_status, bool status, bool has_lower_copy, bool lower_copy)  {
    CBMessage msg = CBMESSAGE__INIT;

    size_t packed_size;
    void* buffer;
    packed_message package = {NULL, 0};

    msg.type = type;
    msg.method = method;
    msg.region = region;
    if(data != NULL) {
        msg.n_data = 1;
        msg.data = smalloc(sizeof(ProtobufCBinaryData));
        msg.data->data = data;
        msg.data->len = count;
    }

    if(count != 0) {
        msg.has_size = 1;
        msg.size = count;
    }
    if(has_lower_copy) {
        msg.has_lower_copy = true;
        msg.lower_copy = true;
    }
    if(has_status) {
        msg.has_status = 1;
        msg.status = status;
    }
    packed_size = cbmessage__get_packed_size(&msg);
   
    buffer = smalloc(packed_size);
    cbmessage__pack(&msg, buffer);

    package.buf = buffer;
    package.size = packed_size;
    free(msg.data);

    return package;
}


int clipboard_connect(char *clipboard_dir) {
    struct sockaddr_un clipboard_addr;

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd == -1)
    {
        logs(strerror(errno), L_ERROR);
        return -1;
    }

    clipboard_addr.sun_family = AF_UNIX;
    strcpy(clipboard_addr.sun_path, CLIPBOARD_SOCKET);

    if (connect(socket_fd, (const struct sockaddr *)&clipboard_addr, sizeof(clipboard_addr)) == -1)
    {
        logs(strerror(errno), L_ERROR);
        return -1;
    }

    return socket_fd;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {
    if(!validate_region(region) || count <= 0 || buf == NULL) return 0;

    CBMessage *msg;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    int bytes = 0;
    

    
    packed_message request = new_message(Request, Copy, region, buf, count, 0,0,0,0);
    packed_message request_with_size = new_message(Request, Copy, region, NULL, request.size, 0,0,0,0);

    
    printf("Sending %d bytes\n", request_with_size.size);
    //Send Request to server
    bytes = write(clipboard_id, request_with_size.buf, request_with_size.size);
    if (bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(request_with_size.buf);
        return 0;
    }
    //Receive response
    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(request_with_size.buf);
        return 0;
    }

    msg = cbmessage__unpack(NULL, bytes, response_buffer);
    
    if(msg->has_status && msg->status) {
        bytes = write(clipboard_id, request.buf, request.size);
        if (bytes == -1 || !bytes) {
            if(bytes == -1) logs(strerror(errno), L_ERROR);
            free(request.buf);
            free(request_with_size.buf);
            return 0;
        }
    }

    cbmessage__free_unpacked(msg, NULL);
    
    free(request.buf);
    free(request_with_size.buf);

    return bytes;

}

int clipboard_lower_copy(int clipboard_id, int region, void *buf, size_t count)
{
    if (!validate_region(region) || count <= 0 || buf == NULL)
        return 0;
    printf("LOWER COPY!!!!!!!\n");
    CBMessage *msg;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    int bytes = 0;

    packed_message request = new_message(Request, Copy, region, buf, count, 0, 0, 1, 1);
    packed_message request_with_size = new_message(Request, Copy, region, NULL, request.size, 0, 0, 1, 1);

    printf("Sending %d bytes\n", request_with_size.size);
    //Send Request to server
    bytes = write(clipboard_id, request_with_size.buf, request_with_size.size);
    printf("LALLA\n");
    if (bytes == -1 || !bytes)
    {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(request_with_size.buf);
        return 0;
    }
    //Receive response
    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes)
    {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(request_with_size.buf);
        return 0;
    }
    printf("After read\n");
    msg = cbmessage__unpack(NULL, bytes, response_buffer);
    printf("After read2\n");

    if (msg->has_status && msg->status)
    {
        bytes = write(clipboard_id, request.buf, request.size);
        if (bytes == -1 || !bytes)
        {
            if(bytes == -1) logs(strerror(errno), L_ERROR);
            free(request.buf);
            free(request_with_size.buf);
            return 0;
        }
    }
    printf("After read3\n");

    cbmessage__free_unpacked(msg, NULL);

    free(request.buf);
    free(request_with_size.buf);
    printf("After read4\n");

    return bytes;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {
    if (!validate_region(region) || count == 0 || buf == NULL) return 0;

    CBMessage *msg;
    int bytes = 0;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t size;
    void *buffer;
    packed_message response;
    packed_message request = new_message(Request, Paste, region, NULL,0,0,0,0,0);

    bytes = write(clipboard_id, request.buf, request.size);
    if(bytes == 0) {
        free(request.buf);
        return 0;
    }
    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        return 0;
    }

    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if(bytes == 0) {
        free(request.buf);
        return 0;
    }
    if(bytes == -1)
    {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        return 0;
    }
    msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if(msg->has_status && !msg->status) {
        printf("Paste went wrong\n");
        cbmessage__free_unpacked(msg, NULL);
        free(request.buf);
        return 0;
    }

    size = msg->size;
    buffer = smalloc(size);

    cbmessage__free_unpacked(msg, NULL); //lets free the msg to reuse later

    response = new_message(Request, Paste, region, NULL, 0, 1,1,0,0);

    bytes = write(clipboard_id, response.buf, response.size);
    if(bytes == 0) {
        free(request.buf);
        free(response.buf);
        return 0;
    }
    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    bytes = sread(clipboard_id, buffer, size);
    if(bytes == 0) {
        free(request.buf);
        free(response.buf);
        return 0;
    }
    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    msg = cbmessage__unpack(NULL, size, buffer);

    size = msg->data->len;
    
    if (size > count) memcpy(buf, msg->data->data, count);
    else memcpy(buf, msg->data->data, size);

    cbmessage__free_unpacked(msg, NULL);

    free(request.buf);
    free(response.buf);
    free(buffer);

    return size;

}
/**
 * (int) clipboard_wait - waits for a change in <region> and then pastes it to <buf> up to <count>
 *  - (int) clipboard_id - descriptor of the clipboard
 *  - (int) region - clipboard region
 *    (void*) buf - pointer to data buffer
 *    (size_t) count - size of data buffer
 * return: (int) number of bytes paste
 */
int clipboard_wait(int clipboard_id, int region, void* buf, size_t count) {
    if (!validate_region(region) || count == 0 || buf == NULL)
        return 0;

    CBMessage *msg;
    int bytes = 0;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t size;
    void *buffer;
    packed_message response;
    packed_message request = new_message(Request, Wait, region, NULL, 0, 0, 0, 0, 0);

    bytes = write(clipboard_id, request.buf, request.size);
    if(bytes == 0) {
        free(request.buf);
        return 0;
    }
    if (bytes == -1)
    {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        return 0;
    }

    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if(bytes == 0) {
        free(request.buf);
        return 0;
    }
    if (bytes == -1)
    {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        return 0;
    }
    msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if (msg->has_status && !msg->status)
    {
        printf("Paste went wrong\n");
        cbmessage__free_unpacked(msg, NULL);
        free(request.buf);
        return 0;
    }

    size = msg->size;
    buffer = smalloc(size);

    cbmessage__free_unpacked(msg, NULL); //lets free the msg to reuse later

    response = new_message(Request, Wait, region, NULL, 0, 1, 1, 0, 0);

    bytes = write(clipboard_id, response.buf, response.size);
    if(bytes == 0) {
        free(response.buf);
        free(request.buf);
        return 0;
    }
    if (bytes == -1)
    {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    bytes = sread(clipboard_id, buffer, size);
    if(bytes == 0) {
        free(response.buf);
        free(request.buf);
        return 0;
    }
    if (bytes == -1)
    {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    msg = cbmessage__unpack(NULL, size, buffer);

    size = msg->data->len;

    if (size > count)
        memcpy(buf, msg->data->data, count);
    else
        memcpy(buf, msg->data->data, size);

    cbmessage__free_unpacked(msg, NULL);

    free(request.buf);
    free(response.buf);
    free(buffer);

    return size;
}

int clipboard_close(int clipboard_id) {
    int status = close(clipboard_id);
    return (status ? status : -1);
}