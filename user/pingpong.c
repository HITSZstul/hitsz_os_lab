#include"kernel/types.h"
#include"user.h"
int main(int argc, char*argv[]){
    if(argc != 1){
        printf("pingpong needs one argument!\n"); 
        exit(-1);
    }
    char buffer[64] = "0";
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
        int a = atoi(buffer);
        printf("%d: received ping from pid %d\n",getpid(),a);
    }
    else{
        wait(&status);
        printf("%d: received pong from pid %d\n",getpid(),pid);
        close(p[0]);
        close(p[1]);
    }
    exit(0);
}