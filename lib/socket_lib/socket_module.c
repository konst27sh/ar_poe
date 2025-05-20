//
// Created by sheverdin on 6/24/24.
//

#include "socket_module.h"
#include "time.h"

static SOCKET_ERR_t create_socket(int *socket_fd);
static uint8_t connect_socket(const int *socket_fd);
static void bind_socket(const int *socket_fd);
static void setNonBlockingState(const int *socket_fd);
static void listen_socket(const int *socket_fd);

static int errcheck(int rc, char *funcName);

void socket_test(void)
{
    printf("Hello from SOCKET\n");
}

SOCKET_ERR_t socket_client(int *socket_fd)
{
    SOCKET_ERR_t status = SOCKET_ERR_OK;
    status |= create_socket(socket_fd);
    status |= connect_socket(socket_fd);
    return status;
}

SOCKET_ERR_t socket_asyncServer(int *socket_fd)
{
    SOCKET_ERR_t err_socket = create_socket(socket_fd);
    //setNonBlockingState(socket_fd);
    bind_socket(socket_fd);
    listen_socket(socket_fd);
    return err_socket;
}

static SOCKET_ERR_t create_socket(int *socket_fd)
{
    SOCKET_ERR_t socketErr = SOCKET_ERR_OK;

    //int rc = errcheck(*socket_fd, "socket_lib");
    if (socket(AF_UNIX, SOCK_STREAM, 0) < 0)
    {
        socketErr = SOCKET_ERR_CREATE;
    }
    return socketErr;
}

static uint8_t connect_socket(const int *socket_fd)
{
    struct sockaddr_un serverAddress = {0};
    serverAddress.sun_family = AF_UNIX;
    strcpy(serverAddress.sun_path,SOCKET_PATH);
    int connection = connect(*socket_fd,(const struct sockaddr *)(&serverAddress),sizeof(serverAddress));
    //int rc =  errcheck(connection, "connect_socket");
    if (connection != 0){
        return  SOCKET_ERR_CONNECT;
    }
    else {
        return SOCKET_ERR_OK;
    }
}

//static void setNonBlockingState(const int *socket_fd)
//{
//    const int flags = fcntl(*socket_fd, F_GETFL, 0);
//    int fcntl_rc = fcntl(*socket_fd, F_SETFL, flags | O_NONBLOCK);
//    int rc =  errcheck(fcntl_rc, "connect");
//    if (rc == EXIT_FAILURE){
//        printf("error set non Blocking State \n");
//        exit(1);
//    }
//    else {
//        printf("set non Blocking State successful\n");
//    }
//}

static void bind_socket(const int *socket_fd)
{
    struct sockaddr_un server_addr, client_addr;
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCKET_PATH);
    unlink(SOCKET_PATH);
    int bind_rc = bind(*socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    int rc =  errcheck(bind_rc, "bind");
    if (rc == EXIT_FAILURE){
        printf("Bind socket error\n");
        exit(1);
    }
    else {
        //printf("bind successful\n");
    }
}

static void listen_socket(const int *socket_fd)
{
    int listen_rc = listen(*socket_fd, MAX_CLIENTS);
    int rc =  errcheck(listen_rc, "listen");
    if (rc == EXIT_FAILURE){
        printf("Listen socket error\n");
        exit(1);
    }
    else {
        //printf("Listen successful\n");
    }
}

static int errcheck(int rc, char *funcName)
{
    if (rc == -1) {
        perror(funcName);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void select_socket(fd_set *read_fds, int sock1, int sock2)
{
    FD_ZERO(read_fds);
    FD_SET(sock1, read_fds);
    FD_SET(sock2, read_fds);
    int max_fd = sock1 > sock2 ? sock1 : sock2;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    //printf("Before select - %ld\n", time(NULL));
    long timeNow1 = time(NULL);
    int select_rc= select(max_fd + 1, read_fds, NULL, NULL, &timeout);
    long timeNow2 = time(NULL);
    int rc = errcheck(select_rc, "select");
    if (rc == EXIT_FAILURE)
    {
        exit(EXIT_FAILURE);
    }
    //printf("After select - %ld\n", (timeNow2-timeNow1));
}

SOCKET_ERR_t accept_socket(int server_fd, int *client_fd)
{
    struct sockaddr_un client_addr;
    SOCKET_ERR_t socketErr = SOCKET_ERR_OK;
    socklen_t addr_len = sizeof(client_addr);
    //printf("accept_socket\n");
    *client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &addr_len);
    if (*client_fd == -1)
    {
        perror("accept");
        socketErr = SOCKET_ERR_CREATE;
    }
    return socketErr;
}

void send_msg(int socket, unsigned char *msg, uint16_t size)
{
    //printf(" **************************   send_msg size = %d\n", size);
    //for (int i = 0; i < size; i++)
    //{
    //    printf("send msg i = %d -> msg_c[i] - %d -> msg_x[i] - %x \n",i, msg[i], msg[i]);
    //}

    write(socket,msg,size);
}

void d_send_msg(int socket, unsigned char *msg, uint16_t size)
{
    //printf(" **************************   send_msg size = %d\n", size);
    //for (int i = 0; i < size; i++)
    //{
    //    printf("send msg i = %d -> msg_c[i] - %d -> msg_x[i] - %x \n",i, msg[i], msg[i]);
    //}

    write(socket,msg,size);
}

void  ctrl_send_msg(int socket, unsigned char *msg, uint16_t size)
{
    //printf(" **************************   send_msg size = %d\n", size);
    //for (int i = 0; i < size; i++)
    //{
    //    printf("send msg i = %d -> msg_c[i] - %d -> msg_x[i] - %x \n",i, msg[i], msg[i]);
    //}

    write(socket,msg,size);
}

CLIENT_STATE d_receive_msg(int socket_fd, unsigned char *rec_msg)
{
    CLIENT_STATE clientState = SOCKET_READ_ERROR;
    unsigned char msg[5];
    int readByte = (int) read(socket_fd,  msg, 5);
    if (readByte < 0 )
    {
        clientState = SOCKET_READ_ERROR;
        close(socket_fd);
    }
    else if(readByte == 0)
    {
        clientState = SOCKET_DISCONNECTED;
    }
    else
    {
        clientState = SOCKET_READ_OK;
        memcpy(rec_msg, msg, 5);
    }
    return clientState;
}

CLIENT_STATE ctrl_receive_msg(int socket_fd, unsigned char *rec_msg, uint16_t nbytes)
{
    CLIENT_STATE clientState = SOCKET_READ_ERROR;
    unsigned char msg[nbytes];
    int readByte = (int) read(socket_fd,  msg, nbytes);
    if (readByte < 0 )
    {
        clientState = SOCKET_READ_ERROR;
        close(socket_fd);
    }
    else if(readByte == 0)
    {
        clientState = SOCKET_DISCONNECTED;
    }
    else
    {
        clientState = SOCKET_READ_OK;
        memcpy(rec_msg, msg,     sizeof(msg));
    }
    return clientState;
}


