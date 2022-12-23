#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <math.h>

#define PORT 8080
#define BUFFER_SIZE 256
#define SEND_LIMIT 20

void printError(char * message){
    perror(message);
    exit(EXIT_FAILURE);
}

long long factorial(long long n){
    if(n == 0 || n == 1) return 1;
    return n*factorial(n-1);
}

int main(){
    int sockfd, newsockfd, outputfd, ret;
    int port_number = PORT;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0); //0 for IP
    if (sockfd < 0) printError("Error opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr)); //set to 0

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_number);

    if ((bind(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) printError("Error binding");

    if(listen(sockfd, 1) < 0) printError("Not listening");

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (sockaddr *) &cli_addr, &clilen);
    if(newsockfd < 0) printError("Error to accept");

    printf("Connected successfully\n");

    outputfd = open("out_seq.txt", O_CREAT|O_TRUNC|O_WRONLY, 0777); //providing all permissions to file
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
            ret = write(outputfd, buffer, strlen(buffer));
            if (ret < 0) printError("Error write to text file");
            sprintf(buffer, "Factorial: %lld\n", result);
            ret = write(newsockfd, buffer, strlen(buffer));
            if (ret < 0) printError("Error writing to client");
        }
    }

    close(sockfd);
    close(outputfd);
    printf("Closed connection\n");
    return 0;
}