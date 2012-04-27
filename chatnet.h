 /* Copyright (C) 2012 Chol Nhial <ch01.cout@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <vector>
#include <pthread.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctime>
#include <sys/select.h>
#include <signal.h>


#ifndef CHATNET_H_
#define CHATNET_H_

  void *LaunchMemberFunction(void *obj); 
 void *LaunchMemberFunction2(void *obj); 
struct client
{
std::string name;
int sockfd;
};


// ---------- Chatnet class -------------//
class chatnet
{ 
private:
int sfd, newfd,rv;
struct addrinfo hints, * servinfo, *p;
struct sockaddr_storage client_addr;
socklen_t client_addr_size;
std::vector<client> client_list;
pthread_t client_thread;
pthread_mutex_t mutex;
pthread_mutexattr_t mutex_attr;
struct timeval tv;
fd_set rfds;
public:
 ~chatnet();
void HostServer(const char * PORT);
void ConnectClient(const char * PORT, const char * addr);
void * ServeClient(void * client_sfd);
void Help(unsigned int sock);
void * Pinger(void * sock);
int GetClientName(unsigned int & c, std::string & n,unsigned int sock);
void RemoveClient(int position);
int FindClient(const char * c_name,unsigned int & socket);
int recvtimeout(unsigned int sock, char * buffer,int size,int timeout);
};
//---------------------------------------------------------------------//



//-------------------- Thread launcher classes ------------------/
//-----------------------------------------------------------------//

// This thread launcher class launches ServeClient thread
class ThreadLauncher
{
public:
    ThreadLauncher(chatnet *Obj, void *arg) : Obj(Obj), Arg(arg) {}
    
    void *launch() { return Obj->ServeClient(Arg); }
 
   chatnet *Obj;
    void *Arg;
};


// This thread launcher launches Pinger thread
class ThreadLauncher2
{
public:
    ThreadLauncher2(chatnet *Obj, void *arg) : Obj(Obj), Arg(arg) {}
    
    void *launch() { return Obj->Pinger(Arg); }
 
   chatnet *Obj;
    void *Arg;
};
#endif
