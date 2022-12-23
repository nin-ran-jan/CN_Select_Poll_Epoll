#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <poll.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

#define PORT 8082
#define SIZE 256
#define MAX_CLIENTS 11

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

int wait_client(int server_socket)
{
    struct pollfd pollfds[MAX_CLIENTS + 1];

    int outputfd = open("out_poll.txt", O_CREAT|O_APPEND|O_WRONLY, 0777); //all permissions to file

    pollfds[0].fd = server_socket;
    pollfds[0].events = POLLIN;
    int useClient = 0;

    for(int i = 1; i < MAX_CLIENTS; i++){
        pollfds[i].fd = 0;
        pollfds[i].events = POLLIN;
    }

    struct sockaddr_in cliaddr;
    int addrlen = sizeof(cliaddr);

    int ctr = 0;

    while (true)
    {
        if(ctr == MAX_CLIENTS-1) break;
        // printf("useClient => %d\n", useClient);
        int pollResult = poll(pollfds, MAX_CLIENTS+1, 5000);
        if (pollResult > 0)
        {
            if (pollfds[0].revents & POLLIN)
            {
                
                int client_socket = accept(server_socket, (struct sockaddr *)&cliaddr,(socklen_t *)&addrlen);
                // printf("accept success %s\n", inet_ntoa(cliaddr.sin_addr));
                if(client_socket >= 0){
                    for (int i = 1; i < MAX_CLIENTS; i++)
                    {
                        if (pollfds[i].fd == 0)
                        {
                            pollfds[i].fd = client_socket;
                            pollfds[i].events = POLLIN;
                            useClient++;
                            break;
                        }
                    }
                }
                else{
                    printError("Accept failed");
                }
                pollResult--;
                if(!pollResult) continue;
            }
            for (int i = 1; i < MAX_CLIENTS; i++)
            {
                if (pollfds[i].fd > 0 && pollfds[i].revents & POLLIN)
                {
                    char buf[SIZE];
                    int bufSize = read(pollfds[i].fd, buf, SIZE);
                    if (bufSize <= 0)
                    {
                        pollfds[i].fd = 0;
                        pollfds[i].events = 0;
                        pollfds[i].revents = 0;
                        useClient--;
                        ctr++;
                    }
                    else
                    {
                        buf[bufSize] = '\0';
                        // printf("From client: %s\n", buf);
                        int ret;
                        long long n = atoi(buf);
                        long long result = factorial(n);
                        getpeername(pollfds[i].fd, (sockaddr *)&cliaddr, (socklen_t *)&addrlen);
                        sprintf(buf, "Factorial: %lld, IP: %s, Port: %d\n", result, inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                        sem_wait(&mutex);
                        ret = write(outputfd, buf, strlen(buf));
                        if (ret < 0) printError("Error write to text file");
                        sem_post(&mutex);

                        sprintf(buf, "Factorial: %lld\n", result);
                        ret = write(pollfds[i].fd, buf, strlen(buf));
                        if (ret < 0) printError("Error writing to client");
                    }
                }
            }
        }
    }
}


int main()
{
    clock_t t = clock();
    int server_socket = creat_socket();

    sem_init(&mutex, 1, 1); //creating a mutex lock to control race conditions

    int client_socket = wait_client(server_socket);

    printf("server end\n");

    close(client_socket);
    close(server_socket);

    sem_destroy(&mutex); //destroying mutex after completion of tasks

    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Time taken: %f\n", time_taken);

    return 0;
}