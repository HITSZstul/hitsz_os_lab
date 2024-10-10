#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *fmtname(char *path) {
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ) return p;
    memmove(buf, p, strlen(p));//将p的值传给buf，并且未赋值的全部赋值为"\0",方便后续的比较操作
    memset(buf + strlen(p), '\0', DIRSIZ - strlen(p));
    return buf;
}

void find(char* path,char* name){
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }
    
    if (fstat(fd, &st) < 0) {
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch(st.type){
        case T_FILE: //判断当前文件是否为目标文件
            char* file_name;
            file_name = fmtname(path);
            if(strcmp(file_name,name)){
                printf("%s\n",name);
            }
            break;
        case T_DIR:
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';//复制文件路径并一一查找目录下的所有文件或目录
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0) continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if (stat(buf, &st) < 0) {
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                char* file_name = fmtname(buf);
                if(strcmp(file_name,".")==0||strcmp(file_name,"..")==0){
                    continue;
                }
                if(st.type==T_DIR){
                    find(buf,name);
                }
                if(strcmp(fmtname(buf),name)==0){
                    printf("%s\n",buf);
                }
            }
        break;
        close(fd);
    }
}
int main(int argc, char*argv[]){
    if(argc != 3){
        printf("Find needs three argument!\n"); 
        exit(-1);
    }
    for(int i=1;i<argc-1;i++){
        // argv[argc-1] is the target
        find(argv[i],argv[argc-1]);
    }
    exit(0);
}