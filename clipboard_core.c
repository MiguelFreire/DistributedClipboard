#include "clipboard_core.h"

extern store_object *store;
extern pthread_rwlock_t rwlocks[NUM_REGIONS];

int clipboard_lower_copy(int clipboard_id, int region, void *buf, size_t count)
{
    if (!(region > 0 || region < NUM_REGIONS) || count <= 0 || buf == NULL)
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
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(request_with_size.buf);
        return 0;
    }
    //Receive response
    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
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
            if (bytes == -1)
                logs(strerror(errno), L_ERROR);
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

/*
@Name: new_sync_message()
@Arg: None;
@Desc: Creates a message to sync with another clipboard;
@Return: struct packed_message;
*/

packed_message new_sync_message()
{
    CBMessage msg = CBMESSAGE__INIT;

    size_t packed_size;
    void *buffer;
    packed_message package = {NULL, 0};
    store_object *replica;
    replica = scalloc(NUM_REGIONS, sizeof(store_object));

    msg.type = Request;
    msg.method = Sync;

    msg.data = scalloc(NUM_REGIONS, sizeof(ProtobufCBinaryData));

    for (int i = 0; i < NUM_REGIONS; i++)
    {
        msg.n_data++;
        if (store[i].size > 0)
        {
            pthread_rwlock_rdlock(&rwlocks[i]);
            replica[i].size = store[i].size;
            replica[i].data = smalloc(sizeof(replica[i].size));
            memcpy(replica[i].data, store[i].data, replica[i].size);
            pthread_rwlock_unlock(&rwlocks[i]);

            msg.data[i].data = replica[i].data;
            msg.data[i].len = replica[i].size;
        }
    }

    packed_size = cbmessage__get_packed_size(&msg);

    buffer = smalloc(packed_size);
    cbmessage__pack(&msg, buffer);

    package.buf = buffer;
    package.size = packed_size;

    for (int i = 0; i < NUM_REGIONS; i++)
    {
        if (replica[i].data != NULL)
            free(replica[i].data);
    }
    free(replica);
    free(msg.data);

    return package;
}

/*
@Name: cbstore()
@Args: (size_t) region - region number;
       (void *) data - pointer to data buffer;
       (size_t) count - number of bytes to be copied;
@Desc: Saves data in store/regions;
@Return: (int) 1 if the data has been saved correctly;
              -1 if something went wrong;
*/

int cbstore(size_t region, void *data, size_t count)
{
    if (!(region > 0 || region < NUM_REGIONS) || data == NULL || store == NULL)
        return -1;
    //We dont need locks here because it's done in init time
    if (store[region].data != NULL)
    {
        free(store[region].data);
        store[region].size = 0;
    }

    store[region].data = smalloc(count);
    memcpy(store[region].data, data, count);
    store[region].size = count;

    return 1;
}
/*
@Name: clipboard_sync()
@Args: (int) clipboard_id - id of the clipboard that sends a request do sync;
@Desc: Performs a syncronization request to the clipboard identified by clipboard_id;
@Return: (int) size of all regions;
*/

int clipboard_sync(int clipboard_id)
{
    CBMessage *msg;
    int bytes = 0;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t size;
    void *buffer;
    packed_message response;
    packed_message request = new_message(Request, Sync, 0, NULL, 0, 0, 0, 0, 0);

    bytes = write(clipboard_id, request.buf, request.size);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(request.buf);
        return 0;
    }

    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
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

    response = new_message(Request, Sync, 0, NULL, 0, 1, 1, 0, 0);

    bytes = write(clipboard_id, response.buf, response.size);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    bytes = sread(clipboard_id, buffer, size);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    msg = cbmessage__unpack(NULL, size, buffer);

    for (int i = 0; i < NUM_REGIONS; i++)
    {
        if (msg->data[i].len > 0)
        {
            store[i].data = smalloc(msg->data[i].len);
            memcpy(store[i].data, msg->data[i].data, msg->data[i].len);
            store[i].size = msg->data[i].len;
        }
    }

    cbmessage__free_unpacked(msg, NULL);

    free(request.buf);
    free(response.buf);

    return size;
}