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
#define NI_MAXHOST 1025 // http://www.microhowto.info/howto/convert_an_ip_address_to_the_corresponding_domain_name_in_c.html

int packetsSent = 0;
int packetSize = 0;

int noFailure = 0;

//get packet to return time
// char sentTimeBuffer[30]; // holds the string of delay
// char receivedTimeBuffer[30]; // holds the string of delay
struct timeval sent;
struct timeval received;
struct timeval programStart;// this is used in the console when sig int is received
struct timeval programEnd;// this is used in the console when sig int is received

time_t sentTime;
time_t receivedTime;
time_t programStartTime;
time_t programEndTime;
long delay = 0;
long totalRoundTripTime = 0;
long programRunTime; // this is printed in the console when sig int is received

// used for alarm signal handler
int firstRun = 0; // change this to 1 after the first run finishes
int globalVar = 0;
int count = 0;
int success = 0; // increment this when a packet is successfuly sent
int fail = 0; // increment this when a packet receive fails
char * argsGlobal = NULL; // not a good idea
int icmpReqCount = 0;
//https://stackoverflow.com/questions/6970224/providing-passing-argument-to-signal-handler

float minDelayFloat = 0; // min
float avgDelayFloat = 0; // avg
float maxDelayFloat = 0; // max
float standardDeviation = 0; // mdev
float totalDelayFloat = 0;
float devArray[100];
int devCount = 0;
//make this dynamic if i work on this in the future

char hostname[NI_MAXHOST] = "";
//need to implement fail dropped packets


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
  //find the ttl
  //example http://man7.org/linux/man-pages/man3/cmsg.3.html
  //struct msghdr msgh;
  // struct cmsghdr *cmsg;
  // int *ttlptr;
  // int received_ttl;
  //http://www.microhowto.info/howto/convert_an_ip_address_to_the_corresponding_domain_name_in_c.html
  struct sockaddr_in *addr; // this was in first run


  //process addr info
  getaddrinfo(argsGlobal, NULL, NULL, &ai);
  // printf("%s delete",ai->ai_addr->sa_data);

