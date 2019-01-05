//------This is the main server program.

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <map>
#include <utility>
#include <fstream>
#include <vector>
#include <cstddef>
#include <algorithm>
#include <sstream>
#define PORT "5432"
using namespace std;
    fd_set master;
    fd_set readfds;
    int fdmax;
    int listener;
    int newfd;
    struct sockaddr_storage clientsAddr;
    
    socklen_t addrlen;
    char option[5];
    char FileName[100];
    int nbytes;
    map<string,vector<int> > DB;//-------C++ Map.which will store file Name as key.and as value list of socket fd's of connected peers who have that file
    map<int,pair<string,int> > peerInfo;//--------stores socket fd of the peer as key.and it's peer-server ip address and port num as value.
    struct addrinfo hints,*res,*p;
    
    map<string,int> PublishFile;
    
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
//------childHandler function takes a socket file descriptor and works according to the type of the file descriptor.
void ChildHandler(int i)
{   //------If the socket fd is listner socket fd
    if(i==listener)
            {   
                addrlen=sizeof(clientsAddr);
                //------accepting the new conection
                newfd=accept(listener,(struct sockaddr*)&clientsAddr,&addrlen);
                char s[200];

                inet_ntop(clientsAddr.ss_family,getAddr((struct sockaddr *)&clientsAddr),s, sizeof s);
                
                
                printf("server: got connection from %s : %d\n", s,getPort((struct sockaddr *)&clientsAddr));
                if(newfd==-1)
                    perror("accept");
                else
                {   //------Adding new socket fd to master list
                    FD_SET(newfd,&master);
                    if(newfd>fdmax)
                        fdmax=newfd;//------making fdmax as newfd.
                }
            
            
                
                if(send(newfd,"Hi!You have joined the J4F Network!",strlen("Hi!You have joined the J4F Network!"),0)==-1)
                {
                perror("send");
                
                }
                //clientState[i]="NONE";
               
                return;
            }

            //--------else the socket fd is a already exsisting socket fd

          else
            {   
                char s[200];
                inet_ntop(clientsAddr.ss_family,getAddr((struct sockaddr *)&clientsAddr),s, sizeof s);
                //-------getting the port no of the peer connected on that socket
                int portNo=getPort((struct sockaddr *)&clientsAddr);
                
                //cout<<"test2\n";
                    char str[4000];
                    char buff[4000];
                    memset(&buff,'\0',sizeof(buff));
                    memset(&str,'\0',sizeof(str));
                    //----Get the message from peer which will indicate a search,publish or giving own peer-server adress
                    if ((nbytes = recv(i, buff, sizeof buff, 0)) <= 0)
                     {
                        
                        if (nbytes == 0) {
                            // connection closed
                            printf("peer:socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // ----closing the socket
                        FD_CLR(i, &master);

                        map<string,vector<int> >::iterator it;

                        for (it = DB.begin(); it!=DB.end(); ++it)
                        {
                            vector<int>::iterator delit=find((it->second).begin(),(it->second).end(),i);
                            if(delit!=(it->second).end())
                                (it->second).erase(delit);

                        }
                        peerInfo.erase(i);


                     } // remove from master set
                
                else

                {   
                    strcpy(str,buff);
                    memset(buff,0,strlen(buff));
                    //cout<<"received message is: "<<str<<endl;
                    std::vector<string> FileNames;
                    char *p=strtok(str, " ");
                    //----parsing the message sent by peer by spaces.And putting it in a vector FileNames.
                    while(p)
                    {
                        string tmp=p;
                        FileNames.push_back(tmp);
                        p=strtok(NULL, " ");
                    }

                    memset(str,0,strlen(str));
                    /*for(vector<string>::iterator it=FileNames.begin();it!=FileNames.end();++it)
                        cout<<*it<<" ";
                    cout<<endl;*/
                    
                    //-----if the first part of message is '3' the peer is publishing its ip and port num to the server.
                    if(*(FileNames.begin())=="3")
                    {

                        string ipstr=FileNames.at(1);//---ip of the peer
                        string portNo=FileNames.at(2);//---Port No of the peer


                        //cout<<"port: "<<portNo<<endl;

                        pair <string,int> cliIpPort (ipstr,atoi(portNo.c_str()));
                        peerInfo.insert(pair<int,pair<string,int> >(i,cliIpPort));//---adding the peer adress in the map peerInfo where the key is the socket id.
                        return;
                        

                    }

                    //-----If the message starts with '1' its a publish operation.
                    else if(*(FileNames.begin())=="1")
                    {   

                        
                                    
                        for(vector<string>::iterator it=FileNames.begin()+1;it!=FileNames.end();++it)
                        {
                            DB[*it].push_back(i);//---adding the filename as key and the socket id as value in the map DB.
                            
                        }

                        //-------List file shared in server
                        printf("\n----List file client shared with server: ----\n");
                        for(map<string,vector<int> >::iterator it=DB.begin(); it!=DB.end(); ++it)
                        {
                            if(std::find((it->second).begin(), (it->second).end(), i) != (it->second).end()) {
                                cout << it->first<<"\n";
                            }

                        }
                    }
                    //-------If the message starts with '2' its a Fetch operation.
                    else if(*(FileNames.begin())=="2")
                    {
                        //-----The file Name to Fetch
                        string fileToFetch = FileNames.at(1);
                        
                        map<string,vector<int> >::iterator it=DB.find(fileToFetch);
                        //------If P2P system has the file
                        if(it != DB.end())
                        {   

                            //-----List server contain file name
                            string clientList;
                            std::stringstream ss;
                            vector<int> vecClient = it->second;
                            cout<<"\n----Has "<<vecClient.size()<<" client contain file: "<<fileToFetch<<"-----";
                            for(int j = 0; j < vecClient.size(); j++)
                            {

                                map<int,pair<string,int> >::iterator client = peerInfo.find(vecClient.at(j));

                                clientList = clientList + "\n"+std::to_string((j+1))+") ip: "+(client->second).first+" port: "+std::to_string((client->second).second);
                                fflush(stdout);
                            }
                            cout<<clientList;
                            fflush(stdout);
                            char clients[200];
                            strcpy(clients, clientList.c_str());
                            //-----send list peer has file to peer-----
                            send(i, clients, strlen(clients), 0);

                            //----test---
                            char buffer[4000];
                            memset(&buffer,'\0',sizeof(buffer));
                            recv(i, buffer, sizeof buffer, 0);

                            //-----Num of peers who has that file
                            int NumOfPeers=DB[fileToFetch].size();
                           
                            //-----Server is randomly choosing a peer who has that file.
                            int selectedPeer=rand()%NumOfPeers;
                            //Getting the socket fd of the connected peer
                            int socketFd= DB[fileToFetch].at(selectedPeer);
                            //Getting the address of the peer associated with the socket
                            map<int,pair<string,int> >::iterator it2=peerInfo.find(socketFd);

                            char portNum[2000];
                           
                            snprintf(portNum,sizeof(portNum),"%d",it2->second.second);
                            string address=it2->second.first+" "+portNum;
                            //------sending the port num and ip to the peer from which the peer will get the file
                            send(i,address.c_str(),strlen(address.c_str()),0);


                        }
                        //----else the no peer has the requested file.
                        else
                        {
                            send(i,"Requested File is not in P2P system!",strlen("Requested File is not in P2P system!"),0);
                        }
                        




                    }
                        
                        
                    //----If the sent query sent by the peer not in correct format send an error message.   
                    
                    else
                    {
                        send(i,"Send query in correcr format",strlen("Send query in correcr format"),0);
                        




                    }

                }
              }
            }
                







int main(int argc, char const *argv[])
{   

    FD_ZERO(&master);//-------Initializing fd set master
    FD_ZERO(&readfds);//------Initializing fd set for read mode file descriptors

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    
    int rv;
    if (rv = getaddrinfo(argv[1], argv[2], &hints, &res) != 0)
    //------calling getaddrinfo with arguments server ip address and port no.
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p=res;p!=NULL;p=p->ai_next)//------Getting the valid ip address 
    {
        listener=socket(p->ai_family,p->ai_socktype,p->ai_protocol);//----creating the socket.
        if(listener<0)
            continue;
        int yes=1;

        if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    } 

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) //-----Binding
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
cout<<"-----------------Welcome to P2P system------------------\n";
    freeaddrinfo(res);
    //-----Listening on newly created socketfd listener
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    //-------Adding the listener socket fd to master set
    FD_SET(listener,&master);
    //-------Making fdmax as listener
    fdmax=listener;
    //continuously server will call select and will check on fds.
    for(;;)
    {
        //----- readfds set as master set
        readfds=master;
        //calling system call readfds set
        if(select(fdmax+1,&readfds,NULL,NULL,NULL)==-1)
        {
            perror("select");
            exit(4);
        }
    
    for(int i=0;i<=fdmax;++i)
    {   
        
        
        if(FD_ISSET(i,&readfds))//-----if socket fd i is set notified by select
        {
            ChildHandler(i);
        }
    }



    
}
return 0;
}