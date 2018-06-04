#include "clipboard.h"
#include "clipboard_core.h"
#include "clipboard_handler.h"

extern connected_list *cblist;
extern store_object *store;
extern pthread_rwlock_t rwlocks[NUM_REGIONS];

extern pthread_mutex_t upper_mutex;
extern pthread_cond_t upper_cond;

extern pthread_mutex_t lower_mutex;
extern pthread_cond_t lower_cond;

extern pthread_mutex_t wait_mutex;
extern pthread_cond_t wait_cond;

extern int last_region;
extern bool connected_mode;


/*
@Name: requestHandler()
@Args: (CBMessage *) msg - pointer to the message recieved;
       (int) client - id of the client socket;
@Desc: Handles the request by type;
@Return: (void);
*/
void requestHandler(CBMessage *msg, int client)
{
    int status = 0;
    switch (msg->method)
    {
    case Copy:
        logs("Handling copy method...", L_INFO);
        status = handle_copy(client, msg);
        status ? logs("Copy method handled successfuly", L_INFO) : logs("Error handlying copy method", L_ERROR);

        pthread_mutex_lock(&wait_mutex);
        last_region = msg->region;
        pthread_cond_broadcast(&wait_cond);
        pthread_mutex_unlock(&wait_mutex);

        if (connected_mode && status && !msg->lower_copy)
        {
            logs("Sending copy call to parent", L_INFO);
            pthread_mutex_lock(&upper_mutex);
            pthread_cond_signal(&upper_cond);
            pthread_mutex_unlock(&upper_mutex);
        }
        else if ((!connected_mode || msg->lower_copy) && (cblist->size > 0) && status)
        {
            logs("Sending copy call to children", L_INFO);
            pthread_mutex_lock(&lower_mutex);
            pthread_cond_signal(&lower_cond);
            pthread_mutex_unlock(&lower_mutex);
        }
        break;
    case Paste:
        logs("Handling paste method...", L_INFO);
        status = handle_paste(client, msg);
        status ? logs("Paste method handled successfuly", L_INFO) : logs("Error handling paste method", L_ERROR);
        break;
    case Sync:
        logs("Handling sync method...", L_INFO);
        status = handle_sync(client, msg);
        status ? logs("Sync method handled successfuly", L_INFO) : logs("Error handling sync method", L_ERROR);
        break;
    case Wait:
        logs("Handling wait method...", L_INFO);
        status = handle_wait(client, msg);
        status ? logs("Wait methond handled successfuly", L_INFO) : logs("Error handling wait method", L_ERROR);
    default:
        break;
    }
}
/*
@Name: handle_sync()
@Args: (int) client - id of the client socket;
       (CBMessage*) msg - pointer to the message recieved;
@Desc: Handles sync request;
@Return: (int) 1 if the sync request was handled right;
               0 if something went wrong;
*/
int handle_sync(int client, CBMessage *msg)
{
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;
    CBMessage *user_msg;

    //Create response with data
    response = new_sync_message();
    //Create response with size
    response_with_size = new_message(Response, msg->method, 0, NULL, response.size, 0, 0, 0, 0);

    bytes = write(client, response_with_size.buf, response_with_size.size);

    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    user_msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if (user_msg->has_status && user_msg->status)
    {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if (bytes == -1 || !bytes)
        {
            if (bytes == -1)
                logs(strerror(errno), L_ERROR);
            free(response.buf);
            free(response_with_size.buf);
            return 0;
        }
    }

    cbmessage__free_unpacked(user_msg, NULL);

    free(response.buf);

    return 1;
}
/*
@Name: handle_copy()
@Args: (int) client - id of the client socket;
       (CBMessage*) msg - pointer to the message recieved;
@Desc: Handles copy request;
@Return: (int) 1 if the copy request was handled right;
               0 if something went wrong
*/
int handle_copy(int client, CBMessage *msg)
{
    size_t count;
    void *data_buffer;
    packed_message response;
    packed_message response_with_status;
    size_t bytes;
    CBMessage *user_msg;

    //Save data size to count
    count = msg->size;
    //Alloc buffer to save data
    data_buffer = smalloc(count);
    //Tell the client we are ready to receive data
    response = new_message(Response, msg->method, msg->region, NULL, 0, 1, 1, 0, 0);
    //Lets clear the message structure to reuse

    //Write response to client
    bytes = write(client, response.buf, response.size);

    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(data_buffer);
        return 0;
    }

    //Read all bytes (count) from client
    bytes = sread(client, data_buffer, count);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(data_buffer);
        return 0;
    }

    user_msg = cbmessage__unpack(NULL, count, data_buffer);
    //Store new data in the store, store will be shared var

    pthread_rwlock_wrlock(&rwlocks[user_msg->region]);
    cbstore(user_msg->region, user_msg->data->data, user_msg->data->len);
    pthread_rwlock_unlock(&rwlocks[user_msg->region]);

    response_with_status = new_message(Response, user_msg->method, user_msg->region, NULL, user_msg->data->len, 
                                       1,1,0,0);
    bytes = write(client, response_with_status.buf, response_with_status.size);

    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(data_buffer);
        return 0;
    }

    cbmessage__free_unpacked(user_msg, NULL);
    free(response.buf);
    free(data_buffer);

    return 1;
}

