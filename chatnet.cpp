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


 //--------- Chatnet class methods --------------------//
//-----------------------------------------------------//

#include "chatnet.h"
#include <pthread.h>
using std::cout;
using std::endl; 

// --- protocols used to communicate with clients-----//
const char * protocol[] = 
{
 "LONGNAME",  
 "SHORTNAME", 
 "ILLEGALNAME",
 "NAMEINUSE",
 "NAMEACCEPTED",
 "NOSUCHCOMMAND",
 "MSGNOTSENT"
};
//--------------------------------------------------//

//----This function starts the server and listens for clients----//
void chatnet::HostServer(const char *PORT)
{

// Initialize data used by threads
pthread_mutexattr_init(&mutex_attr);
pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_ERRORCHECK_NP);
  pthread_mutex_init(&mutex, &mutex_attr);
  pthread_mutexattr_destroy(&mutex_attr);
cout << "[+] Chatnet Host started..." << endl;

int yes = 1; // used with setsockopt


memset(&hints,0,sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;

//--------------Do DNS lookup -------------------------//
if((rv = getaddrinfo(NULL,PORT,&hints,&servinfo)) == -1)
{
cout << "[X] Error: " << endl;
cout << gai_strerror(rv) << endl;
exit(EXIT_FAILURE);
}

//------------- loop through the linklist and find a correct socket-----//
for(p = servinfo; p != NULL; p = p->ai_next)
{
sfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
if(sfd == -1)
{
    cout << "[X] Error: creating socket...trying again" << endl;
    continue;
}

//---------------- The address might be used again ---------------//
if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
{
cout << "[X] Error: setsockopt" << endl;
exit(EXIT_FAILURE);
}

//---------------Try Bind to the socket --------------------------//
if(bind(sfd,p->ai_addr,p->ai_addrlen) == -1)
{
    cout << "[X] Error: bind" << endl;
    continue;
}
//------------ We are binded to the socket and everything went well break the loop---------------//
break;
}

//-----Things might have not went well -----//
if(p == NULL)
{
    cout << "Unable to host" << endl;
    exit(EXIT_FAILURE);
}

//---------- We are done with the linklist  -------------//
freeaddrinfo(servinfo);

// ------ Listen on the port specified -----//
if(listen(sfd,10) == -1)
{
    cout << "[X] Error: listen" << endl;
    exit(EXIT_FAILURE);
}

// ---------------- Start accepting connections ------//
while(true)
{
    client_addr_size = sizeof(client_addr);
    //------- Accept a new client -------//
    newfd = accept(sfd,(sockaddr *) &client_addr,&client_addr_size);
  
    //---------- Error occured while accepting ---------//
    if(newfd == -1)
    {
        perror("accept");
        continue;
    }
    
  //------- This thread is started to serve a client.--------//
  //------- The second argurment to ThreadLauncher is the clients socket ---//
  //-------The client socket passed is used by the function that serves the client ---//
  ThreadLauncher launcher(this, &newfd);
 pthread_create(&client_thread,NULL, LaunchMemberFunction, &launcher);
 
}

}


