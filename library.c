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
#include "clipboard_protocol.h"
#include "utils.h"
#include "cbmessage.pb-c.h"

/*
@Name: clipboard_connect()
@Args: (char*) clipboard_dir - directory   where   the  local clipboard was launched
@Desc: Connects to a local clipboard
@Return: (int) return -1 if the local clipboard can not be accessed and a positive value in
            case of success. The returned value will be used in all other functions as  clipboard_id
*/
int clipboard_connect(char *clipboard_dir)
{
    struct sockaddr_un clipboard_addr;

    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd == -1)
    {
        return -1;
    }

    clipboard_addr.sun_family = AF_UNIX;
    
    sprintf(clipboard_addr.sun_path, "%sCLIPBOARD_SOCKET", clipboard_dir);
    if (connect(socket_fd, (const struct sockaddr *)&clipboard_addr, sizeof(clipboard_addr)) == -1)
    {
        return -1;
    }

    return socket_fd;
}
/*
@Name: clipboard_copy()
@Args: (int) clipboard_id - value returned by clipboard_connect
       (int) region - region number
       (void*) buf -  pointer to the data
       (size_t) count - lenght of data pointed by buf
@Desc: Copies to a clipboard identified by clipboard_id
@Return: (int) returns the number of bytes copied or 0 in case of error
*/
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {
    if (!(region > 0 || region < NUM_REGIONS) || count <= 0 || buf == NULL)
        return 0;

    CBMessage *msg, *conf_msg;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    int bytes = 0;
    
    
    packed_message request = new_message(Request, Copy, region, buf, count, 0,0,0,0);
    packed_message request_with_size = new_message(Request, Copy, region, NULL, request.size, 0,0,0,0);

    //Send Request to server
    bytes = write(clipboard_id, request_with_size.buf, request_with_size.size);
    if (bytes == -1 || !bytes) {
        free(request.buf);
        free(request_with_size.buf);
        return 0;
    }
    //Receive response
    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes) {
        free(request.buf);
        free(request_with_size.buf);
        return 0;
    }

    msg = cbmessage__unpack(NULL, bytes, response_buffer);
    
    if(msg->has_status && msg->status) {
        bytes = write(clipboard_id, request.buf, request.size);
        if (bytes == -1 || !bytes) {
            free(request.buf);
            free(request_with_size.buf);
            return 0;
        }
        bzero(response_buffer, MESSAGE_MAX_SIZE);
        
        bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
        if (bytes == -1 || !bytes)
        {
            free(request.buf);
            free(request_with_size.buf);
            return 0;
        }

        conf_msg = cbmessage__unpack(NULL, bytes, response_buffer);

        bytes = conf_msg->size;
        cbmessage__free_unpacked(conf_msg, NULL);
    }

    cbmessage__free_unpacked(msg, NULL);
    

    free(request.buf);
    free(request_with_size.buf);

    return bytes;

}

/*
@Name: clipboard_paste()
@Args: (int) clipboard_id - value returned by clipboard_connect
       (int) region - region number
       (void*) buf -  pointer to the data
       (size_t) count - lenght of data pointed by buf
@Desc: PAstes data from clipboard identified by clipboard_id
@Return: (int) returns the number of bytes pasted or 0 in case of error
*/
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {
    if (!(region > 0 || region < NUM_REGIONS) || count == 0 || buf == NULL)
        return 0;

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
        free(request.buf);
        return 0;
    }
    msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if(msg->has_status && !msg->status) {
        cbmessage__free_unpacked(msg, NULL);
        free(request.buf);
        return 0;
    }

    size = msg->size;
    buffer = smalloc(size);

    cbmessage__free_unpacked(msg, NULL); //lets free the msg to reuse later

    response = new_message(Request, Paste, region, NULL, 0, 1,1,0,0);

    bytes = write(clipboard_id, response.buf, response.size);
    if(bytes == -1 || !bytes) {
        free(request.buf);
        free(response.buf);
        return 0;
    }
    

    bytes = sread(clipboard_id, buffer, size);
    if (bytes == -1 || !bytes) {
        free(request.buf);
        free(response.buf);
        return 0;
    }


    msg = cbmessage__unpack(NULL, size, buffer);

    size = msg->data->len;
    
    if (size > count) {
        memcpy(buf, msg->data->data, count);
        size = count;
    }
    else memcpy(buf, msg->data->data, size);

    cbmessage__free_unpacked(msg, NULL);

    free(request.buf);
    free(response.buf);
    free(buffer);

    return size;

}
/*
@Name: clipboard_wait()
@Args: (int) clipboard_id - value returned by clipboard_connect
       (int) region - region number
       (void*) buf -  pointer to the data
       (size_t) count - lenght of data pointed by buf
@Desc: Waits for a change in a region froms a clipboard identified by clipboard_id
@Return: (int) returns the number of bytes copied to that region or 0 in case of error
*/
int clipboard_wait(int clipboard_id, int region, void* buf, size_t count) {
    if (!(region > 0 || region < NUM_REGIONS) || count == 0 || buf == NULL)
        return 0;

    CBMessage *msg;
    int bytes = 0;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t size;
    void *buffer;
    packed_message response;
    packed_message request = new_message(Request, Wait, region, NULL, 0, 0, 0, 0, 0);

    bytes = write(clipboard_id, request.buf, request.size);
    if(bytes == -1 || !bytes) {
        free(request.buf);
        return 0;
    }


    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes) {
        free(request.buf);
        return 0;
    }
    
    msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if (msg->has_status && !msg->status)
    {
        cbmessage__free_unpacked(msg, NULL);
        free(request.buf);
        return 0;
    }

    size = msg->size;
    buffer = smalloc(size);

    cbmessage__free_unpacked(msg, NULL); //lets free the msg to reuse later

    response = new_message(Request, Wait, region, NULL, 0, 1, 1, 0, 0);

    bytes = write(clipboard_id, response.buf, response.size);
    if(bytes == -1 || !bytes)
    {
        free(response.buf);
        free(request.buf);
        return 0;
    }
    

    bytes = sread(clipboard_id, buffer, size);
    if (bytes == -1 || !bytes) {
        free(response.buf);
        free(request.buf);
        return 0;
    }

    msg = cbmessage__unpack(NULL, size, buffer);

    size = msg->data->len;

    if (size > count) {
        memcpy(buf, msg->data->data, count);
        size = count;
    }  else {
        memcpy(buf, msg->data->data, size);
    }
        

    cbmessage__free_unpacked(msg, NULL);

    free(request.buf);
    free(response.buf);
    free(buffer);

    return size;
}
/*
@Name: clipboard_close()
@Args: (int) clipboard_id - value returned by clipboard_connect
@Desc: Closes the connection with a clipboard identified by clipboard_id
@Return: (int) returns a positive number in case of success or -1 in case of error
*/
int clipboard_close(int clipboard_id) {
    int status = close(clipboard_id);
    return (status ? status : -1);
}