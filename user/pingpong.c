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
        close(0);//关闭标准读取流std
        dup(p[0]);//复制读取管道地址
        //dup 系统调用的主要作用是创建一个新的文件描述符，该描述符引用与原文件描述符相同的打开文件。
        close(p[0]);//将重复的管道关闭
        read(0,buffer,sizeof(buffer));
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