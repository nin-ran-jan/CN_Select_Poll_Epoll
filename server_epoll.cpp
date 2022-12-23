#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>

#include <time.h>

#define PORT 8082
#define SIZE 256
#define MAX_CLIENTS 10
#define MAX_EVENTS 10

sem_t mutex;

void printError(char * message){
    perror(message);
    exit(EXIT_FAILURE);
}

long long factorial(long long n){
    if(n == 0 || n == 1) return 1;
    return n*factorial(n-1);
}

int creat_socket()
{
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    int bindResult = bind(server_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (bindResult == -1)
    {
        perror("bindResult");
        exit(EXIT_FAILURE);
    }

    int listenResult = listen(server_socket, 5);
    if (listenResult == -1)
    {
        perror("listenResult");
        exit(EXIT_FAILURE);
    }
    printf("server start\n");
    return server_socket;
}

int main()
{
    clock_t t = clock();
    int listen_sock = creat_socket();

    sem_init(&mutex, 1, 1); //creating a mutex lock to control race conditions

    int outputfd = open("out_epoll.txt", O_CREAT|O_APPEND|O_WRONLY, 0777); //all permissions to file

    struct epoll_event ev, events[MAX_EVENTS];
    int conn_sock, nfds, epollfd;
    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    int client_sockets[MAX_CLIENTS];
    struct sockaddr_in cliaddr;
    int addrlen = sizeof(cliaddr);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_sockets[i] = 0;
    }

    int ctr = 0;

    while (true)
    {
        if(ctr == MAX_CLIENTS) break;
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

        if (nfds == -1)
        {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == listen_sock)
            {

                conn_sock = accept(listen_sock, (sockaddr *)&cliaddr,(socklen_t *)&addrlen);
                if (conn_sock == -1)
                {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }

                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (client_sockets[i] == 0)
                    {
                        client_sockets[i] = conn_sock;
                        ev.events = EPOLLIN | EPOLLET;
                        ev.data.fd = client_sockets[i];
                        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sockets[i], &ev) == -1)
                        {
                            perror("epoll_ctl: conn_sock");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_sockets[i] > 0 && events[n].data.fd == client_sockets[i])
                {
                    char buf[SIZE];
                    bzero(buf, SIZE);
                    int bufSize = read(client_sockets[i], buf, SIZE);
                    // printf("buf %s\n", buf);
                    if (bufSize <= 0)
                    {
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, client_sockets[i], NULL);
                        client_sockets[i] = 0;
                        ctr++;
                    }
                    else
                    {
                        int ret;
                        long long n = atoi(buf);
                        long long result = factorial(n);
                        getpeername(client_sockets[i], (sockaddr *)&cliaddr, (socklen_t *)&addrlen);
                        sprintf(buf, "Factorial: %lld, IP: %s, Port: %d\n", result, inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);

                        sem_wait(&mutex);

                        ret = write(outputfd, buf, strlen(buf));
                        if (ret < 0) printError("Error write to text file");

                        sem_post(&mutex);

                        sprintf(buf, "Factorial: %lld\n", result);
                        ret = write(client_sockets[i], buf, strlen(buf));
                        if (ret < 0) printError("Error writing to client");
                    }
                }
            }
        }
    }

    printf("server end\n");

    sem_destroy(&mutex); //destroying mutex after completion of tasks

    close(listen_sock);

    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Time taken: %f\n", time_taken);

    return 0;
}