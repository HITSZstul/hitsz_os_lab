#include"kernel/types.h"
#include"user.h"
int main(int argc, char*argv[]){
    if(argc != 1){
        printf("pingpong needs one argument!\n"); 
        exit(-1);
    }
    char buffer[64] = "0";
    int p[2],pc[2];
    // int pid=getpid();
    int status;

    pipe(p);
    pipe(pc);

    // itoa(pid,buffer);
    // write(p[1],buffer,sizeof(buffer));
    int pid;
    if((pid = fork())<0){
        printf("CREATR CHILD PROCCESS ERROR!");
        exit(1);
    }
    if(pid==0){
        // close(0);//关闭标准读取流std
        // dup(p[0]);//复制读取管道地址
        //dup 系统调用的主要作用是创建一个新的文件描述符，该描述符引用与原文件描述符相同的打开文件。
        // close(p[0]);//将重复的管道关闭
        //don't need write to father pipe;
        close(p[1]);
        // read from father
        read(p[0],buffer,sizeof(buffer));
        int a = atoi(buffer);
        printf("%d: received ping from pid %d\n",getpid(),a);
        itoa(getpid(),buffer);
        write(pc[1],buffer,sizeof(buffer));
    }
    else{
        //father don't need read
        close(p[0]);
        close(pc[1]);
        itoa(getpid(),buffer);
        write(p[1],buffer,sizeof(buffer));
        close(p[1]);
        wait(&status);
        //receive from child
        read(pc[0],buffer,sizeof(buffer));
        int child_pid = atoi(buffer);
        printf("%d: received pong from pid %d\n",getpid(),child_pid);
        close(pc[0]);
        
    }
    exit(0);
}