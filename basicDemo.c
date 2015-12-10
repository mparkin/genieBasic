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

#include <geniePi.h>  //the ViSi-Genie-RaspPi library


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
  if(reply->cmd == GENIE_REPORT_EVENT)    //check if the cmd byte is a report event
  {
    if(reply->object == GENIE_OBJ_SLIDER) //check if the object byte is that of a slider
      {
        if(reply->index == 0)		  //check if the index byte is that of Slider0
          //write to the LED digits object	
          genieWriteObj(GENIE_OBJ_LED_DIGITS, 0x00, reply->data);			
      }
  }
  
  //if the received message is not a report event, print a message on the terminal window
  else
    printf("Unhandled event: command: %2d, object: %2d, index: %d, data: %d \r\n", reply->cmd, reply->object, reply->index, reply->data);
}

int main()
{
  pthread_t myThread;              //declare a thread
  struct genieReplyStruct reply ;  //declare a genieReplyStruct type structure
  
  //print some information on the terminal window
  printf("\n\n");
  printf("Visi-Genie-Raspberry-Pi basic demo\n");
  printf("==================================\n");
  printf("Program is running. Press Ctrl + C to close.\n");

  //open the Raspberry Pi's onboard serial port, baud rate is 115200
  //make sure that the display module has the same baud rate
  genieSetup("/dev/ttyAMA0", 115200);  
  //start the thread for writing to the cool gauge	
  (void)pthread_create (&myThread,  NULL, handleCoolGauge, NULL);

  genieWriteStr(0x00, "Hello world!"); //write to Strings0
  for(;;)                         //infinite loop
  {
    while(genieReplyAvail())      //check if a message is available
    {
      genieGetReply(&reply);      //take out a message from the events buffer
      handleGenieEvent(&reply);   //call the event handler to process the message
    }	
    usleep(10000);                //10-millisecond delay.Don't hog the 
  }	                          //CPU in case anything else is happening
  return 0;
}