//---- This is the function that serves the clients needs. ----// 
//---- It is created as a thread for every client that connects. ---//
//----- It talks over a udp socket with another thread that managers clients ----// 
 void * chatnet::ServeClient(void * client_sfd)
{

 // ---- get the socket for this client -----------//
 unsigned  client_sock = *(unsigned int *)client_sfd; 
 
 // ------ set up a local char buffer ------------//
 char buffer[1024];

 //---- Some local variables -----//
 unsigned int code; 
 int counter = 0;
 int arg_counter = 0;
 int number_of_argurments = 0; 
 int dont_complain = 0;
 pthread_t pinger_thread;
 int location;
 
 //---- Holds the name of the client -----//
 std::string name; 



 //---- Read the users name and check it against errors. --------//
 //------ if error handle it probably. ---------------------------//
 do
 {
 // ---- Read a name ---//
 rv = GetClientName(code,name,client_sock);

if(rv == -1)
{
 //---- A fatal error was detected  ---//
 close(client_sock);
 return NULL;
}

//-- ERROR: name might be illegal ---//
if(rv != 0)
{
strcpy(buffer,protocol[code]);
strcat(buffer,": ");
if(code == 0)
strcat(buffer,"Your nickname can only be 16 characters long.\n");
if(code == 1)
strcat(buffer,"Your nickname has to be atleast 3 characters long.\n");
if(code == 2)
strcat(buffer,"Your nickname should not contain space characters.\n");
if(code == 3)
  strcat(buffer,"The nickname your are trying to use is already in use.\n");
//------ Send error to client ------------------------------------------//
if(send(client_sock,buffer,strlen(buffer),0) == -1)
{
//---- Fatal error ---//
  close(client_sock);
 return NULL;
}
}

//---- Get the name until there's no error -------//
}while(rv != 0);

//---- Client name has been accepted -----//
//---- Let the client now that the name has been accepted ---//
strcpy(buffer,protocol[4]);
if(send(client_sock,buffer,strlen(buffer),0) == -1)
{ 
  close(client_sock);
  return NULL;
}

//--- Holds the clients commands,--//
std::string command; 

//--- These two hold command argurments --//
char  arg1[16], arg2[512]; 

// ---set up client information structure ---// 
client temp; 
temp.name = name;
temp.sockfd = client_sock;
//--- add new client to the client list -----//
client_list.push_back(temp); 

int recv_bytes;

 //----------- Start Pinger thread --------------------//
   ThreadLauncher2 launcher(this, &client_sock);
 pthread_create(&pinger_thread,NULL, LaunchMemberFunction2, &launcher);
 //-----------------------------------------------------------------//
 



//--------- start accepting commands, while the command is not /exit ---------//
do
{ 
  // Receive the command ----//
  rv = recvtimeout(client_sock,buffer,sizeof(buffer) - 2, 30);
 
  //------------ Check for erros from recvtimeout--------//
  //------------- If a error ocurred the do-while loop is broken and ServeClient cleans up and returns------//
  if(rv == -1)
  break;buffer[recv_bytes -1] = 0;
  if(rv == 1)
    break;
  if(rv == 2)
  {
  strcpy(buffer,"Connection timeout.\n");
  if(send(client_sock,buffer,strlen(buffer),0))
    break;
  break;
  }
  
  //--------------- Everything went well with recvtimeout() ----------------------//
  //---- terminate the receive string ----//
  buffer[recv_bytes -1] = 0; 
  //----- get the recevied ready for comparison ----//
  command = buffer;
  
  // --- /list command sends a list of connected clients names ---//
  if(command == "/list")
 {  
   //----- Loop through the client list ------------//
  for(int i = 0;i < client_list.size(); i++)
  { 
    //----- Ge the client info structure at the location (i) --------//
    temp = client_list.at(i);
  
    //---- send names of connected clients to client ----//
   strcpy(buffer,temp.name.c_str());
   strcat(buffer,"\n");
   if(send(client_sock,buffer,strlen(buffer),0) == -1)
     break; // Fatal error 
  
  // ----Check if we have reached the end -----//   
  if(i == client_list.size() -1)
   {
    strcpy(buffer,"--END OF LIST--\n");
    if(send(client_sock,buffer,strlen(buffer),0) == -1)
    break; // error ocurred
   }
  
  }
}
//------------------------------------------------------------//

//----------------- /msg command sends a msg to a client specified ------------------------//
//---------------- The client must be connected ------------------------------------------//
//NOTE: /msg takes exactly two argurments ----------------------------------------------//
else if(!strncmp(command.c_str(), "/msg",3) && ( command[4] == '\0' || command[4] == ' ') )
{
  
  number_of_argurments = 0; 
 //----- use to tell other control statements not send complains to the client --//
  dont_complain = 0; 
  
 //---Holds number of characters in a argurments ---/
  arg_counter = 0;

 //--- Check if there's a first argurment ---//
   counter = 5;
 if(command[counter]) 
   number_of_argurments++;
 
 //--------------------------------- Get the first argurment ------------------------------//
 while((command[counter] != '\0') && (command[counter] != ' ') && arg_counter < sizeof(arg1)) 
  arg1[arg_counter++] = command[counter++];
 arg1[arg_counter] = 0; // terminate string
 arg2[0] = 0;
 
//------- Check if the client as execeeded the expected size of argurment ----------------//
if(arg_counter == sizeof(arg1))
{
  strcpy(buffer,"First argurment to /msg can only be 16 characters long\n");
  if(send(client_sock,buffer,strlen(buffer),0) == -1)
   dont_complain = 1; // okay display this warning only
    continue; // got to the beginning of the loop
}
 
//--------------------------------- Get the second argurment ------------------------------//
 if(command[counter] != 0) // is there more characters?
{ counter++; // skip space character
  arg_counter = 0;
  while((command[counter] != '\0') && arg_counter < sizeof(arg2)) 
  arg2[arg_counter++] = command[counter++];
 arg2[arg_counter] = 0; // terminate string
 number_of_argurments++; 
}

//------------------ check if the client as execeeded the expected size of argurment --------------//
if(arg_counter == sizeof(arg2))
{
  strcpy(buffer,"Message is too long for 512 bytes\n");
  if(send(client_sock,buffer,strlen(buffer),0) == -1)
  dont_complain = 1; // display this warning only
  continue; // got to the beginning of the loop
}

// ------- Check if there's still more characters to be procced -----//
if(command[counter] != 0) // is there still more?
number_of_argurments++;

// -------- Check if too few argurments are specified --------------//
// --------- Let the client know ----------------------------------//
 if(number_of_argurments < 2 && dont_complain == 0)
 {
  strcpy(buffer,"Remember: /msg takes 2 argurment\n");
  strcat(buffer,"/msg <name> :<msg>\n");
  if(send(client_sock,buffer,strlen(buffer),0) == -1)
  dont_complain = 1; 
    continue; // got to the top of the loop
}


//--------- Check if the start of message is there( ':' indicates the start of the msg)-----------//
if(arg2[0] != ':' && dont_complain == 0)
{
  strcpy(buffer,"Oops did you forget ':'\n");
  strcat(buffer,"/msg <name> :<msg>\n");
  if(send(client_sock,buffer,strlen(buffer),0) == -1)
   continue; // got to the beginning of the loop
}


int isfound;

// ----------------- No problems. Message the client --------------------//
if(number_of_argurments == 2 && arg2[0] == ':')
{
  //------- Search for the name in the client list --------------------//
 isfound = FindClient(arg1,client_sock);
 //---------Check if the client has not been found Client not found in the list ---//
  if(isfound == -1)
    continue;
 
 //------ If the client name exist try and send  a message -----------------------//
  if(isfound >= 0)
  {temp = client_list.at(isfound);
   strcpy(buffer,"/msg ");
   strcat(buffer,name.c_str());
   strcat(buffer," ");
   strcat(buffer,arg2);
   //------- Send message if not sending to self --------------------------------//
 if(name != temp.name)  
  if(send(temp.sockfd,buffer,strlen(buffer),0) == -1)
  { 
    strcpy(buffer,protocol[6]);
   if(send(client_sock,buffer,strlen(buffer),0) == -1)
   continue;
  }    
}

  //-------------- Client sending a message to self,that's odd ------------------//
  if(name == arg1)
 {
  strcpy(buffer,"It's not very wise to send a message to your self\n");
   if(send(client_sock,buffer,strlen(buffer),0) == -1)
     continue; // an  error occured, goto the top of the loop
 } 
    
}


}
//----------------------------------------------------------------------------------------//

//----------- /help command ----//
else if(command == "/help")
  Help(client_sock);
//-----------------------------//

// --------/exit exits  ---------//  
else if(command == "/exit")
{
  // say bye to user as he/she exits
 strcpy(buffer,"Bye.\n");
 if(send(client_sock,buffer,strlen(buffer),0) == -1)
   break;
}
//----------------------------------------------//

// ----------- PING command from client -------------------//
//------------ send the ping to clientmanager ping server ---//
else if(command == "PING")
{
 
 cout << "PING RECEIVED!\n";
}
//-------------------------------------------------------------------------------------//

//---------------------- /chat tells another client to chat with this client ----------//
//----------------------NOTE: /chat takes 2 argurments --------------------------------//
else if(!strncmp(command.c_str(), "/chat",4) && ( command[5] == '\0' || command[5] == ' ') )
{
 number_of_argurments = 0; 
 dont_complain = 0; // use to tell other control statements to not send msg to client
 arg_counter = 0;
  counter = 6;
  
 //---------- Check if there's a first argurment---------------------------------------//
 if(command[counter] != 0) 
   number_of_argurments++;
 
  //------------------------------- Get first agurment ------------------------------------//
 while((command[counter] != '\0') && (command[counter] != ' ') && arg_counter < sizeof(arg1)) 
  arg1[arg_counter++] = command[counter++];
 arg1[arg_counter] = 0; // terminate string
 
 
//---------------------- Check if the client as execeeded the expected size of argurment ----//
if(arg_counter == sizeof(arg1))
{
  strcpy(buffer,"First argurment to /chat can only be 16 characters long\n");
  if(send(client_sock,buffer,strlen(buffer),0) == -1)
   dont_complain = 1; // okay display this warning only
    continue; // got to the beginning of the loop
}

//----- Check if there's more characters to proccess ------------//
if(command[counter] != 0) // is there still more?
number_of_argurments++;

//--------------- report if no argurments were given or too many argurments ---------------------//
 if(number_of_argurments > 1 && dont_complain == 0)
 {
  strcpy(buffer,"Remember: /chat takes 1 argurment\n");
  strcat(buffer,"/chat <name>\n");
  if(send(client_sock,buffer,strlen(buffer),0) == -1)
  dont_complain = 1; 
    continue; 
}

int isfound;

//----- check if the user the client wants to chat to is online ----------------------------------//
isfound = FindClient(arg1,client_sock);
if(isfound == -1)
  continue;

if(isfound >= 0)
{
   temp = client_list.at(isfound);
 sprintf(buffer,"%s %s","xchat",name.c_str());
 if(send(temp.sockfd,buffer,strlen(buffer),0) == -1)
 { 
    strcpy(buffer,temp.name.c_str());
   strcat(buffer," Has gone offline.\n");
if(send(client_sock,buffer,strlen(buffer),0) == -1)
   continue;
 }
}

}

else
{
 strcpy(buffer,protocol[5]);
 strcat(buffer,": ");
 strcat(buffer,"'");
 strcat(buffer,command.c_str());
 strcat(buffer,"'");
 strcat(buffer," Is not a valid command. Enter /help for help.\n");
 if(send(client_sock,buffer,strlen(buffer),0) == -1)
   break;
}

sleep(1);
}while(command != "/exit");

// client has  exited 

//------- Remove user from the list ------------//
 location = FindClient(name.c_str(),client_sock);
      if(location >= 0)
	RemoveClient(location);

cout << "Client Unexpectively exited\n";
close(client_sock);
return NULL;
}
  
  // This function is called by ServeClient to get the name of the user a check against common errors
   int chatnet::GetClientName(unsigned int & c,std::string & n,unsigned int sock)
   {  char buffer[512];
      int recv_bytes;
    
     rv = recvtimeout(sock,buffer,sizeof(buffer) -2,30);
    if(rv == -1)
      return -1;
    if(rv == 1)
  return -1;
  if(rv == 2)
  {
    strcpy(buffer,"Connection timeout.\n");
  send(sock,buffer,strlen(buffer),0);
  return -1;
    
  }
   
    //check if the name is shorter than 3 characters
    if(strlen(buffer) < 3)
     {
       c = 1;
       return 1;
    }
    
    // check if the name is longer than 16 characters
    if(strlen(buffer) > 16)
    {
     c = 0;
     return 1;
    }
  
 
 // check if the name is legal
 for(int i = 0; i < strlen(buffer); i++)
 {
   // spaces in name are not allowed
  if(buffer[i] == ' ')
  {
   c = 2;
   return 1;
  }
 }
 
 //check if the name already exist in the list
 client temp;
 for(int i = 0; i < client_list.size(); ++i)
 {
   temp = client_list.at(i);
   if(buffer == temp.name)
   {
     c = 3;
     return 1;
   }
     
 }

// No issues were found!
n = buffer; // asign the name in the buffer to n for the caller function to use
     
 // everthing went well return 0
    return 0;
 }

