#ifndef CLIPBOARD_HANDLER_H
#define CLIPBOARD_HANDLER_H
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "cbmessage.pb-c.h"

void requestHandler(CBMessage *msg, int client);
int handle_sync(int client, CBMessage *msg);
int handle_copy(int client, CBMessage *msg);
int handle_paste(int client, CBMessage *msg);
int handle_wait(int client, CBMessage *msg);

#endif