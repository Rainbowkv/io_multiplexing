#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<unistd.h>

#define SERVER_PORT 7777

int main(){
    // apply for a fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);  // sys/socket.h
    
    printf("lfd: %d\n", lfd);
    // generate a addr of server
    struct sockaddr_in server_addr;  // netinet/in.h
    memset(&server_addr, 0, sizeof(server_addr));  // string.h
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    
    // bind addr to fd
    bind(lfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // set state of lfd from CLOSE to LISTEN
    listen(lfd, 64);
    printf("port: %d, waiing to be connected...\n", ntohs(server_addr.sin_port));
    

    while(1){
        struct sockaddr_in cli_addr;
        socklen_t cli_socklen = sizeof(cli_addr);
        int cfd = accept(lfd, (struct sockaddr*)&cli_addr, &cli_socklen);  // blocking to wait
        printf("cfd: %d\n", cfd);
        char ip_buf[128];
        printf("client ip: %s, port: %d\n", inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, ip_buf, sizeof(ip_buf)), ntohs(cli_addr.sin_port));  // arpa/inet.h
        char data_buf[1024] = {0};
        while(1){
            int len = recv(cfd, data_buf, sizeof(data_buf), 0);  // default behavior to recv
            if(len == -1){
                perror("");
                exit(1);
            }
            if(len == 0){
                printf("Current client connection has closed.\n");
                break;
            }
            printf("raw data:\n%s\n", data_buf);
            for(int i=0;i<len;i++){
                data_buf[i] = toupper(data_buf[i]);  // ctype.h
            }
            printf("transformerd data:\n%s\n", data_buf);
            send(cfd, data_buf, strlen(data_buf), 0);
            memset(data_buf, 0, sizeof(data_buf));
        }
        close(cfd);  // unistd.h
    }
    close(lfd);
    
    return 0;
}
