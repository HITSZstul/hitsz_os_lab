#include"kernel/type.h"
#include"user/user.h"

#define INDEX_READ 0;
#define INDEX_WRITE 1;

void child(int p[]){
    close(p[INDEX_WRITE]);
    int i;
    if(read(p,&i,sizeof(i))==0){
        close(p[INDEX_READ]);
        exit(0);
    }
    printf("prime is %d",i);
    int p_[2];
    pipe(p_);
    int pid;
    if((pid = fork()) == 0){
        child(p_);
    }
    else{
        close(p_[INDEX_READ]);
        int j;
        while(read(p,&j,sizeof(j))!=0){
            if(j%i!=0){
                write(p_,&j,sizeof(j));
            }
        }
        close(p_[INDEX_WRITE]);
        wait(0);
    }
    exit(0);
}

int main(int argc,char* argv[]){
    int p[2];
    pipe(p);
    int pid;
    if((pid = fork()) == 0){
        child(p);
    }else{
        close(p[INDEX_READ]);
        for(int i=2;i<36;i++){
            write(p,&i,sizeof(i));
        }
        close(p[INDEX_WRITE]);
        wait(0);
    }
    exit(0);
}
