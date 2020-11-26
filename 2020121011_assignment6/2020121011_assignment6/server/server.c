
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
//following are for listing files
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <grp.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#define PORT 8000
char list_buffer[1024]={0};//this buffer contains list of all files
void call_ls(char input[]);
int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;  
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "connection establised with server";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  // creates socket, SOCK_STREAM is for TCP. SOCK_DGRAM for UDP
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // This is to lose the pesky "Address already in use" error message
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt))) // SOL_SOCKET is the socket layer itself
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;  // Address family. For IPv6, it's AF_INET6. 29 others exist like AF_UNIX etc. 
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any IP address - listens from all interfaces.
    address.sin_port = htons( PORT );    // Server port to open. Htons converts to Big Endian - Left to Right. RTL is Little Endian

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Port bind is done. You want to wait for incoming connections and handle them in some way.
    // The process is two step: first you listen(), then you accept()
    if (listen(server_fd, 3) < 0) // 3 is the maximum size of queue - connections you haven't accepted
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // returns a brand new socket file descriptor to use for this single accepted connection. Once done, use send and recv
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                       (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    valread = read(new_socket , buffer, 1024);  // read infromation received into the buffer
    printf("%s\n",buffer);
    send(new_socket , hello , strlen(hello) , 0 );  // use sendto() and recvfrom() for DGRAM
    printf("ready\n");
    while(1)
    {
        //printf("hai\n");
        char buffer2[1024]={0};//input from client
        memset(buffer2,0,1024);
        valread = read(new_socket , buffer2, 1024);
        char *token;
        char buffer3[10][1024]={0};
        int filecount=0;//no.of files to be sent
        token=strtok(buffer2," ");
        while(token!=NULL)
        {
            strcpy(buffer3[filecount],token);
            filecount++;
            token=strtok(NULL," ");
        }
        if(strcmp(buffer2,"list")==0)
        {
            call_ls("ls");
            //printf("%s\n",list_buffer);
            send(new_socket ,list_buffer , strlen(list_buffer) , 0 );
        }
        if(strcmp(buffer2,"exit")==0)
        {
            break;
        }
        if(strcmp(buffer3[0],"get")==0)
        {
            //file transfer main program
            for(int i=1;i<filecount;i++)
            {
                //check if its valid filename
                sleep(1);
                write(1,buffer3[i],strlen(buffer3[i]));
                int ret=0;
                char filesize[1024]={}; 
                //open the file requested
                int fd = open(buffer3[i], O_RDONLY);
                if (fd == -1)
                {
                   
                    perror(" open");
                    strcpy(filesize,"not valid");
                    send(new_socket,filesize,strlen(filesize),0);
                    continue;
                }
                //to get file size
                struct stat sb; 
                stat(buffer3[i],&sb);
                
                sprintf(filesize, "%ld", sb.st_size);
                //printf("%s %ld\n",filesize,sb.st_size);
                send(new_socket,filesize,strlen(filesize),0);
                sleep(2);
                long int m =65500;
                char *bufferarray = (char *)malloc(sizeof(char) * m);
                int flag=0;
                char interme[260];
                long int n;
                long int ab=sb.st_size-m;
                int tot_bits=0;
                while(ab>0)
                {
                   // printf("hai\n");
                    flag=1;
                    int bits=0;
                    n=read(fd,bufferarray,m);
                    tot_bits+=n;
                    if(n<0)
                    {
                        perror("program");
                        flag=2;
                        break;
                    }
                    //send
                   //printf("no of bytes read from file -%d\n",n);
                    send(new_socket ,bufferarray,n,0);
                    sprintf(interme,"\r%.2f",(tot_bits*100.0)/sb.st_size);
                    write(1,interme,7);  
                    ab=ab-n;
                }
                //printf("%d -totalbits\n",tot_bits);
                //for remaining bytes of larger file or,for small files whose is less than m bytes 
            char *bufferarray1=(char *)malloc(sizeof(char) * m);    
            if(ab!=0)
            {
                long int x=ab+m;
                memset(bufferarray1,0,x);
                n=read(fd,bufferarray1,x);
                
                if(n<0)
                {
                    perror("read"); 
                    break;  
                }
                //send
                send(new_socket ,bufferarray1,n, 0);
                //printf("read-%d,expected read-%d\n",n,x);
                sprintf(interme,"\r%.2f",(lseek(fd,0,SEEK_END)*100.0)/sb.st_size);
                write(1,interme,7);  
            }
            write(1,"\n",2);
            int flag1;
            flag1=close(fd);
            if(flag1<0)
            write(1,"unable to close file\n",50);
            }
        }
           // sleep(20);
    }   
    close(new_socket);
    return 0; 
}
//function for listing all files
void call_ls(char input[])
{
    memset(list_buffer,0,1024);
    struct passwd *pw; 
    struct group   *grp;
    struct tm      *tm;
    char  outstr[256];   
    int dircount=0;
    char alldir[10][1024]={{}};
    char *token=NULL;
    int count=0;
    int onlydir=0;
    char *argv[1024]={};
    token=strtok(input," ");
    while(token!=NULL)
    {        
       argv[count]=token;
       token=strtok(NULL," ");  
       count++;   
    }                    
    int code=0;   
    DIR *dir ;
    struct dirent *d;
    char this_dir[1024]={};	 
    if(count==1)
	{	
        getcwd(alldir[0],999);
        code =1;
    } 
    if(onlydir==0)
        dircount=1;
    for(int i=0;i<dircount;i++)
    {   
        if(dircount>1)
        {
            char interme[20]={};
            sprintf(interme,"%s : \n",alldir[i]);
            write(1,interme,strlen(interme));  
        }
        strcpy(this_dir,alldir[i]);	
        dir = opendir(this_dir);
        if(dir==NULL)
        {
            char temp[1024]="/";
            getcwd(alldir[i],999); 
            strcat(temp,argv[i]);            //when dir name is mentioned, store names of dir to be executed
            strcat(alldir[i],temp);
            strcpy(this_dir,alldir[i]);
            dir = opendir(this_dir);
            if(dir==NULL)
            {
                perror("dir");
                return ;
            }
                            
        }
        if(code==1)
        {
            while((d=readdir(dir)) != NULL)
            {		       
                if( !(strcmp(d->d_name,".") == 0 || strcmp(d->d_name,"..") == 0) ) 
                {
                    char interme[260]={};
                    sprintf(interme,"%s\t",d->d_name);
                    //write(1,interme,strlen(interme));
                    strcat(list_buffer,interme);  
                }  			            
            }
		    closedir(dir);
		    write(1,"\n",1);
        }
    }
}
