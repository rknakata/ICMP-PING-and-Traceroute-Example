#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h> //ip hdeader library (must come before ip_icmp.h)
#include <netinet/ip_icmp.h> //icmp header
#include <arpa/inet.h> //internet address library
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> // added
#include <time.h> // added
#include <signal.h> // added
#include <arpa/inet.h>

#include "checksum.h" //my checksum library

#define BUFSIZE 1500 //1500 MTU (so within one frame in layer 2)
#define PROTO_ICMP 1
int packetsSent = 0;
int packetSize = 0;

//get packet to return time
// char sentTimeBuffer[30]; // holds the string of delay
// char receivedTimeBuffer[30]; // holds the string of delay
struct timeval sent;
struct timeval received;

time_t sentTime;
time_t receivedTime;
long delay = 0;
long totalRoundTripTime = 0;

// used for alarm signal handler
int firstRun = 0; // change this to 1 after the first run finishes
int globalVar = 0;
int count = 0;
int success = 0; // increment this when a packet is successfuly sent
int fail = 0; // increment this when a packet receive fails
char * argsGlobal = NULL; // not a good idea
//https://stackoverflow.com/questions/6970224/providing-passing-argument-to-signal-handler


void handler(int signum) { //signal handler

  // set global variables everytime this is called
  count++; // alarm was delivered increment the count
  globalVar = 1; // this is checked in mains while loop

  //printf("%s delete me \n",argsGlobal);

  char sendbuf[BUFSIZE], recvbuf[BUFSIZE], controlbuf[BUFSIZE];
  struct icmp * icmp;
  struct ip * ip;
  int sockfd;
  int packet_len, recv_len, ip_len, data_len;
  struct addrinfo * ai;
  struct iovec iov;
  struct msghdr msg;

  //process addr info
  getaddrinfo(argsGlobal, NULL, NULL, &ai);
  // printf("%s delete",ai->ai_addr->sa_data);

//https://stackoverflow.com/questions/20115295/how-to-print-ip-address-from-getaddrinfo
if(firstRun == 0){
  struct sockaddr_in *addr;
  addr = (struct sockaddr_in *)ai->ai_addr;
  printf("PING %s (%s) *unkown values read man page* bytes of data.\n", ai->ai_canonname ? ai->ai_canonname : argsGlobal, inet_ntoa((struct in_addr)addr->sin_addr));
  firstRun = 1; // this message wont print again
}
//***********************************************************************************
  // //process destination address
  // printf("Dest: %s\n", ai->ai_canonname ? ai->ai_canonname : argsGlobal);
  //*********************************************************************************

  //Initialize the socket
  if((sockfd = socket(AF_INET, SOCK_RAW, PROTO_ICMP)) < 0){
    perror("socket"); //check for errors
    exit(1);
  }

  //initiate ICMP header
  icmp = (struct icmp *) sendbuf; //map to get proper layout
  icmp->icmp_type = ICMP_ECHO; //Do an echoreply
  icmp->icmp_code = 0;
  icmp->icmp_id = 42;
  icmp->icmp_seq= 0;
  icmp->icmp_cksum = 0;

  //compute checksum
  icmp->icmp_cksum = checksum((unsigned short *) icmp, sizeof(struct icmp));

  packet_len = sizeof(struct icmp);

  //send the packet
  gettimeofday(&sent, NULL);
   sentTime = sent.tv_sec;
//    strftime(sentTimeBuffer,30,"%m-%d-%Y  %T.",localtime(&sentTime));
// printf("%s%ld\n",sentTimeBuffer,sent.tv_usec);
  if( sendto(sockfd, sendbuf, packet_len, 0, ai->ai_addr, ai->ai_addrlen) < 0){
    perror("sendto");//error check
    exit(1);
  }
  else{
    success ++;
  }

  //built msgheader structure for receiving reply
  iov.iov_base = recvbuf;
  iov.iov_len = BUFSIZE;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control=controlbuf;
  msg.msg_controllen=BUFSIZE;

  //recv the reply
  if((recv_len = recvmsg(sockfd, &msg, 0)) < 0){ //could get interupted ??
    perror("recvmsg");
    fail ++;
    exit(1);
  }

  receivedTime = received.tv_sec;
  gettimeofday(&received, NULL);
  receivedTime = received.tv_sec;
  // strftime(receivedTimeBuffer,30,"%m-%d-%Y  %T.",localtime(&receivedTime));
  // printf("%s%ld\n",receivedTimeBuffer,received.tv_usec);

  delay = (received.tv_sec - sent.tv_sec) * 1000000 + received.tv_usec - sent.tv_usec; // useq is a milionth of a second ms is microsecond which is 1/1000th of a second
  totalRoundTripTime = delay + totalRoundTripTime;
  float delayFloat = delay;
  delayFloat = delayFloat / 1000;
  printf("time = %.1f ms\n", delayFloat);
//   if(delay >= 0){}
//   totalRoundTripTime = delay + totalRoundTripTime;
//
// //put the stats here
//
//
//   }
//   else{ // the time is negative the interrupt didnt let the function finish
//     perror("negative time")
//   }
  delay = 0; // set global variable back to 0

  //clean globalVar structs
  memset(&sent, 0, sizeof(sent));
  memset(&received, 0, sizeof(received));

  //recv_len packet print this was here originally
  printf("%d\n",recv_len); //this is the amount of bytes received? no what is this

  ip = (struct ip*) recvbuf;
  ip_len = ip->ip_hl << 2; //length of ip header

  icmp = (struct icmp *) (recvbuf + ip_len);
  data_len = (recv_len - ip_len);

  printf("%d bytes from %s\n", data_len, inet_ntoa(ip->ip_src));
}

// print stats when it closes
void sighandler(int signum) {
  float totalRoundTripTimeFloat = totalRoundTripTime;
  totalRoundTripTimeFloat = totalRoundTripTimeFloat / 1000;
  // --- google.com ping statistics --- 5 packets transmitted, 5 received, 0% packet loss, time 4005ms rtt
//min/avg/max/mdev = 8.796/9.003/9.411/0.236 ms
  printf("\n--- %s ping statistics ---\n",argsGlobal);
  float packetLossPercent = 0.0f;
  // fail = 2; used to test
  packetLossPercent = ((float) fail / (float)success) * 100;
  int packetLossPercentInt = packetLossPercent;
  printf("%d %d delete\n", fail, success);
  printf("%d packets transmitted, %d received, %d%% percent packet loss, time = %.1f ms now exiting...\n", success, success - fail, packetLossPercentInt,totalRoundTripTimeFloat);
  exit(1);
}

int main(int argc, char * argv[]){
  argsGlobal = argv[1];

 // // char *ip = inet_ntoa(ai->ai_addr);
 //  if(inet_ntoa(ai.ai_addr) != NULL){
 //    printf("inet_ntoa() works delete\n");
 // }
 // else{
 //   printf("error occured getting IP");
 //   exit(1);
 // }
  //  pthread_t thread_id;
  //  pthread_create(&thread_id, NULL, handlerCaller, NULL);
  //  printf("test\n");
  //  pthread_join(thread_id, NULL);
  //signal(SIGALRM,handler); //register handler to handle SIGALRM
  signal(SIGINT, sighandler);
      signal(SIGALRM,handler); //register handler to handle SIGALRM
  while(1){
    alarm(1); //Schedule a SIGALRM for 1 second
    while(globalVar != 1);//busy wait for signal to be delivered
    //printf("change this to print stats\n");
    globalVar = 0;
  }
    return 0;
}
