#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


int err_handler(int func, const char *errstr, int quantity, int *sock)
{
    if(func < 0)
    {
        if(quantity > 0)
        {
            int i;
            for(i = 0; i < quantity; i++)
                close(sock[i]);
        }
        perror(errstr);
        exit(-1);
    }
    return func;
}


int main(int argc, char **argv)
{
    int sock, bytes_read, j, i, n;
    char buf[16];
    struct sockaddr_in addr;
    if (argc < 3)
    {
        printf("Usage:\n./client <IP> <port> <i> <n>\n");
        exit(0);
    }
    i = atoi(argv[3]);
    n = atoi(argv[4]);
    sock = err_handler(socket(AF_INET, SOCK_STREAM, 0), "socket", 0, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    err_handler(connect(sock, (struct sockaddr *)&addr, sizeof(addr)), "connect", 1, &sock);
    bytes_read = err_handler(recv(sock, buf, 16, 0), "recv", 1, &sock);
    printf("%s\n", buf);
    err_handler(send(sock, &n, sizeof(n), 0), "send", 1, &sock);
    for(j = 0; j < n; j++)
    {
        printf("%d\n", i);
        err_handler(send(sock, &i, sizeof(i), 0), "send", 1, &sock);
        sleep(i);
    }
    close(sock);
    exit(0);
}
