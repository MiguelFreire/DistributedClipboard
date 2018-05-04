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

#include "clipboard.h"
#include "utils.h"

clipboard_message new_copy_message(int region, char *data) { return new_message(region, data, Copy); }

clipboard_message new_paste_message(int region) { return new_message(region, NULL, Paste); }

clipboard_message new_message(int region, char *data, message_type type)
{

    clipboard_message msg;

    msg.region = region;
    msg.type = type;

    if (data == NULL)
    {
        msg.size = 0;
        msg.data[0] = '\0';

        return msg;
    }

    msg.size = strlen(data);
    strcpy(msg.data, data);

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
    if(region < 0 || region > NUM_REGIONS) return 0;

    clipboard_message msg = new_copy_message(region, (char *) buf);

    int bytes = 0;

    bytes = write(clipboard_id, &msg, sizeof(msg));
    
    if(bytes != sizeof(msg)) {
        logs("Could not send all bytes", L_ERROR);
    }

    int result = 0;
    
    bytes = read(clipboard_id, &result, sizeof(int));
    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
    }

    logs("Copied successfuly!", L_INFO);
    return result;
}
