#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
int strncmp(const char *p, const char *q, uint n) {
  while (n > 0 && *p && *p == *q) n--, p++, q++;
  if (n == 0) return 0;
  return (uchar)*p - (uchar)*q;
}
char *fmtname(char *path) {
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ) return p;
    memmove(buf, p, strlen(p));//将p的值传给buf，并且未赋值的全部赋值为空格
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
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
        case T_FILE:
            char* file_name;
            file_name = fmtname(path);
            if(strncmp(file_name,name,strlen(name))==0){
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
            *p++ = '/';
            // printf("%s\n",buf);
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0) continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if (stat(buf, &st) < 0) {
                    printf("ls: cannot stat %s\n", buf);
                    continue;
                }
                char* file_name = fmtname(buf);
                
                printf("path = %s\t",path);
                // if(strncmp(path,".",strlen(path))!=0){
                //     
                printf("file_name = %s\n",file_name);
                // }
                if(strncmp(file_name,".",1)==0){
                    continue;
                }
                // printf("%d\n",st.type);
                // printf("%s\t",file_name);
                // printf("buff = %s\n",buf);
                if(st.type==T_DIR){
                    find(buf,name);
                }
                // printf("filename = %s,name = %s\n",file_name,name);
                if(strncmp(fmtname(buf),name,strlen(name))==0){
                    printf("%s\n",buf);
                }
                // printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
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