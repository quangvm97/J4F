//--------This is the Peer program

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <fstream>
#include <vector>
#include <cstddef>
#include <sstream>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#define PORT "5000" // the port client will be connecting to

#define MAXDATASIZE 500 // max number of bytes we can get at once 
using namespace std;

    fd_set master;  //----file descripter set as master
    fd_set readfds; //----file descriptor set for reading mode fds
    int fdmax;      //----max file descriptor value
    int listener;   //----listener socket fd
    int newfd;
    struct sockaddr_storage clientsAddr;
    char *peerServPort=NULL;
    socklen_t addrlen;
    struct addrinfo hints,*res,*p;


//-----Function to get the file from the correct peer.
void GetFileFromPeer(char* peerInfo,string fileName, clock_t begin)
{
    
    //-----Splitting the peerInfo into ip address and port no.
    char *a=strtok(peerInfo, " ");
    //-----variable to store the ip.
    char ip[200];
    strcpy(ip,a);
    a=strtok(NULL," ");
    //-----variable to store the port No.
    char *peerPort=a;
    int sockfd,numbytes;
    
    char buff[MAXDATASIZE];
    memset(&buff,0,sizeof(buff));
    struct addrinfo hints,*servinfo,*p;
    int rv;
    char s[100];
    int clientSockfd;
     
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    //------putting the peer ip and port No using getaddrinfo in servinfo node
     if ((rv = getaddrinfo(ip, peerPort, &hints, &servinfo)) != 0)
      {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
      }
      //----looping through the linked list of valid addresses whose first node returned by servinfo
    for (p = servinfo; p!=NULL; p=p->ai_next)
    {   //-----creating a socket using info of peeer server.
        clientSockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(sockfd==-1)
        {
            perror("client:socket");
            continue;
                    
        }
        //-----connecting to the peer server.
        if(connect(clientSockfd,p->ai_addr,p->ai_addrlen)==-1)
        {
            close(clientSockfd);
            perror("client:connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return;
    }
    
    freeaddrinfo(servinfo);
    
    //------sending file name to the server.
    send(clientSockfd,fileName.c_str(),strlen(fileName.c_str()),0);
    //File pointer for the receiving file.
    FILE *receivedFile;
    int remainData;
    int fileSize;
    ssize_t len;
    string fileLocation="./File-share/" + fileName;
    receivedFile= fopen(fileLocation.c_str(), "w");
    //cout<<"fileName is "<<fileName<<endl;
    if (receivedFile == NULL)
        {
                fprintf(stderr, "Failed to open file foo --> %s\n", strerror(errno));

                exit(EXIT_FAILURE);
        }
    //buffer to chunk to receive the file.
    char chunk[10];
    memset(&chunk,'\0',sizeof chunk);
    if(fork()==0)
    {
    //Receiving the file in chunks.

    while ((len = recv(clientSockfd, chunk, sizeof(chunk), 0)) > 0)
        {       
                
                //cout<<chunk<<" "<<len<<endl;
                fwrite(chunk, 1,len, receivedFile);
    
        }
        fclose(receivedFile);
        
        printf("Receiving file %s status %s\n",fileName.c_str(),strerror(errno));
        clock_t end = clock();
        double time_spent = (double)-(end - begin) / CLOCKS_PER_SEC;
        cout<<"\nExcution download time: "<<abs(time_spent)<<"\n";
        
    }

    close(clientSockfd);//----close the connection
        --fdmax;//---decrease the fdmax by 1.




}




//------function to handle a socket fd returned by select function call
void PeerHandler(int i)
{   //-----if the socket fd is listening socket fd
    if(i==listener)
            {   //accept the new client who wants to get file
                addrlen=sizeof(clientsAddr);
                newfd=accept(listener,(struct sockaddr*)&clientsAddr,&addrlen);
                char s[200];
                
                if(newfd==-1)
                    perror("accept");
                else
                {   //--------putting the newly created socket fd in master set.
                    FD_SET(newfd,&master);
                    if(newfd>fdmax)
                        fdmax=newfd;
                }
            
            
                
               
                return;
            }
    //---------else the socket fd is a exsisting connection socket fd.
          else
            {   
                
                
                
                    char str[2000];
                    //------buffer for reading the requested file name.
                    char buff[MAXDATASIZE];
                    memset(&buff,0,sizeof(buff));
                    int nbytes;
                    //-----receive the file name to send
                    if ((nbytes = recv(i, buff, sizeof buff, 0)) <= 0)
                     {
                        
                        if (nbytes == 0) {
                            // connection closed
                            printf("peer: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // close the connection
                        FD_CLR(i, &master);
                     } // remove from master set
                
                else

                {  

                    //cout<<"received file request is: "<<buff<<endl;
                    
                    FILE *fp=NULL;
                    //-----string to create the file path with directory name
                    string fileLocation="./File-share/" + string(buff);
                   
                    //-------create a new file pointer to read the file.
                    fp=fopen(fileLocation.c_str(),"r");
                    if(!fp)
                         fprintf(stderr, "Error fopen ----> %s", strerror(errno));
                    //cout<<"point to file "<<fp<<endl;
                    
                    int sentData=0;
                    //----buffer chunk to create the file in chunk.
                     char chunk[10];
                     memset(&chunk,0,sizeof(chunk));
                     int len;
                     //-------reading the requested file in chunk.
                     while ((len=fread(chunk,1,sizeof chunk, fp)) > 0) 
                     {  //-----sending the chunk.
                        len=send(i,chunk,len,0);
                        
                        sentData+=len;
                        //cout<<"data sent"<<len<<" bytes.still data to send "<<(remainData-=len)<<endl;
                     }
                     fclose(fp);
                     close(i); //----close the connection
                     FD_CLR(i, &master);//------remove the file descriptor.
                    
                                            
 
                }
              }
            
                
}
//------peer-server part
void peerAsServer(const char* servIp)
{
    //-------Initializing master set and readfds set for select function call.
   FD_ZERO(&master);
    FD_ZERO(&readfds);
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    //-----creating a random number between 1025 t0 65535 for peer-server port.
    srand(time(0));
    int tmp=rand();
    
    tmp=(tmp%64510)+1025;
    

    //-----putting tmp number as a string in peerServPort.
    sprintf(peerServPort,"%d",tmp);

    //cout<<"U r in peer server on port "<<peerServPort<<endl;
    
    int rv;
    if (rv = getaddrinfo(servIp, peerServPort, &hints, &res) != 0) 
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p=res;p!=NULL;p=p->ai_next)
    {
        //-------creating socket.
        listener=socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(listener<0)
            continue;
        int yes=1;

        if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    } 
        //-------Binding the socket.
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) 
        {
            close(listener);
            continue;
        }
        break;

    }
    
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
//cout<<"test1\n";
    freeaddrinfo(res);
    //peer-server is listening.Maximum 10 clients are allowed in queue.
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    //------adding listener socket fd in master set for select call.
    FD_SET(listener,&master);
    //------initially fdmax is listener socket fd.
    fdmax=listener;
    for(;;)
    {

        readfds=master;
        //-----calling select function call.
        if(select(fdmax+1,&readfds,NULL,NULL,NULL)==-1)
        {
            perror("select");
            exit(4);
        }
    
    for(int i=0;i<=fdmax;++i)
    {   
        
        
        if(FD_ISSET(i,&readfds))
        {   //calling peerhandler funtion on socketfd i which has been notified by the select function call
            PeerHandler(i);
        }
    }


}

}
//-------function to publish data to server.
int publish(int sockfd)
{   
    DIR *d;
    struct dirent *dir;
    //this string will contain all file names with 1 at start to tell the server it's a publish operation.
    string str="1";
    //------Directory in which my publish files are present
  d = opendir("File-share");
  
  if (d)
  { 
    //-----reading file names from directory
    cout<<"Published Files Are\n";
    cout<<"-----------------------------------\n";
    while ((dir = readdir(d)) != NULL)
    {   
        if(!strcmp(dir->d_name,".") || !strcmp(dir->d_name,".."))
            continue;
        
        printf("%s\n", dir->d_name);
        //------concatenatinating all file names in a string.
        str=str+" "+dir->d_name;
        
      
    }
    //------sending the string of file names to server. 
    send(sockfd,str.c_str(),strlen(str.c_str()),0);
    closedir(d);
  }
  return 1;
}
//-------This function is search operation.This will fetch the peer info from the main server who has the searched file.
void fetchFromServer(int sockfd)
{

    printf("Enter the file name you want to fetch:\n");
    //-----File name to be searched.
    string fetchFile;
    cin>>fetchFile; 

    //----string which will have the searched file name and '2' at first to indicate main server its a fetch operation.
    string fetchStr="2 " + fetchFile;

    cout<<fetchStr.c_str()<<endl;

    //----sending searched file name to server.
    clock_t begin = clock();
    send(sockfd,fetchStr.c_str(),strlen(fetchStr.c_str()),0);

    //----receive list file contain fetch file
    char listPeer[2000];
    memset(&listPeer,'\0',sizeof(listPeer));
    recv(sockfd,listPeer,sizeof(listPeer),0);
    printf("\n----list client contain file: ----");
    printf(listPeer);
    printf("\n");

    send(sockfd,"ok",strlen("ok"),0);

    //-----String to keep the peer adress from whom I will get the file.
    char peerInfo[2000];
    memset(&peerInfo,'\0',sizeof(peerInfo));

    //-----Receiving the peer address.
    recv(sockfd,peerInfo,sizeof(peerInfo),0);

    //------if server sends the peer info.
    if(strcmp(peerInfo,"Requested File is not in P2P system!"))
    {
        cout<<"You will get file "<<fetchFile<<" from peer "<<peerInfo<<endl;
        cout<<"-----------------------------------\n";

        //----Call this function to get the file from peer whose address server has sent.
        GetFileFromPeer(peerInfo,fetchFile, begin);

   }

   //-----else server sends Requested File is not in P2P system!
   else{
        cout<<peerInfo<<endl;
        cout<<"-----------------------------------\n";
   }


}

void *getAddr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int getPort(struct sockaddr *sa)
{
    return ntohs(((struct sockaddr_in*)sa)->sin_port);
}

int main(int argc, char const *argv[])
{ 
    //port which peer as server will use.Using 'mmap' so both child and parent process can modify the variable.
    peerServPort = (char*)mmap(NULL, sizeof *peerServPort, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS, -1, 0);  
    setpgid(0,0);
    //parent process will run the server part.
    if(fork())
    {
        peerAsServer(argv[3]);
    }
    else
    {
	int sockfd,numbytes;
	char buff[MAXDATASIZE];
    //variables for using addrinfo for server informations.
	struct addrinfo hints,*servinfo,*p;
	int rv;
	char s[100];
    //-------------If not correct number of arguments given.Kill all processes and exit.
	 if (argc != 4) 
	 {
        fprintf(stderr,"Give correct Number of arguments\n");
        kill(0, SIGINT);  
        kill(0, SIGKILL);
        exit(0);
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
     if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
      {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
      }
    for (p = servinfo; p!=NULL; p=p->ai_next)
    {
    	sockfd=socket(p->ai_family,p->ai_socktype,p->ai_protocol);
    	if(sockfd==-1)
    	{
    		perror("client:socket");
    		continue;
    		    	
    	}
    	if(connect(sockfd,p->ai_addr,p->ai_addrlen)==-1)
    	{
    		close(sockfd);
    		perror("client:connect");
    		continue;
    	}
    	break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, getAddr((struct sockaddr *)p->ai_addr),s, sizeof s);
    
    freeaddrinfo(servinfo);
    //creating string for peer-server address.
    string peerServAddr="3 "+string(argv[3])+" "+string(peerServPort);
    //sending the peer-server address to main server.
    send(sockfd,peerServAddr.c_str(),strlen(peerServAddr.c_str()),0);
   
    if ((numbytes = recv(sockfd, buff, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    else
    {
        printf("You got connected with Main server %s:%d\n\n",s,getPort((struct sockaddr *)p->ai_addr));
        printf("-----------------------------------\n");
        printf("%s\n",buff);
        LoopBack:  
        printf("-----------------------------------\n");      
        printf("Press 1 to Publish list file name form File-share folder to server\n");
        printf("Press 2 to Download file form other client\n");
        printf("Press 3 to Exit\n");
        int k;
        
        scanf("%d",&k);
        
        switch(k)
        {
        case 1:
        {

        //function call to publish data
        publish(sockfd);
        goto LoopBack;
        
        }
        case 2:
        {
        //function call to search
        fetchFromServer(sockfd);
        goto LoopBack;
        
        }
        case 3:
        {

        kill(0, SIGINT);  
        kill(0, SIGKILL);
        exit(0);
        
        }


        }
    


    }
    buff[numbytes] = '\0';

    
    getchar();

    }

    //close(sockfd);
	return 0;
}
