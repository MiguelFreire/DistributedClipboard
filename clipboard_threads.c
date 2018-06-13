#include "clipboard_threads.h"

extern bool connected_mode;
extern int last_region;
extern store_object *store;
extern connected_list *cblist;
extern connected_list *applist;

extern pthread_rwlock_t rwlocks[NUM_REGIONS];

extern pthread_mutex_t upper_mutex;
extern pthread_cond_t upper_cond;

extern pthread_mutex_t lower_mutex;
extern pthread_cond_t lower_cond;

extern pthread_mutex_t wait_mutex[NUM_REGIONS];
extern pthread_cond_t wait_cond[NUM_REGIONS];

extern pthread_mutex_t region_mutex;

extern int socket_fd_inet_remote;
extern int socket_fd_inet_local;

extern struct sockaddr_un client_addr;
/*
@Name: thread_lower_com()
@Args: (void *) arg;
@Desc: Thread that handles all lower communication copy from parent to all children;
@Return: (void *);
*/

void *thread_lower_com(void *arg)
{
    int l_region;
    int bytes;
    cb_client *cb;
    logs("Lower Com Thread launched", L_INFO);
    while (1)
    {   
        pthread_mutex_lock(&lower_mutex);
        pthread_cond_wait(&lower_cond, &lower_mutex);

        l_region = last_region;

        pthread_mutex_unlock(&region_mutex);
        pthread_rwlock_rdlock(&rwlocks[l_region]);
        pthread_mutex_lock(&(cblist->mutex));
        cb = cblist->cb;

        while (cb != NULL)
        {
            bytes = clipboard_lower_copy(cb->socket_fd2, l_region, store[l_region].data, store[l_region].size);
            cb = cb->next;
        }
        pthread_mutex_unlock(&(cblist->mutex));
        pthread_rwlock_unlock(&rwlocks[l_region]);
        pthread_mutex_unlock(&lower_mutex);
    }
}

/*
@Name: thread_upper_com()
@Args: (void *) arg;
@Desc: Thread that handles all communication from children to the parent;
@Return: (void *);
*/

void *thread_upper_com(void *arg)
{
    int l_region;
    int bytes;
    logs("Thread Upper com handler started!", L_INFO);
    while (1)
    {
        pthread_mutex_lock(&upper_mutex);
        pthread_cond_wait(&upper_cond, &upper_mutex);
        
        l_region = last_region;

        pthread_mutex_unlock(&region_mutex);
        pthread_rwlock_rdlock(&rwlocks[l_region]);

        bytes = clipboard_copy(socket_fd_inet_remote, l_region, store[l_region].data, store[l_region].size);

        if (bytes == -1 || !bytes)
        {
            connected_mode = 0;
            pthread_rwlock_unlock(&rwlocks[l_region]);
            pthread_mutex_unlock(&upper_mutex);
            pthread_exit(NULL);
        }

        pthread_rwlock_unlock(&rwlocks[l_region]);
        pthread_mutex_unlock(&upper_mutex);
    }
}

/*
@Name: thread_client()
@Args: (void *) arg;
@Desc: Thread that handles all types of network communication:
    -> app-clipboard;
    -> parent-children;
    -> clipboard-clipboard;
@Return: (void *);
*/

void *thread_client(void *arg)
{
    thread_arg *args = (thread_arg *)arg;
    int client = args->client;
    client_t client_type = args->type;
    CBMessage *msg;
    packed_message error_msg;

    uint8_t size_buffer[MESSAGE_MAX_SIZE];
    int bytes = 0;

    logs("New client connected!", L_INFO);
    while (1)
    {
        bzero(size_buffer, MESSAGE_MAX_SIZE);
        bytes = read(client, size_buffer, MESSAGE_MAX_SIZE);
        if (bytes == 0)
        {
            switch (client_type)
            {
            case Clipboard:
                remove_clipboard_by_thread_id(cblist, pthread_self());
                logs("CB removed from cb list", L_INFO);
                break;
            case App:
                remove_clipboard_by_thread_id(applist, pthread_self());
                logs("App removed from app list", L_INFO);
                break;
            case Parent:
                connected_mode = 0;
                logs("Parent died!", L_INFO);
                break;
            default:
                break;
            }
            break;
        }
        if (bytes == -1)
        {
            logs(strerror(errno), L_ERROR);
        }

        //Unpacks from proto to c format
        msg = cbmessage__unpack(NULL, bytes, size_buffer);
        
        if(msg->region > NUM_REGIONS-1) {
            
            error_msg = new_message(Response, msg->method, 0, NULL, 0, 1, 0, 0, 0);
            bytes = write(client, error_msg.buf, error_msg.size);
            
            if(bytes == -1 || bytes == 0) {
                logs(strerror(errno), L_ERROR);
            }

            cbmessage__free_unpacked(msg, NULL);
            free(error_msg.buf);
            continue;
        }

        requestHandler(msg, client);
        cbmessage__free_unpacked(msg, NULL);
    }
    logs("Client disconnected!", L_INFO);
    logs("Thread terminating!", L_INFO);

    return 0;
}

/*
@Name: thread_inet_handler()
@Args: (void *) arg;
@Desc: Thread that waits and accepts internet clients (clipboards);
@Return: (void *);
*/
void *thread_inet_handler(void *arg)
{
    unsigned int addr_size = sizeof(client_addr);
    int parent;
    int child;
    cb_client *rcb;

    while (1)
    {
        logs("Waiting for remote clients...", L_INFO);
        rcb = NULL;

        parent = accept(socket_fd_inet_local, (struct sockaddr *)&client_addr, &addr_size);
        if (parent == -1)
        {
            logs(strerror(errno), L_ERROR);
            pthread_exit(NULL);
        }
        child = accept(socket_fd_inet_local, (struct sockaddr *)&client_addr, &addr_size);
        if (child == -1)
        {
            logs(strerror(errno), L_ERROR);
            pthread_exit(NULL);
        }

        rcb = new_clipboard(parent, child);
        add_clipboard(cblist, rcb);

        thread_arg args;
        args.client = rcb->socket_fd;
        args.type = Clipboard;

        if (pthread_create(&rcb->thread_id, NULL, thread_client, &args) != 0)
        {
            logs(strerror(errno), L_ERROR);
            pthread_exit(NULL);
        }
    }

    return 0;
}

