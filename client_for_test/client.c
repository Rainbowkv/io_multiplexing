#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }
    
    // create socket of client
    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    
    // set addr of obj server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr.s_addr);
    server_addr.sin_port = htons(atoi(argv[2]));
    
    // connect to server
    if(connect(cfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        perror("");
        exit(1);
    }

    // get self port
    struct sockaddr_in cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);
    getsockname(cfd, (struct sockaddr*)&cli_addr, &cli_addr_len);
    printf("self port: %d\n", ntohs(cli_addr.sin_port));
    char data_buf[1024];
    while(1){
        memset(data_buf, 0, sizeof(data_buf));
        printf("Enter message:\n");
        if(fgets(data_buf, sizeof(data_buf), stdin) != NULL){
            data_buf[strcspn(data_buf, "\n")] = '\0';
        }
        if(strlen(data_buf) == 0){
            printf("Close connection actively.\n");
            break;
        }
        send(cfd, data_buf, strlen(data_buf), 0);
        memset(data_buf, 0, sizeof(data_buf));
        recv(cfd, data_buf, sizeof(data_buf), 0);
        printf("recv data: %s\n", data_buf);
    } 
    
    close(cfd);  // unistd.h
    //strcpy(data_buf, "hello server.");
    
    
    return 0;
}
