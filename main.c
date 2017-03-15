#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

void query();
void currentSocketPorcess();
void freeList();
int isDirectory(const char *);
void readFile(char *, char *);

typedef struct element{
    char protocol[6];
    char localAddress[65];
    char foreignAddress[65];
    char pidNameArguments[256];
    struct element *next;
}LISTELEMENT;
LISTELEMENT *lHead = NULL;
LISTELEMENT *lTail = NULL;

typedef struct pElement{
    char pid[20]; 
    char nameArguments[256];
    char inodeID[20];
    struct pElement *next;
}PIDINFO;
PIDINFO *pHead = NULL;
PIDINFO *pTail = NULL;

int main(int argc, char *argv[]){
    int flag = 0;
    if (setuid(0)){
        fprintf(stderr, "setuid error\n");
        return 1;
    }
    else{
        currentSocketPorcess();
        char * const short_options = "tu";
        const struct option long_options[] = {
            {"tcp", 0, NULL,'t'},
            {"udp", 0, NULL,'u'},
            {NULL, 0, NULL, 0}
        };
        int c;
        while((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
            switch(c){
                case 't':
                    query(0, 0);
                    query(0, 1);
                    flag = 1;
                    break;
                case 'u':
                    query(1, 0);
                    query(1, 1);
                    flag = 1;
                    break;
            }
        }
    }
    if(flag == 1){
        LISTELEMENT *t = malloc(sizeof(LISTELEMENT));
        strcpy(t -> protocol, "Proto");
        strcpy(t -> localAddress, "LocalAddress");
        strcpy(t -> foreignAddress, "ForeignAddress");
        strcpy(t -> pidNameArguments, "PID/Program name and arguments");
        t -> next = lHead;
        lHead = t;
        LISTELEMENT *i; 
        for( i = lHead; i != NULL; i = i -> next)
            printf("%-10s %-45s %-45s %-100s\n", i -> protocol, 
                    i -> localAddress, i -> foreignAddress, i -> pidNameArguments);
    }
    freeList();
    return 0;
}
void query(int TCPUDP, int version){
    FILE *fptr;
    char lineBuf[1000];
    if(!TCPUDP){
        if(!version)
            fptr = fopen("/proc/net/tcp","r");
        else
            fptr = fopen("/proc/net/tcp6","r");
    }
    else{
        if(!version)
            fptr = fopen("/proc/net/udp","r");
        else
            fptr = fopen("/proc/net/udp6","r");
    }
    LISTELEMENT *curr = NULL;
    fgets(lineBuf, 1000, fptr);//skip first line
    while(fgets(lineBuf, 1000, fptr) != NULL){
        curr = malloc(sizeof(LISTELEMENT));
        curr -> next = NULL;
        memset(curr, 0, sizeof(LISTELEMENT));
        if(!TCPUDP){
            if(!version)
                strcpy(curr -> protocol,"tcp");
            else
                strcpy(curr -> protocol,"tcp6");
        }
        else{
            if(!version)
                strcpy(curr -> protocol,"udp");
            else
                strcpy(curr -> protocol,"udp6");
        }
        if(!lHead){
            lHead = curr;
            lTail = lHead;
        }
        else{
            lTail -> next = curr;
            lTail = lTail -> next;
        }
        int i = 0;
        char *token;
        char *delim = " ";
        token = strtok(lineBuf,delim);
        while (token != NULL){
            if(i == 1)
                strcpy(curr -> localAddress, token);
            else if(i == 2)
                strcpy(curr -> foreignAddress, token);
            else if(i == 9){
                
                //printf("%s\n", token);
                //inode2Pid(colBuf);
                //strcpy(curr -> localAdderss, );
                PIDINFO *i;
                for( i = pHead; i != NULL; i = i -> next){
                    if(!strcmp(i -> inodeID, token)){
                        strcpy(curr -> pidNameArguments, i -> pid);
                        strcat(curr -> pidNameArguments, "/");
                        char *name;
                        char *n;
                        char *d = "/";
                        name = strtok( i -> nameArguments, d);
                        while (name != NULL){
                            printf("%s\n",name);
                            n = name;
                            name = strtok (NULL, d);  
                        }
                        strcat(curr -> pidNameArguments, n);
                        break;
                    }
                }
            }
            i++;
            token = strtok (NULL, delim);
        }
    }
}
void currentSocketPorcess(){
    struct dirent *procDirent;
    DIR *procDir;
    procDir = opendir("/proc");
    if(ENOENT == errno){
        fprintf(stderr, "/proc directory doesn't exist\n");
        closedir(procDir);
        exit(1);
    }
    while ((procDirent = readdir(procDir)) != NULL) {
        if(!(strcmp(procDirent->d_name,".") && strcmp(procDirent->d_name,"..")))
            continue;
        struct dirent *pidDirent;
        DIR *pidDir;
        char pidPath[256] = "/proc/";
        strcat(pidPath, procDirent -> d_name);
        strcat(pidPath, "/fd");
        if(!isDirectory(pidPath))
            continue;
        //printf("%s\n",pidPath);char
        pidDir = opendir(pidPath);
        if(ENOENT == errno){
            closedir(pidDir);
            continue;
        }
        while((pidDirent = readdir(pidDir)) != NULL){
            if(!(strcmp(pidDirent->d_name,".") && strcmp(pidDirent->d_name,"..")))
                continue;
            char linkBuf[256];
            char fdPath[256];
            memset(linkBuf, 0, 256);
            memset(fdPath, 0, 256);
            strcpy(fdPath, pidPath);
            strcat(fdPath, "/");
            strcat(fdPath, pidDirent->d_name);
            if(readlink(fdPath, linkBuf, 255) > 0){
                char *token;
                char *delim = ":";
                token = strtok(linkBuf, delim);
                PIDINFO *curr;
                if (!strcmp(token, "socket")){
                    token = strtok(NULL, delim);
                    token[strlen(token)-1] = '\0';
                    token++;
                    curr = malloc(sizeof(PIDINFO));
                    curr -> next = NULL;
                    strcpy(curr -> pid, procDirent -> d_name);
                    strcpy(curr -> inodeID, token);
                    char path[256] = "/proc/";
                    strcat(path ,curr -> pid);
                    strcat(path, "/cmdline");
                    readFile(path, curr -> nameArguments);
                    if(!pHead){
                        pHead = curr;
                        pTail = pHead;
                    }
                    else{
                        pTail -> next = curr;
                        pTail = pTail -> next;
                    }
                }
            }
        }
        closedir(pidDir);
    }
    closedir(procDir);
    PIDINFO *i;
    //for( i = pHead; i != NULL; i = i -> next)
    //    printf("%s\n", i->nameArguments);
}
int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}
void readFile(char *path, char *buffer){
    FILE *fptr = fopen(path, "r");
    fgets(buffer, 256, fptr);
    fclose(fptr);
}
void freeList(){
    LISTELEMENT *i;
    for( i = lHead; i != NULL;){
        LISTELEMENT *t = i;
        free(t);
        i = i -> next;
    }
}
