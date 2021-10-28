
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <grp.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>

#define ANSI_CYAN "\033[1;36m"
#define ANSI_RED "\033[1;31m"
#define ANSI_GREEN "\033[1;32m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_MAGENTA "\033[1;35m"
#define ANSI_DEFAULT "\033[0m"

#define PORT 8000



int main(int argc, char const *argv[])
{
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Connection established with client";
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr)); // to make sure the struct is empty. Essentially sets sin_zero as 0
                                                // which is meant to be, and rest is defined below

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converts an IP address in numbers-and-dots notation into either a 
    // struct in_addr or a struct in6_addr depending on whether you specify AF_INET or AF_INET6.
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  // connect to the server address
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    send(sock , hello , strlen(hello) , 0 );  // send the message.
    printf("connection request sent\n");
    valread = read( sock , buffer, 1024);  // receive message back from server, into the buffer
    printf("%s\n",buffer);
    
    while(1)
    {
        printf(ANSI_CYAN"Client >");
        printf(ANSI_DEFAULT" ");
        char input1[1024],*token,input2[1024]={0}; 
        int k=0;int size=1024;
        char commands[5][1024]={};    
        scanf(" %[^\n]s",input1);
        int buffer1[1024]={0};
        strcpy(input2,input1);
        token=strtok(input1," ");
        while(token!=NULL)       //separate through ; and store 
        {
            strcpy(commands[k],token);
            k++;
            token=strtok(NULL," ");
        }
        if(strcmp(input2,"list")==0)
        {
            char *list="list";
            char buffer1[1024]={0};
            memset(buffer1,0,1024);
            send(sock , list , strlen(list) , 0 );  // send the message.
            valread = read( sock , buffer1, 1024);  // receive message back from server, into the buffer
            printf("%s\n",buffer1);
        } 
       
        else if(strcmp(commands[0],"get")==0)
        {
            send(sock, input2, strlen(input2), 0); //send whole command ,break it at server side and use it
            //create a filewith samename and size and write into it
            //just loop here
            for(int i=1;i<k;i++)
            {
                
                char filesize[20]={}; //passfilesize and write client side file receive
                valread = read( sock ,filesize, 20);  // receive message back from server, into the buffer
                if(strcmp(filesize,"not valid")==0)
                {
                    printf(ANSI_RED"file %s doesnt exist in server",commands[i]);
                    printf(ANSI_DEFAULT"\n");
                    continue;
                }
                long int m=65500;//chunk size
                int size1=atoi(filesize);
                char arr[1000]={};
                
                sprintf(arr,"Downloading  %s whose size is %d bytes..........\n",commands[i],size1);  
                write(1,arr,strlen(arr));          
                char *bufferarray = (char *)malloc(sizeof(char) * m);
                memset(bufferarray,0,m);
                char *str;
                int size=size1;
                str=(char*)malloc(size);
                strcpy(str,commands[i]);
                //to check whether its valid file
                char pathFile[260];
                char interme[260];
                sprintf(pathFile, "%s", str);
                int total_bits=0;
                int fdrev=open(pathFile, O_RDWR | O_CREAT |O_TRUNC,0777);
                //error handling 
                if(fdrev==-1)
                {
                    perror("program");
                }
                long int ab=size1-m;
                while(ab>0)
                {
                    //printf("loop1\n");
                    memset(bufferarray,0,m);
                    valread = recv(sock ,bufferarray,m,0);
                    //printf("no of bits read-%d , m-%d\n",valread,m);
                    write(fdrev,bufferarray,m);
                    total_bits+=valread;
                    //printf("%d is bits read till now\n", total_bits);
                    sprintf(interme,"\r%.2f",(total_bits*100.0)/size1);
                    write(1,interme,7);
                    ab=ab-valread;
                }
                if(ab!=0)
                {
                    long int x=ab+m;
                    memset(bufferarray,0,m);
                    valread = read( sock ,bufferarray,m);  
                    write(fdrev,bufferarray,valread);
                    total_bits+=valread;
                    //printf("bits-%d ,expected write- %d\n",valread,x);
                    sprintf(interme,"\r%.2f",(total_bits*100.0)/size1);
                    write(1,interme,7);
                }
                ftruncate(fdrev,size1); //to avoid eof problem
                close(fdrev);
                printf("\n");
            }
            
        }
        else if(strcmp(input2,"exit")==0)
        {
            //send to server we need to close
            char *list="exit";
            send(sock , list , strlen(list) , 0);
            return 0;
        }
        else{
            printf(ANSI_RED"invalid input\n");
        }
    }
    return 0;
}