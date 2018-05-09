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
#include "cbmessage.pb-c.h";

bool validate_region(int region) {
    return (region > 0 || region < NUM_REGIONS);
}


packed_message new_request(message_method method, int region, void *data, size_t count) {
    
    CBMessage msg = CBMESSAGE__INIT;

    void* buffer;
    msg.type = CBMESSAGE__TYPE__Request;
    packed_message package = {NULL,0};
    int packed_size;

    switch(method) {
        case Copy:
            msg.method = CBMESSAGE__METHOD__Copy;
            msg.region = region;
            msg.has_data = 1;
            msg.data.data = data;
            msg.data.len = count;

            packed_size = cbmessage__get_packed_size(&msg);

            buffer = smalloc(packed_size);
            cbmessage__pack(&msg, buffer);
            
            package.buf = buffer;
            package.size = packed_size;

            return package;
            break;

        case Paste:
            msg.method = CBMESSAGE__METHOD__Paste;
            msg.region = region;

            packed_size = cbmessage__get_packed_size(&msg);

            buffer = smalloc(packed_size);
            cbmessage__pack(&msg, buffer);

            package.buf = buffer;
            package.size = packed_size;

            return package;
            break;
        default:
            return package;
            break;
    }
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

    CBMessage *unpacked_msg;

    packed_message request = new_request(Copy, region, buf, count);
    packed_message response;
    
    int bytes = write(clipboard_id, &(request.buf), request.size);

    if(bytes != sizeof(request)) {
        logs("Could not send request", L_ERROR);
    }

    bytes = read(clipboard_id, &(response.buf), sizeof(CBMessage));

    cbmessage__unpack(unpacked_msg, sizeof(CBMessage), response.buf);

    if(!unpacked_msg->status) {
        logs("Could not receive all bytesxxx", L_ERROR);
        return 0;
    }

    logs("Copied successfuly!", L_INFO);
    return bytes;
}

int clipboard_paste(int clipboard_id, int region, void *buf, size_t count) {
    if (!validate_region(region) || count <= 0 || buf == NULL) return 0;

    // clipboard_message *request = new_request(Paste, region, buf, count);
    // clipboard_message response;

     int bytes = 0;

    // bytes = write(clipboard_id, &request, sizeof(request));

    // if(bytes != sizeof(request)) {
    //     logs("Could not send request", L_ERROR);
    // }
    // bytes= read(clipboard_id, &response, sizeof(response));
    // if(!response.status) {
    //     logs("Could not receive all bytes", L_ERROR);
    //     return 0;
    // }
    // logs ("Pasted successfuly!", L_INFO);

    // memcpy(buf, response.data, count);
    return bytes;
}