//https://stackoverflow.com/questions/20115295/how-to-print-ip-address-from-getaddrinfo
if(firstRun == 0){
  //struct sockaddr_in *addr;
  addr = (struct sockaddr_in *)ai->ai_addr;
  // printf("PING %s (%s) *unkown values read man page* bytes of data.\n", ai->ai_canonname ? ai->ai_canonname : argsGlobal, inet_ntoa((struct in_addr)addr->sin_addr));
    printf("PING %s (%s) 56(84) bytes of data.\n", ai->ai_canonname ? ai->ai_canonname : argsGlobal, inet_ntoa((struct in_addr)addr->sin_addr));
    //change this to be like original
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
  // gettimeofday(&sent, NULL);
  //  sentTime = sent.tv_sec;
//    strftime(sentTimeBuffer,30,"%m-%d-%Y  %T.",localtime(&sentTime));
// printf("%s%ld\n",sentTimeBuffer,sent.tv_usec);
  if( sendto(sockfd, sendbuf, packet_len, 0, ai->ai_addr, ai->ai_addrlen) < 0){
    perror("sendto");//error check
    exit(1);
  }
  else{
    success ++;
    icmpReqCount ++;
  }
  gettimeofday(&sent, NULL);
   sentTime = sent.tv_sec;
   //built msgheader structure for receiving reply
   iov.iov_base = recvbuf;
   iov.iov_len = BUFSIZE;

   msg.msg_name = NULL;
   msg.msg_namelen = 0;
   msg.msg_iov = &iov;
   msg.msg_iovlen = 1;
   msg.msg_control=controlbuf;
   msg.msg_controllen=BUFSIZE;

  // //built msgheader structure for receiving reply
  // iov.iov_base = recvbuf;
  // iov.iov_len = BUFSIZE;
  //
  // msg.msg_name = NULL;
  // msg.msg_namelen = 0;
  // msg.msg_iov = &iov;
  // msg.msg_iovlen = 1;
  // msg.msg_control=controlbuf;
  // msg.msg_controllen=BUFSIZE;

  //recv the reply
  // gettimeofday(&received, NULL);
  // receivedTime = received.tv_sec;
  //-W timeout
  //       Time to wait for a response, in seconds. The option affects only
  //       timeout  in  absence  of any responses, otherwise ping waits for
  //       two RTTs.
  // did not implement maybe in future
  if((recv_len = recvmsg(sockfd, &msg, 0)) < 0){ //could get interupted ??
    perror("recvmsg");
    fail ++;
    exit(1);
  }

  //receivedTime = received.tv_sec;
  gettimeofday(&received, NULL);
  receivedTime = received.tv_sec;
  // strftime(receivedTimeBuffer,30,"%m-%d-%Y  %T.",localtime(&receivedTime));
  // printf("%s%ld\n",receivedTimeBuffer,received.tv_usec);

  delay = (received.tv_sec - sent.tv_sec) * 1000000 + received.tv_usec - sent.tv_usec; // useq is a milionth of a second ms is microsecond which is 1/1000th of a second
  //delay = received.tv_usec - sent.tv_usec; // useq is a milionth of a second ms is microsecond which is 1/1000th of a second
  totalRoundTripTime = delay + totalRoundTripTime;
  // totalRoundTripTime = delay + totalRoundTripTime;
  float delayFloat = delay;
  delayFloat = delayFloat / 1000;


  if(firstRun == 0 || minDelayFloat > delayFloat){
    minDelayFloat = delayFloat;
  }

  if(firstRun == 0 || maxDelayFloat < delayFloat){
    maxDelayFloat = delayFloat;
  }


    totalDelayFloat = totalDelayFloat + delayFloat;
    devArray[devCount] = delayFloat;
    devCount++;

noFailure ++;

  // float minDelayFloat = 0; // min
  // float avgDelayFloat = 0; // avg
  // float maxDelayFloat = 0; // max
  // float standardDeviation = 0; // mdev



  // printf("time = %.1f ms\n", delayFloat);
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

//*****************************************
  //recv_len packet print this was here originally
//  printf("%d\n",recv_len); //this is the amount of bytes received? no what is this
//************************************************
  ip = (struct ip*) recvbuf;
  ip_len = ip->ip_hl << 2; //length of ip header

  icmp = (struct icmp *) (recvbuf + ip_len);
  data_len = (recv_len - ip_len);


// https://unix.superglobalmegacorp.com/Net2/newsrc/netinet/ip.h.html
//   struct ip {
// #if BYTE_ORDER == LITTLE_ENDIAN
// 	u_char	ip_hl:4,		/* header length */
// 		ip_v:4;			/* version */
// #endif
// #if BYTE_ORDER == BIG_ENDIAN
// 	u_char	ip_v:4,			/* version */
// 		ip_hl:4;		/* header length */
// #endif
// 	u_char	ip_tos;			/* type of service */
// 	short	ip_len;			/* total length */
// 	u_short	ip_id;			/* identification */
// 	short	ip_off;			/* fragment offset field */
// #define	IP_DF 0x4000			/* dont fragment flag */
// #define	IP_MF 0x2000			/* more fragments flag */
// 	u_char	ip_ttl;			/* time to live */
// 	u_char	ip_p;			/* protocol */
// 	u_short	ip_sum;			/* checksum */
// 	struct	in_addr ip_src,ip_dst;	/* source and dest address */
// };
//
  int reply_ttl = ip->ip_ttl;


  //  /* Receive auxiliary data in msgh */
  //
  //  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
  //    if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
  //      ttlptr = (int *) CMSG_DATA(cmsg);
  //      received_ttl = *ttlptr;
  //      break;
  //    }
  //  }
  //
  // if (cmsg == NULL) { // could not extract ttl
  //  /* Error: IP_TTL not enabled or small buffer or I/O error */
  //  perror("could not extract ttl from packet");
  // }




  //
  //
  // struct sockaddr_storage addr;
  // socklen_t addr_len=sizeof(addr);
  // int err=getpeername(sock_fd,(struct sockaddr*)&addr,&addr_len);
  // if (err!=0) {
  //     die("failed to fetch remote address (errno=%d)",errno);
  // }








  // char urlHost[NI_MAXHOST];
  // int err = getnameinfo(ai->ai_addr,sizeof(ai->ai_addr),urlHost,sizeof(urlHost),0,0,0);
  // if (err!=0) {
  //     printf("Remote address: %s\n",urlHost);
  //     perror("failed to convert address to string");
  // }
  // printf("Remote address: %s\n",urlHost);
  //http://minirighi.sourceforge.net/html/structip.html
  //data_len = (recv_len + ip_len); if i use this i get 68 bytes back from google check this out in future might need to subtract ipversion or ip header length
//printf("time = %.1f ms\n", delayFloat);
  printf("%d bytes from %s (%s): icmp_req=%d ttl=%d time=%.1f ms\n", data_len, hostname, inet_ntoa(ip->ip_src), icmpReqCount, reply_ttl, delayFloat);
  if(firstRun == 0){
    firstRun = 1; // this message wont print again
  }

}

