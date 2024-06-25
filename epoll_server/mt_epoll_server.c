#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<pthread.h>

#define SERVER_PORT 7777

static pthread_mutex_t mutex;

struct params{
    int fd;
    int epfd;
};

static void* accept_connection(void *ptr){
    pthread_t tid = pthread_self();
    struct params *param = (struct params*)ptr;
    struct sockaddr_in cli_addr;
    socklen_t cli_socklen = sizeof(cli_addr);
    int cfd = accept(param->fd, (struct sockaddr*)&cli_addr, &cli_socklen);

    // add cfd to fds
    struct epoll_event epev;
    epev.data.fd = cfd;
    epev.events = EPOLLIN | EPOLLET;

    // manipulate the shared resource.
    pthread_mutex_lock(&mutex);
    epoll_ctl(param->epfd, EPOLL_CTL_ADD, cfd, &epev);
    pthread_mutex_unlock(&mutex);

    char ip_buf[128];
    printf("\nthread %lx\ncfd: %d, client ip: %s, port: %d\n", tid, cfd, inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, ip_buf, sizeof(ip_buf)), ntohs(cli_addr.sin_port));  // arpa/inet.h
    
    free(param);
    return NULL;
}

static void* serve_cli(void *ptr){
    pthread_t tid = pthread_self();
    struct params *param = (struct params*)ptr;
    char data_buf[1024] = {0};
    int len = recv(param->fd, data_buf, sizeof(data_buf), 0);  // default behavior to recv
    if(len == -1 || len == 0){  // If len == -1 were to happen, we should have terminated the whole process by using exit(1). But we didn't do so.
	    if(len == -1) 
            perror("");
        else 
            printf("\nthread %lx detected the cfd %d has been closed.\n", tid, param->fd);
        // remove cfd from fds
        // manipulate the shared resource.
        pthread_mutex_lock(&mutex);
        epoll_ctl(param->epfd, EPOLL_CTL_DEL, param->fd, NULL);
        pthread_mutex_unlock(&mutex);
	    close(param->fd);
        free(param);
        return NULL;
    }

    printf("\nthread %lx\ncfd: %d, raw data: %s\n", tid, param->fd, data_buf);
    for(int i=0;i<len;i++)
        data_buf[i] = toupper(data_buf[i]);  // ctype.h
    printf("\nthread %lx\ntransformerd data:\n%s\n", tid, data_buf);
    send(param->fd, data_buf, strlen(data_buf), 0);
    memset(data_buf, 0, sizeof(data_buf));
    
    free(param);
    return NULL;
}


int main(){
    pthread_mutex_init(&mutex, NULL);
    // apply for a fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);  // sys/socket.h

    // set portplexing, what happened? did't solute our problem.
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
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

    // set the data struct of epoll
    int epfd = epoll_create(1);  // The para > 0 is right, will be ignored.
    struct epoll_event epev;
    epev.data.fd = lfd;
    epev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &epev);
    
#define TMP_SIZE 1024
    struct epoll_event revents[TMP_SIZE]; 
    pthread_t main_tid = pthread_self();
    while(1){
        // A terminal hint.
        printf("\nthread %lx is listening...\n", main_tid);
        
        // Reset is not needed in epoll.
       
        // Epoll I/O Model
        int num = epoll_wait(epfd, revents, TMP_SIZE, -1);        

        // Process different affair.
        for(int i=0;i<num;i++){
            struct params *param = (struct params*)malloc(sizeof(struct params));
            param->fd = revents[i].data.fd;
            param->epfd = epfd;
            pthread_t tid;
            if(revents[i].data.fd == lfd)
                pthread_create(&tid, NULL, accept_connection, param);
            else
                pthread_create(&tid, NULL, serve_cli, param);
            pthread_detach(tid);
        }
    }
    // Close listen fd.
    close(lfd);
    return 0;
}
