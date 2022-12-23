#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>

#include <string.h>
#include <math.h>

#include <time.h>

#define DATA_BUFFER 256
#define MAX_CONNECTIONS 11
#define PORT 8082

sem_t mutex;

void printError(char * message){
    perror(message);
    exit(EXIT_FAILURE);
}

long long factorial(long long n){
    if(n == 0 || n == 1) return 1;
    return n*factorial(n-1);
}

int create_tcp_server_socket(){
    struct sockaddr_in saddr;
    int fd, ret;

    //TCP socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) printError("Socket failed");

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(PORT);
    saddr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(fd, (sockaddr *) &saddr, sizeof(sockaddr_in));
    if (ret < 0) printError("Bind failed");

    ret = listen(fd, 1);
    if (ret < 0) printError("Listen failed");

    return fd;

}

int main(){

    clock_t t = clock();

    fd_set read_fd_set;
    struct sockaddr_in new_addr;
    int server_fd, new_fd, ret, i, outputfd;
    socklen_t addrlen;
    char buf[DATA_BUFFER];
    int all_connections[MAX_CONNECTIONS];

    sem_init(&mutex, 1, 1); //creating a mutex lock to control race conditions

    server_fd = create_tcp_server_socket();
    if (server_fd < 0) printError("Server creation failed");

    for(i = 0; i < MAX_CONNECTIONS; i++){
        all_connections[i] = -1;
    }
    all_connections[0] = server_fd;

    outputfd = open("out_select.txt", O_CREAT|O_APPEND|O_WRONLY, 0777); //all permissions to file

    int ctr = 0;

    while(true){
        if(ctr == MAX_CONNECTIONS-1)break;
        FD_ZERO(&read_fd_set);

        for(int i = 0; i < MAX_CONNECTIONS; i++){
            if(all_connections[i] >= 0){
                FD_SET(all_connections[i], &read_fd_set);
            }
        }

        // exit(EXIT_FAILURE);
        ret = select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);

        if(ret >= 0){
            if(FD_ISSET(server_fd, &read_fd_set)){
                new_fd = accept(server_fd, (sockaddr *) &new_addr, (socklen_t*)&addrlen); //for server
                if(new_fd >= 0){
                    for(i = 1; i < MAX_CONNECTIONS; i++){
                        if(all_connections[i] < 0){
                            all_connections[i] = new_fd;
                            break;
                        }
                    }
                }
                else{
                    printError("Accept failed");
                }
                ret--;
                if(!ret) continue;
            }

            for(i = 1; i < MAX_CONNECTIONS; i++){

                bzero(buf, DATA_BUFFER);
                if((all_connections[i] > 0) && FD_ISSET(all_connections[i], &read_fd_set)){

                    ret = read(all_connections[i], buf, DATA_BUFFER);
                    if (ret < 0) printError("recv() failed");
                    else if (ret == 0){
                        printf("Closing connection\n");
                        close(all_connections[i]);
                        all_connections[i] = -1;
                        ctr++;
                    }
                    else{
                        
                        long long n = atoi(buf);
                        long long result = factorial(n);
                        getpeername(all_connections[i], (sockaddr *)&new_addr, (socklen_t *)&addrlen);
                        sprintf(buf, "Factorial: %lld, IP: %s, Port: %d\n", result, inet_ntoa(new_addr.sin_addr), new_addr.sin_port);

                        sem_wait(&mutex);

                        ret = write(outputfd, buf, strlen(buf));
                        if (ret < 0) printError("Error write to text file");

                        sem_post(&mutex);

                        sprintf(buf, "Factorial: %lld\n", result);
                        ret = write(all_connections[i], buf, strlen(buf));
                        if (ret < 0) printError("Error writing to client");
                    }
                }
                ret--;
                if(!ret) continue;
            }
        }
        else printError("Error selecting");
    }

    for (i = 0; i < MAX_CONNECTIONS; i++){
        if(all_connections[i] > 0){
            close(all_connections[i]);
        }
    }

    sem_destroy(&mutex); //destroying mutex after completion of tasks

    t = clock() - t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Time taken %f\n", time_taken);

    return 0;
}