// print stats when it closes
void sighandler(int signum) {
  float totalRoundTripTimeFloat = totalRoundTripTime;
  totalRoundTripTimeFloat = totalRoundTripTimeFloat / 1000;
  // --- google.com ping statistics --- 5 packets transmitted, 5 received, 0% packet loss, time 4005ms rtt
//min/avg/max/mdev = 8.796/9.003/9.411/0.236 ms
  gettimeofday(&programEnd, NULL);
  programEndTime = programEnd.tv_sec;
  programRunTime = (programEnd.tv_sec-programStart.tv_sec)*1000000 + programEnd.tv_usec-programStart.tv_usec;
  programRunTime = programRunTime / 1000;

  printf("\n--- %s ping statistics ---\n",argsGlobal);
  float packetLossPercent = 0.0f;
  // fail = 2; used to test
  fail = success - noFailure;
  packetLossPercent = ((float) fail / (float)success) * 100;
  int packetLossPercentInt = packetLossPercent;
  // printf("%d %d delete\n", fail, success); // testing percent packet packet Loss
  //https://www.youtube.com/watch?v=pFGcMIL2NVo

  // calculate the standardDeviation
  //https://www.wikihow.com/Calculate-Standard-Deviation


// standardDeviation uses last 100 values
  int newSuccess = 0;
  newSuccess = success;

  if(newSuccess > 100){
    newSuccess = 100;
  }

  if(devCount > 100){
    devCount = 100;
  }

  for(int i = 0; i < devCount; i++){
    devArray[i] = devArray[i] - (totalDelayFloat / success);
  }
  //printf("delete %.1f\n", totalDelayFloat);
  for(int i = 0; i < devCount; i++){
    devArray[i] = devArray[i] * devArray[i];
  }
  float addSquare = 0;
  for(int i = 0; i < devCount; i++){
    addSquare = addSquare + devArray[i];
  }

  standardDeviation = addSquare / (success-1);

  printf("%d packets transmitted, %d received, %d%% percent packet loss, time %ldms\nrtt min/avg/max/mdev = %.1f/%.1f/%.1f/%.1f ms\n", success, success - fail, packetLossPercentInt,programRunTime, minDelayFloat, totalDelayFloat / success, maxDelayFloat, standardDeviation);
  //printf("%d packets transmitted, %d received, %d%% percent packet loss, time = %.0f ms now exiting...\n", success, success - fail, packetLossPercentInt,totalRoundTripTimeFloat);
  exit(1);
}

int main(int argc, char * argv[]){
  struct addrinfo *result;
  struct addrinfo *res;
  int error;


  /* resolve the domain name into a list of addresses */
  error = getaddrinfo(argv[1], NULL, NULL, &result);
  if (error != 0)
  {
      fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
      return EXIT_FAILURE;
  }

  //fix this later
  /* loop over all returned results and do inverse lookup */
  for (res = result; res != NULL; res = res->ai_next)
  {
      // char hostname[NI_MAXHOST] = "";


      error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0);
      if (error != 0)
      {
          fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
          continue;
      }
      if (*hostname != '\0'){
          //hostname is sets

          // printf("hostname: %s\n", hostname);// char hostname[NI_MAXHOST] = "";
      }
  }


  freeaddrinfo(result);


  gettimeofday(&programStart, NULL);
  programStartTime = programStart.tv_sec;
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
