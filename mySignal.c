/* mySignal.c
Program solutions to the following problems by extending MySignal.c:
1. Change MySignal.c such that after the handler is invoked, an additional printf("Turing was right!\n")
occurs in main() before exiting. You will probably need to use a global variable and change the condition
on the while loop.
2. Change hello signal.c such that every second, first “Hello World!” prints from the signal handler
followed by “Turing was right!” in main(), over and over again indefinitely. The output should look like:
Hello World!
Turing was right!
Hello World!
Turing was Right!
 ...

 3. Program a new program timer.c that after exiting (via CTRL-C), will print out the total time the
program was executing in seconds. To accomplish this task, you will need to register a second signal
handler for the SIGINT signal, the signal that is delivered when CTRL-C is pressed. Conceptually, your
program will request a SIGALRM signal to occur every second, tracking the number of alarms delivered,
and when the program exits via CTRL-C, it will print how many alarms occurred, or the number of
seconds it was executed.

***Original Code***

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
void handler(int signum) { //signal handler
  printf("Hello World!\n");
  exit(1); //exit after printing
}
int main(int argc, char * argv[]){
  signal(SIGALRM,handler); //register handler to handle SIGALRM
  alarm(1); //Schedule a SIGALRM for 1 second
  while(1); //busy wait for signal to be delivered
  return 0; //never reached
}

*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

int globalVar = 0;

void handler(int signum) { //signal handler
  printf("Hello World!\n");
  globalVar = 1; // this is checked in mains while loop
  // signal(SIGALRM,main); //register handler to handle SIGALRM
  // alarm(1);
  exit(1); //exit after printing
}

void *handlerCaller(){
signal(SIGALRM,handler); //register handler to handle SIGALRM
alarm(1); //Schedule a SIGALRM for 1 second
}

int main(int argc, char * argv[]){
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, handlerCaller, NULL);
  printf("test\n");
  pthread_join(thread_id, NULL);
  // signal(SIGALRM,handler); //register handler to handle SIGALRM
  // alarm(1); //Schedule a SIGALRM for 1 second
  //https://stackoverflow.com/questions/10600250/is-it-necessary-to-call-pthread-join
  while(globalVar != 1);//busy wait for signal to be delivered
  printf("Turing was right!\n");
  return 0; //never reached
}
