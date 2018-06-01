#ifndef CLIPBOARD_COM_H
#define CLIPBOARD_COM_H
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/un.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"

#define CLIPBOARD_SOCKET "./CLIPBOARD_SOCKET"

void configure_unix_com();
void configure_inet_local_com();
void configure_inet_remote_com();

#endif