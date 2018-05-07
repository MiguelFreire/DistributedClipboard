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

bool validate_region(int region) {
    return (region > 0 || region < NUM_REGIONS);
}

clipboard_message new_copy_message(int region, void *data, size_t count) { 
    return new_message(Copy, region, data, count); 
}

clipboard_message new_paste_message(int region) { return new_message(Paste, region, NULL, 0); }

clipboard_message new_request(message_method method, int region, void *data, size_t count) {
    
    clipboard_message msg;

    msg.type = Request;

    switch(method) {
        case Copy:
            msg = new_copy_message(region,data,count);
            return msg;
            break;
        case Paste:
            msg = new_paste_message(region);
            return msg;
            break;
        default:
            return msg;
            break;
    }
}

clipboard_message new_message(message_method method,int region, void *data, size_t count)
{

    clipboard_message msg;

    msg.region = region;
    msg.method = method;
    msg.status = false;
    if(count != 0) {
        msg.data = smalloc(count);
        memcpy(msg.data, data, count);
    } 

    msg.size = count;

    return msg;
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
    //set timeouts
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

    return socket_fd;
}

int clipboard_copy(int clipboard_id, int region, void *buf, size_t count) {
    if(!validate_region(region) || count <= 0 || buf == NULL) return 0;

    printf("Count : %d\n" , count);


    clipboard_message request = new_request(Copy, region, buf, count);
    
    printf("Request.Count: %d \n", request.size);
    printf("Request.Data: %s\n", (char*) request.data);


    clipboard_message response;

    int bytes = 0;

    bytes = write(clipboard_id, &request, sizeof(request));
    printf("Sizeof Request: %d\n", sizeof(request));
    printf("Bytes: %d\n", bytes);

    if(bytes != sizeof(request)) {
        logs("Could not send request", L_ERROR);
    }
    printf("Bytes1: %d\n", sizeof(response));
    bytes = read(clipboard_id, &response, sizeof(response));
    printf("Bytes1: %d\n", bytes);
    if(!response.status) {
        logs("Could not receive all bytesxxx", L_ERROR);
        return 0;
    }

    logs("Copied successfuly!", L_INFO);
    return bytes;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {
    if (!validate_region(region) || count <= 0 || buf == NULL) return 0;

    clipboard_message request = new_request(Paste, region, buf, count);
    clipboard_message response;

    int bytes = 0;

    bytes = write(clipboard_id, &request, sizeof(request));

    if(bytes != sizeof(request)) {
        logs("Could not send request", L_ERROR);
    }
    bytes= read(clipboard_id, &response, sizeof(response));
    if(!response.status) {
        logs("Could not receive all bytes", L_ERROR);
        return 0;
    }
    logs ("Pasted successfuly!", L_INFO);
    printf("Received - %s \n", response.data);
    memcpy(buf, response.data, count);
    return bytes;
}