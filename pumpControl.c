// Demo source code using the ViSi-Genie-RaspPi library
// 4D Systems
// This demo communicates with an intelligent 4D Systems display, configured with ViSi-Genie.
// The display has a slider, a cool gauge, an LED digits and a string box.
// The program receives messages from the slider, which is cofigured to report a message when its status has changed.
// The values received from the slider are written to the LED digits using the function genieWriteObj(). 
// The coolgauge is written to using the function genieWriteObj(), 
// and the string is updated using the function genieWriteStr().

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <math.h>
#include <sys/socket.h>
#include <netdb.h>
#include <geniePi.h>  //the ViSi-Genie-RaspPi library

int socketfd;
char outps[80];
int Debug = 1;

#define  PUMP_ADDRESS  "192.168.0.116"
#define  PUMP_SOCKET "23"

void sendtoPump(char *msg) {
  char rtn[80];
  send(socketfd,msg,sizeof(msg)+1,0);
  recv(socketfd,rtn,80,0);
  if(Debug)printf("%s\n",rtn);	
}
//This a thread for writing to the cool gauge. The value of
//the cool gauge will change from 0 to 99 and then from 99 to 0. 
//This process repeats forever, running in parallel with the main.

static void *handleCoolGauge(void *data)
{
  int gaugeVal = 0;   //holds the value of the cool gauge
  int step = 1;       //increment or decrement value, initialized to 1.
  
  for(;;)             //infinite loop
  {
    //write to Coolgauge0
    genieWriteObj(GENIE_OBJ_COOL_GAUGE, 0x00, gaugeVal);
    usleep(10000);    //10-millisecond delay
    gaugeVal += step; //increment or decrement
    if(gaugeVal == 99)	step = -1;
    if(gaugeVal == 0)	step = 1;
  }
  return NULL;
}
//This is the event handler. Messages received from the display
//are processed here.
void handleGenieEvent(struct genieReplyStruct * reply)
{
  if(Debug)printf("Event: command: %2d, object: %2d, index: %d, data: %d \r\n", reply->cmd, reply->object, reply->index, reply->data);
  if(reply->cmd == GENIE_REPORT_EVENT)    //check if the cmd byte is a report event
  {
    if(reply->object == GENIE_OBJ_SLIDER) //check if the object byte is that of a slider
      {

        if(reply->index == 0){		  //check if the index byte is that of Slider0
          //write to the LED digits object	
          genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x00, reply->data);
	  sprintf(outps,"S%d",reply->data);
	  sendtoPump(outps);
	 }
      }
    if(reply->object == GENIE_OBJ_4DBUTTON) //check if the object byte is that of a slider
      {

        if(reply->index == 0)		  //Init Button
  	  sendtoPump("I");
        if(reply->index == 1){            //Direction Switch
	  if(reply->data == 0)
	    sendtoPump("C");
          else sendtoPump("A");
	}
        if(reply->index == 2) {           //Start / Stop 
	  if(reply->data == 0)
	     sendtoPump("G");
          if(reply->data == 1) 
	     sendtoPump("E");
        }
       if(reply->index == 3) {            // Brake /UnBrake
	  if(reply->data == 0)
	     sendtoPump("U");
          if(reply->data == 1) 
	     sendtoPump("B");
        }
      }
  }
  
  //if the received message is not a report event, print a message on the terminal window
  else
    printf("Unhandled event: command: %2d, object: %2d, index: %d, data: %d \r\n", reply->cmd, reply->object, reply->index, reply->data);
 
}

int main()
{
  int status;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;

  pthread_t myThread;              //declare a thread
  struct genieReplyStruct reply ;  //declare a genieReplyStruct type structure
  memset( &host_info, 0 , sizeof host_info);
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  if ( socketfd == -1) printf("socket error\n");
 //print some information on the terminal window
  printf("\n\n");
  printf("PumpControl Interface SW\n");
  printf("==================================\n");
  printf("Program is running. Press Ctrl + C to close.\n");
  status = getaddrinfo(PUMP_ADDRESS,PUMP_SOCKET, &host_info, &host_info_list);
  socketfd = socket(host_info_list->ai_family,host_info_list->ai_socktype,host_info_list->ai_protocol);
  if (status != 0) printf("Bad inet status\n");
  status = connect(socketfd,host_info_list->ai_addr,host_info_list->ai_addrlen);
  if(status == -1) printf("connect error\n");
  //open the Raspberry Pi's onboard serial port, baud rate is 115200
  //make sure that the display module has the same baud rate
  genieSetup("/dev/ttyAMA0", 115200);  
  //start the thread for writing to the cool gauge	
  (void)pthread_create (&myThread,  NULL, handleCoolGauge, NULL);
  sendtoPump("i"); 
 // sendtoPump("S1234");
 // sendtoPump("S2345");

 // genieWriteStr(0x00, outps); //write to Strings0
  for(;;)                         //infinite loop
  {
    while(genieReplyAvail())      //check if a message is available
    {
      genieGetReply(&reply);      //take out a message from the events buffer
      handleGenieEvent(&reply);   //call the event handler to process the message
    }	
    usleep(10000);                //10-millisecond delay.Don't hog the 
    
  }	                          //CPU in case anything else is happening
  freeaddrinfo(host_info_list);
  close(socketfd);
  return 0;
}

