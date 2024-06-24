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

typedef struct params{
    int fd;
    fd_set *r_set;
    int* max_fd;
} params;

static pthread_mutex_t mutex;

static void* accept_connection(void *argv){
    pthread_t tid = pthread_self();
    params param = *(params*)argv;
    int lfd = param.fd; 
    fd_set *r_set = param.r_set;
    struct sockaddr_in cli_addr;
    socklen_t cli_socklen = sizeof(cli_addr);
    int cfd = accept(lfd, (struct sockaddr*)&cli_addr, &cli_socklen);
    printf("\nthread %lu \ncfd: %d\n", tid, cfd);
    char ip_buf[128];
    printf("client ip: %s, port: %d\n", inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, ip_buf, sizeof(ip_buf)), ntohs(cli_addr.sin_port));  // arpa/inet.h
    pthread_mutex_lock(&mutex);
    FD_SET(cfd, r_set);
    *param.max_fd = cfd > *param.max_fd ? cfd : *param.max_fd;
    pthread_mutex_unlock(&mutex);
    free(argv);
    return NULL;
}

static void* serve_cli(void *argv){
    pthread_t tid = pthread_self();
    params param = *(params*)argv;
    int cfd = param.fd;
    fd_set *r_set = param.r_set;
    char data_buf[1024] = {0};
    int len = recv(cfd, data_buf, sizeof(data_buf), 0);  // default behavior to recv
    if(len == -1){  // If this were to happen, we should have terminated the whole process by using exit(1). But we didn't do so.
        printf("\nthread %lu has exited: ", tid);
	perror("");
        pthread_mutex_lock(&mutex);
        FD_CLR(cfd, r_set);  // r_set is already a pointer.
        pthread_mutex_unlock(&mutex);
	close(cfd);
	free(argv);
        return NULL;
    }
    if(len == 0){
        printf("\nthread %lu is exited because the cfd %d has been closed.\n", tid, cfd);
        pthread_mutex_lock(&mutex);
        FD_CLR(cfd, r_set);  // r_set is already a pointer.
        pthread_mutex_unlock(&mutex);
        close(cfd);  // unistd.h
      	free(argv);
        return NULL;
    }
    printf("\nthread %lu\ncfd: %d\n", tid, cfd);
    printf("raw data:\n%s\n", data_buf);
    for(int i=0;i<len;i++){
        data_buf[i] = toupper(data_buf[i]);  // ctype.h
    }
    printf("transformerd data:\n%s\n", data_buf);
    send(cfd, data_buf, strlen(data_buf), 0);
    memset(data_buf, 0, sizeof(data_buf));

    free(argv);
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

    fd_set r_set;
    FD_ZERO(&r_set);
    FD_SET(lfd, &r_set);
    int max_fd = lfd; 
    
    pthread_t main_tid = pthread_self();
    pthread_mutex_init(&mutex, NULL);
    while(1){
        printf("\nmain thread %lu is listening...\n", main_tid);
	pthread_mutex_lock(&mutex);	
        fd_set tmp_set = r_set;
	pthread_mutex_unlock(&mutex);
        select(max_fd+1, &tmp_set, NULL, NULL, NULL);  // The last para is timeout.
        int i=0;
	if(FD_ISSET(lfd, &tmp_set)){
	    printf("i: %d, lfd yedaole", i);
	    params* param = malloc(sizeof(params));
	    param->fd = lfd;
	    param->r_set = &r_set;
	    param->max_fd = &max_fd;
	    pthread_t tid;
	    pthread_create(&tid, NULL, accept_connection, param);
	    pthread_detach(tid);
        }
        for(;i<=max_fd;i++){
            if(i!=lfd && FD_ISSET(i, &tmp_set)){
		printf("i: %d", i);
                params* param = malloc(sizeof(params));
                param->fd = i;
                param->r_set = &r_set;
                pthread_t tid;
                pthread_create(&tid, NULL, serve_cli, param);
		pthread_detach(tid);
            }
        }
    }
    pthread_mutex_destroy(&mutex);
    close(lfd);
    return 0;
}