/*
@Name: handle_paste()
@Args: (int) client - id of the client socket;
       (CBMessage*) msg - pointer to the message recieved;
@Desc: Handles paste request;
@Return: (int) 1 if the paste request was handled right;
               0 if something went wrong;
*/
int handle_paste(int client, CBMessage *msg)
{
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;
    CBMessage *user_msg;

    int region = msg->region;

    /*Read LOCK!*/
    pthread_rwlock_rdlock(&rwlocks[region]);
    if (store[region].data == NULL)
    {
        logs("Region has no data", L_ERROR);
        //there is no data in that region send negative response
        response = new_message(Response, msg->method, msg->region, NULL, 0, 1, 0, 0, 0);

        pthread_rwlock_unlock(&rwlocks[region]);
        bytes = write(client, response.buf, response.size);
        if (bytes == -1 || !bytes)
        {
            if (bytes == -1)
                logs(strerror(errno), L_ERROR);
            free(response.buf);
            return 0;
        }
        free(response.buf);

        return 0;
    }

    //Create response with data
    response = new_message(Response, msg->method, region, store[region].data, store[region].size, 1, 1, 0, 0);
    /*Read UNLOCK!*/
    pthread_rwlock_unlock(&rwlocks[region]);
    //Create response with size
    response_with_size = new_message(Response, msg->method, region, NULL, response.size, 0, 0, 0, 0);

    bytes = write(client, response_with_size.buf, response_with_size.size);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    user_msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if (user_msg->has_status && user_msg->status)
    {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if (bytes == -1 || !bytes)
        {
            if (bytes == -1)
                logs(strerror(errno), L_ERROR);
            free(response.buf);
            free(response_with_size.buf);
            return 0;
        }
    }

    cbmessage__free_unpacked(user_msg, NULL);

    free(response.buf);

    return 1;
}
/*
@Name: handle_wait()
@Args: (int) client - id of the client socket;
       (CBMessage*) msg - pointer to the message recieved;
@Desc: Handles wait request;
@Return: (int) 1 if the wait request was handled right;
               0 if something went wrong
*/
int handle_wait(int client, CBMessage *msg)
{
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;
    CBMessage *user_msg;

    int region = msg->region;

    logs("CHECKING", L_INFO);

    /*Read LOCK!*/
    pthread_mutex_lock(&wait_mutex);
    pthread_cond_wait(&wait_cond, &wait_mutex);
    pthread_rwlock_rdlock(&rwlocks[region]);

    //Create response with data
    response = new_message(Response, msg->method, region, store[region].data, store[region].size, 1, 1, 0, 0);
    /*Read UNLOCK!*/
    pthread_rwlock_unlock(&rwlocks[region]);
    pthread_mutex_unlock(&wait_mutex);
    //Create response with size
    response_with_size = new_message(Response, msg->method, region, NULL, response.size, 0, 0, 0, 0);

    bytes = write(client, response_with_size.buf, response_with_size.size);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    if (bytes == -1 || !bytes)
    {
        if (bytes == -1)
            logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    user_msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if (user_msg->has_status && user_msg->status)
    {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if (bytes == -1 || !bytes)
        {
            if (bytes == -1)
                logs(strerror(errno), L_ERROR);
            free(response.buf);
            free(response_with_size.buf);
            return 0;
        }
    }

    cbmessage__free_unpacked(user_msg, NULL);

    free(response.buf);

    return 1;
}