#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SIZE 6

FILE *fp;
pthread_mutex_t ch_ready_lock[SIZE];
pthread_mutex_t write_file = PTHREAD_MUTEX_INITIALIZER;
int sock, ch_sock[SIZE], cycle = 1;
char name[] = "my.txt";
pthread_t tid[SIZE];
pthread_cond_t cond[SIZE];

void SigintHandler(int sig)
{
    int i;
    cycle = 0;
    for(i = 0; i < SIZE; i++)
    {
        pthread_cond_signal(&cond[i]);
    }
    for(i = 0; i < SIZE; i++)
    {
        pthread_join(tid[i], NULL);
        pthread_cond_destroy(&cond[i]);
        pthread_mutex_destroy(&ch_ready_lock[i]);
    }
    printf("Server out\n");
    fclose(fp);
    close(sock);
    exit(0);
}

int err_handler(int func, const char *errstr, int quantity, int csock)
{
    if(func < 0)
    {
        if(quantity > 0)
        {
            close(csock);
        }
        close(sock);
        perror(errstr);
        exit(-1);
    }
    return func;
}


void *Child_Main(void *ptr)
{
    int i, n, bytes_read;
    int *num = (int*)ptr;
    int csock;
    int buf;
    char buf2[16] = "ready\n";
    printf("Thread %d ready\n", *num);
    
    while(cycle)
    {
        pthread_mutex_lock(&ch_ready_lock[*num]);
        pthread_cond_wait(&cond[*num], &ch_ready_lock[*num]);
        if(ch_sock[*num] != 0)
        {
            csock = ch_sock[*num];
            pthread_mutex_unlock(&ch_ready_lock[*num]);
            err_handler(send(csock, buf2, sizeof(buf2), 0), "send", 1, csock);
            bytes_read = err_handler(recv(csock, &n, sizeof(int), 0), "recv", 1, csock);
            for(i = 0; i < n; i++)
            {
                bytes_read = err_handler(recv(csock, &buf, sizeof(int), 0), "recv", 1, csock);
                printf("%d\n", buf);
                pthread_mutex_lock(&write_file);
                fprintf(fp, "%d", buf);
                pthread_mutex_unlock(&write_file);

            }
            pthread_mutex_lock(&write_file);
            fflush(fp);
            pthread_mutex_unlock(&write_file);

            pthread_mutex_lock(&ch_ready_lock[*num]);
            ch_sock[*num] = 0;
            close(csock);
        }
        pthread_mutex_unlock(&ch_ready_lock[*num]);
    }
}


int main()
{
    struct sigaction sigint;
    sigint.sa_handler = SigintHandler;
    sigint.sa_flags = 0;
    sigemptyset(&sigint.sa_mask);
    sigaddset(&sigint.sa_mask, SIGINT);
    err_handler(sigaction(SIGINT, &sigint, 0), "sigaction", 0, 0);
    if(signal(SIGCHLD, SIG_IGN) == SIG_ERR)
    {
        perror("signal");
        exit(-1);
    }

    int child_sock, i, ch_num = 0;
    int num[SIZE];
    struct sockaddr_in addr, child, parent;
    socklen_t size_p = sizeof(parent), size_c = sizeof(child);
    if ((fp = fopen(name, "w")) == NULL)
    {
        printf("Error open file");
        exit(-1);
    }
    sock = err_handler(socket(AF_INET, SOCK_STREAM, 0), "socket", 0, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    err_handler(bind(sock, (struct sockaddr*)&addr, sizeof(addr)), "bind", 0, 0);
    err_handler(getsockname(sock, (struct sockaddr*)&parent, &size_p), "getsockname", 0, 0);
    printf("Port: %d\n", ntohs(parent.sin_port));

    listen(sock, SIZE);
    for(i = 0; i < SIZE; i++)
    {
        ch_sock[i] = 0;
        num[i] = i;
        pthread_mutex_init(&ch_ready_lock[i], NULL);
        pthread_create(&tid[i], NULL, Child_Main, &num[i]);
    }

    while(cycle)
    { 
        child_sock = err_handler(accept(sock, (struct sockaddr*)&child, &size_c), "accept", 0, 0);
        if(ch_num == SIZE)
            ch_num = 0;
        if(ch_sock[ch_num] == 0)
        {
            pthread_mutex_lock(&ch_ready_lock[ch_num]);
            ch_sock[ch_num] = child_sock;
            err_handler(pthread_cond_signal(&cond[ch_num]), "pthread_cond_signal", 0, 0);
            pthread_mutex_unlock(&ch_ready_lock[ch_num]);
            ch_num++;
        }
    }
}
