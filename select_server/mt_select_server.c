#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<unistd.h>
#include<pthread.h>

#define SERVER_PORT 7777

typedef struct serve_cli_para{
    int cfd;
    fd_set *r_set;
}serve_cli_para;

void serve_cli(void *argv){
    pthread_t tid = pthread_self();
    serve_cli_para param = *(serve_cli_para*)argv;
    int fd = param.fd;
    fd_set *r_set = param.r_set;
    char data_buf[1024] = {0};
    while(1){
        int len = recv(cfd, data_buf, sizeof(data_buf), 0);  // default behavior to recv
        if(len == -1){
            perror("");
            exit(1);
        }
        if(len == 0){
            printf("Current client connection has closed, thread %lu is exited.\n", tid);
            FD_CLR(cfd, r_set);  // r_set is already a pointer.
            close(cfd);  // unistd.h
            pthread_exit(NULL);
            break;
        }
        printf("thread %lu -> raw data:\n%s\n", tid, data_buf);
        for(int i=0;i<len;i++){
            data_buf[i] = toupper(data_buf[i]);  // ctype.h
        }
        printf("thread %lu -> transformerd data:\n%s\n", tid, data_buf);
        send(cfd, data_buf, strlen(data_buf), 0);
        memset(data_buf, 0, sizeof(data_buf));
    }
}

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

    fd_set r_set, tmp_set;
    FD_ZERO(&r_set);
    FD_SET(lfd, &r_set);
    int max_fd = lfd; 
    
    pthread_t main_tid = pthread_self();
    while(1){
        printf("main thread %lu is listening and dispatching...", main_tid);
        tmp_set = r_set;
        select(max_fd+1, &tmp_set, NULL, NULL, NULL);  // The last para is timeout.
        if(FD_ISSET(lfd, &tmp_set)){
            struct sockaddr_in cli_addr;
            socklen_t cli_socklen = sizeof(cli_addr);
            int cfd = accept(lfd, (struct sockaddr*)&cli_addr, &cli_socklen);
            printf("cfd: %d\n", cfd);
            char ip_buf[128];
            printf("client ip: %s, port: %d\n", inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, ip_buf, sizeof(ip_buf)), ntohs(cli_addr.sin_port));  // arpa/inet.h
            FD_SET(cfd, &r_set);
            max_fd = cfd > max_fd ? cfd : max_fd;
        }
        for(int i=0;i<=max_fd;i++){
            if(i!=lfd && FD_ISSET(i, &r_set)){
                serve_cli_para params;
                params.fd = i;
                params.r_set = &r_set;
                pthread_t tid;
                pthread_create(%tid, NULL, serve_cli, &params);
            }
        }
    }
    close(lfd);
    return 0;
}
