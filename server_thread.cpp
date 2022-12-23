#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <string.h>
#include <math.h>

#include <time.h>

#define PORT 8082
#define BUFFER_SIZE 256
#define SEND_LIMIT 20
#define MAX_REQUESTS 10

sem_t mutex;

void printError(char * message){
    perror(message);
    exit(EXIT_FAILURE);
}

long long factorial(long long n){
    if(n == 0 || n == 1) return 1;
    return n*factorial(n-1);
}

void *threadCalculation(void *vargs){

    int newsockfd = *(int *) vargs;

    char buffer[BUFFER_SIZE];
    int ret, outputfd;

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    ret = getpeername(newsockfd, (sockaddr *) &cli_addr, &clilen);

    outputfd = open("out_thread.txt", O_CREAT|O_APPEND|O_WRONLY, 0777); //all permissions to file
    if (outputfd < 0) printError("Error text file opening");

    while(true){
        bzero(buffer, BUFFER_SIZE);
        ret = read(newsockfd, buffer, BUFFER_SIZE);
        if (ret < 0) printError("Error reading from client");
        else if (ret == 0) break;
        else{
            long long n = atoi(buffer);
            long long result = factorial(n);
            sprintf(buffer, "Factorial: %lld, IP: %s, Port: %d\n", result, inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);

            sem_wait(&mutex);

            ret = write(outputfd, buffer, strlen(buffer));
            if (ret < 0) printError("Error write to text file");
            // printf("%s", buffer);

            sem_post(&mutex);

            sprintf(buffer, "Factorial: %lld\n", result);
            ret = write(newsockfd, buffer, strlen(buffer));
            if (ret < 0) printError("Error writing to client");
        }
    }

    return NULL;
}

int main(){
    clock_t t;
    t = clock(); 
    int sockfd, newsockfd, ret;
    int port_number = PORT;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    sem_init(&mutex, 1, 1); //creating a mutex lock to control race conditions

    sockfd = socket(AF_INET, SOCK_STREAM, 0); //0 for IP
    if (sockfd < 0) printError("Error opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr)); //set to 0

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_number);

    if ((bind(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) printError("Error binding");

    if(listen(sockfd, MAX_REQUESTS+1) < 0) printError("Not listening");

    int clients_accepted = 0;
    clilen = sizeof(cli_addr);

    while(true){

        newsockfd = accept(sockfd, (sockaddr *) &cli_addr, &clilen);
        if(newsockfd >= 0){
            clients_accepted++;
            printf("Connected successfully with client %d\n", clients_accepted);
            
            pthread_t thread_id;
            int *pointer = new int;
            *pointer = newsockfd;
            pthread_create(&thread_id, NULL, threadCalculation, (void *) pointer);
        }
        
        if(clients_accepted >= MAX_REQUESTS) break;
    }

    sem_destroy(&mutex); //destroying mutex after completion of tasks

    close(sockfd);
    printf("Closed connection\n");
    t = clock()-t;
    double time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Time taken: %f\n", time_taken);
    pthread_exit(NULL);
    return 0;
}