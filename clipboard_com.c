#include "clipboard_com.h"

extern char *remote_ip; //remote ip
extern int remote_port; //remote port

extern int socket_fd_inet_local; //socket for inet com
extern int socket_fd_inet_remote;
extern int socket_fd_unix;
extern int socket_fd_inet_child;

extern struct sockaddr_in local_addr;
extern struct sockaddr_in remote_addr;
extern struct sockaddr_un clipboard_addr;
extern struct sockaddr_un client_addr;

extern bool connected_mode;
extern char *local_ip;
extern int local_port;
extern int uport;

/*
@Name: configure_unix_com()
@Args: None;
@Desc: Configues sockets for unix domains communication;
@Return: (void);
*/

void configure_unix_com()
{
    clipboard_addr.sun_family = AF_UNIX;
    strcpy(clipboard_addr.sun_path, CLIPBOARD_SOCKET);

    //Create socket for local clipboard communication (UNIX DATASTREAM)
    unlink(CLIPBOARD_SOCKET);
    socket_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_fd_unix == -1)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    //Bind socket to address

    if (bind(socket_fd_unix, (struct sockaddr *)&clipboard_addr, sizeof(clipboard_addr)) == -1)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    if (listen(socket_fd_unix, 1) == -1)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }
}

/*
@Name: configure_inet_local_com()
@Args: None;
@Desc: Configues sockets for the clipboard to accept remote connections;
@Return: (void);
*/

void configure_inet_local_com()
{
    int local_port;
    if (uport == 0)
    {
        srand(time(NULL)); //Use time as seeder for now, maybe change for getpid?
        local_port = rand() % (64738 - 1024) + 1024;
    }
    else
        local_port = uport;

    socket_fd_inet_local = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_inet_local == -1)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd_inet_local, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in)) == -1)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    if (listen(socket_fd_inet_local, 2) == -1)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    printf("Port:%d\n", local_port);
}
/*
@Name: configure_inet_remote_com()
@Args: None;
@Desc: Configues sockets for internet domains communication;
@Return: (void);
*/

void configure_inet_remote_com()
{
    socket_fd_inet_remote = socket(AF_INET, SOCK_STREAM, 0);
    socket_fd_inet_child = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_inet_remote == -1 || socket_fd_inet_child == -1)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);

    if (inet_aton(remote_ip, &remote_addr.sin_addr) == 0)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    if (connect(socket_fd_inet_remote, (const struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) == -1 ||
        connect(socket_fd_inet_child, (const struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) == -1)
    {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }
}