void *LaunchMemberFunction(void *obj)
{
    ThreadLauncher *myLauncher = reinterpret_cast<ThreadLauncher *>(obj);
    return myLauncher->launch();
}
void *LaunchMemberFunction2(void *obj)
{
    ThreadLauncher2 *myLauncher = reinterpret_cast<ThreadLauncher2 *>(obj);
    return myLauncher->launch();
}

void chatnet::Help(unsigned int sock)
{
    char buffer[512];
 strcpy(buffer,"/list - Display list of connected users.\n");
 strcat(buffer,"/msg <name> :<msg> - Send message to user.\n");
 strcat(buffer,"/help - Display help.\n");
 strcat(buffer,"/exit - Disconnect from chatnet.\n");
 if(send(sock,buffer,strlen(buffer),0) == -1)
   return;
 
}


//----------------- This function removes a client for the client_list vector -----------------//
void chatnet::RemoveClient(int position)
{        pthread_mutex_lock(&mutex);
       client_list.erase(client_list.begin()+position);
       pthread_mutex_unlock(&mutex);
}


//if the name is found return the position of the clients details in client_list vector
// returns -1 if the name is not found
int chatnet::FindClient(const char * c_name,unsigned int & socket)
{
 int found;
 client temp;
 char buffer[32];
 found = -1; // will always equal -1, not until the name is found
 for(int i = 0; i < client_list.size(); i++)
 {
  temp = client_list.at(i);
  
  // is this the right client?
  if(temp.name == c_name)
  {
    found = i;
    break;
  }
}
 
 
 // Client not found in the list
  if(found == -1)
 {
   strcpy(buffer,c_name);
   strcat(buffer," has not been found!\n");
   if(send(socket,buffer,strlen(buffer),0) == -1)
    return -1;
 }
 return found;
}

