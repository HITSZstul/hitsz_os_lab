#include"kernel/types.h"
#include"user.h"
int main(int argc, char*argv[]){
    if(argc != 1){
        printf("pingpong needs one argument!\n"); 
        exit(-1);
    }
    char buffer[64];
    int p[2];
    int pid=getpid();
    int status;
    pipe(p);
    itoa(pid,buffer);
    write(p[1],buffer,sizeof(buffer));

    if((pid = fork())<0){
        printf("CREATR CHILD PROCCESS ERROR!");
    }
    if(pid==0){
        close(0);
        dup(p[0]);
        close(p[0]);
        read(p[0],buffer,sizeof(buffer));
        printf("%d:receive ping from %s\n",getpid(),buffer);
    }
    else{
        wait(&status);
        printf("%d receive pong from %d",getpid(),pid);
        close(p[0]);
        close(p[1]);
    }
    exit(0);
}