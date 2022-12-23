#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <string.h>
#include <math.h>

#define PORT 8082
#define BUFFER_SIZE 256
#define SEND_LIMIT 20

void printError(char * message){
    perror(message);
    exit(EXIT_FAILURE);
}

int main(){
    int sockfd, ret;
    int port_number = PORT;
    struct sockaddr_in serv_addr;

    char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0); //0 for IP
    if (sockfd < 0) printError("Error opening socket");

    bzero((char *)&serv_addr, sizeof(serv_addr)); //set to 0
    serv_addr.sin_family = AF_INET; //IPv4
    serv_addr.sin_port = htons(port_number);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //to loopback

    ret = connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr));
    if(ret < 0) printError("Error connecting to socket");
    printf("Connected successfully\n");

    int num_to_send = 1;

    while(true){
        if(num_to_send > SEND_LIMIT) break;

        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "%d", num_to_send);
        ret = write(sockfd, buffer, sizeof(buffer));
        if(ret < 0) printError("Error writing to server");

        bzero(buffer, sizeof(buffer));
        ret = read(sockfd, buffer, sizeof(buffer));
        if(ret < 0) printError("Error reading from server");
        printf("%s", buffer);
        num_to_send++;
    }

    close(sockfd);
    printf("Closed connection\n");
    return 0;
}