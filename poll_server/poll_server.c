#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<unistd.h>
#include<poll.h>

#define SERVER_PORT 7777


static void* accept_connection(int lfd, struct pollfd *fds){
    struct sockaddr_in cli_addr;
    socklen_t cli_socklen = sizeof(cli_addr);
    int cfd = accept(lfd, (struct sockaddr*)&cli_addr, &cli_socklen);
    // add cfd to fds
    fds[cfd].fd = cfd;
    printf("cfd: %d\n", cfd);
    char ip_buf[128];
    printf("client ip: %s, port: %d\n", inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, ip_buf, sizeof(ip_buf)), ntohs(cli_addr.sin_port));  // arpa/inet.h
    return NULL;
}

static void* serve_cli(int cfd, struct pollfd *fds){
    char data_buf[1024] = {0};
    int len = recv(cfd, data_buf, sizeof(data_buf), 0);  // default behavior to recv
    if(len == -1 || len == 0){  // If len == -1 were to happen, we should have terminated the whole process by using exit(1). But we didn't do so.
	    if(len == -1) 
            perror("");
        else 
            printf("Detected the cfd %d has been closed.\n", cfd);
        // remove cfd from fds
        fds[cfd].fd = -1;
	    close(cfd);
        return NULL;
    }

    printf("\ncfd: %d\n", cfd);
    printf("raw data:\n%s\n", data_buf);
    for(int i=0;i<len;i++){
        data_buf[i] = toupper(data_buf[i]);  // ctype.h
    }
    printf("transformerd data:\n%s\n", data_buf);
    send(cfd, data_buf, strlen(data_buf), 0);
    memset(data_buf, 0, sizeof(data_buf));

    return NULL;
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

    // set the data struct of poll
    struct pollfd fds[FD_SETSIZE];  // This is the limit of the descriptor table of process, instead of poll.
    for(int i=0;i<FD_SETSIZE;i++){
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }
    fds[0].fd = lfd;
    
    while(1){
        // A terminal hint.
        printf("Main thread is listening...\n");
        
        // Reset is not needed in poll.
        
        // Poll I/O Model
        poll(fds, FD_SETSIZE, -1);  // The last para is timeout, -1 indicates unlimited.
        
        // Process connection.
        if(fds[0].revents & POLLIN)
           accept_connection(fds[0].fd, fds); 

        // Serve client.
        for(int i=1;i<FD_SETSIZE;i++)
            if(fds[i].revents & POLLIN)
                serve_cli(fds[i].fd, fds);
    }
    // Close listen fd.
    close(lfd);
    return 0;
}