//----------------- This function blocks and waits for data until a it's time to timeput -------------------//
//----------------- if no data is received 1 is return ----------------------------------------------------//
//------------------ if a timeout then 2 is returned -----------------------------------------------------//
int chatnet::recvtimeout(unsigned int sock,char * buffer,int size,int timeout)
{
struct timeval tv;
tv.tv_sec = timeout;
tv.tv_usec = 0;
int recv_bytes;
fd_set readfds;
while(true)
{
  FD_ZERO(&readfds);
  FD_SET(sock,&readfds);
 rv = select(sock + 1,&readfds,NULL,NULL,&tv);
 if(rv == -1)
  return -1;     
 if(rv)
 {
 if(FD_ISSET(sock,&readfds))
 {
  recv_bytes = recv(sock,buffer,size,0);
  if(recv_bytes == -1 || recv_bytes == 0)
    return 1;
  buffer[recv_bytes -1] = 0;
 // no problem
  break;
}

}
// recv timeout
else
  return 2;

}

// success 
return 0;
}

//-------------- This function is a thread function that is started for every client --------------//
void * chatnet::Pinger(void * sock)
{
  
  unsigned int socket_fd = *(unsigned int *) sock;
  while(true)
  {
    sleep(5);
    //-------------- send 'PING' to client until an error ----------//
   if(send(socket_fd,"PING",4,0) == -1)
     return NULL;
  }
}

chatnet::~chatnet()
{
 
}
