#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc, free, srand, rand */

/*******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1 /* change to 1 if you're doing extra credit */
                        /* and write a routine called B_output */
/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
// 계층 5에서 4로 전달되는 데이터 단위임 msg.
struct msg
{
  char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt
{
  int seqnum;
  int acknum;
  int checksum;     //4bit checksum -> 4bit씩 나눠서 계싼해야됨
  char payload[20]; //
};

struct event;
/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define WIN_SIZE 50
#define TIME_OUT 15.0
/* A =0 B=1*/
/* Here I define some function prototypes to avoid warnings */
/* in my compiler. Also I declared these functions void and */
/* changed the implementation to match */

void init();
void generate_next_arrival();
void insertevent(struct event *p);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char *datasent);
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
int make_CheckSum(struct pkt packet);
///////////////my define section for make go back n////////////////////////////
int nextSeqnum[2] = {
    0,
}; //for sending (it is next seqnum for A=0 or B=1)
struct pkt window_buffer[2][WIN_SIZE] = {
    0,
}; //for save packet in window
int base[2] = {
    0,
}; //for save info where is base?
int expectseqNum[2] = {
    0,
}; //for receiver. this is must correct packet.seqnum.
int bufferNum[2] = {
    0,
}; //for saving window packet.. - It is round window buffer
/*for piggyback*/
int AckNum[2] = {-1, -1}; //It is save Acknumber for next packet
/////////////////////
struct pkt make_pkt(int AorB, struct msg message) //make packet function - seqnum = nuextSeqnum, acknum = Acknum
{
  struct pkt packet;
  strncpy(packet.payload, message.data, 20);
  packet.seqnum = nextSeqnum[AorB];
  packet.acknum = AckNum[AorB];
  packet.checksum = make_CheckSum(packet);
  return packet;
}

//checksum 만드는 함수
int make_CheckSum(struct pkt packet) //for making checksum. this is sum of seq,ack,payload bit and I reversed it.
{
  int i = 0, checksum = 0;
  checksum += packet.seqnum;
  checksum += packet.acknum;
  for (i = 0; i < 20; i++)
  {
    checksum += (int)packet.payload[i];
  }
  checksum = (~checksum);
  return checksum;
}
/////////////////////////////////////end section////////////////////////////

//Name: A_output()
//description: layer4 get data from layer5 by this function. if Window size is not full ->
//save packet for resending. After send packet, nextseqnum is increased.
//if buffer is full packet is loss...
void A_output(struct msg message) //message is from layer 5 data
{
  struct pkt packet; //make packet.
  printf("A-output: get message from layer5 : %.*s\n", 19, message.data);
  if (nextSeqnum[0] < base[0] + WIN_SIZE) //if buffer is not full.
  {
    packet = make_pkt(0, message);           //make packet for sending
    window_buffer[0][bufferNum[0]] = packet; //save packet for resending
    tolayer3(0, packet);                     //send packet
    printf("  [success]Send Message [ack]:%d, [seq]:%d\n\n", packet.acknum, packet.seqnum);
    if (nextSeqnum[0] == base[0]) //if current process is First transmission of an empty window
    {
      starttimer(0, TIME_OUT); //timer start
    }
    nextSeqnum[0] += 1;
    bufferNum[0] = nextSeqnum[0] % WIN_SIZE;
  }
  else //if buffer is full..
  {
    printf("  [error]buffer full. drops message\n\n");
  }
}

//name:B_output
//Descript:is same A_output. diffrence is AorB. so The sequence number in the array is 1.
void B_output(struct msg message)
{
  struct pkt packet;
  message.data[20] = '\0';
  printf("B-output: get message from layer5 :%.*s\n", 19, message.data);
  if (nextSeqnum[1] < base[1] + WIN_SIZE)
  {
    packet = make_pkt(1, message);
    window_buffer[1][bufferNum[1]] = packet;
    tolayer3(1, packet);
    printf("  [success]Send Message [ack]:%d, [seq]:%d\n\n", packet.acknum, packet.seqnum);
    if (nextSeqnum[1] == base[1])
    {
      starttimer(1, TIME_OUT);
    }
    nextSeqnum[1] += 1;
    bufferNum[1] = nextSeqnum[1] % WIN_SIZE;
  }
  else
  {
    printf("  [error]buffer full. drops message\n\n");
  }
}

//Name: A_input()
//description: this get data from layer3. data is from B. first check is corrupt.
//The second test came in order. After cheking, send data layer5. and then operate the timer.
void A_input(struct pkt packet) //packet is from B. This data is a parameter because it has to be processed.
{
  printf("A-input:Got packet from B  [data]:%.*s, [acknum]:%d, [seqnum]:%d\n", 19, packet.payload, packet.acknum, packet.seqnum);
  if (make_CheckSum(packet) != packet.checksum) //chekcing CheckSum!
  {
    printf("  [error]checkSum error.Maybe packet is corrupt.\n\n");
    return;
  }
  if (packet.acknum != -1 && (packet.acknum < base[0])) //checking data is already received.
  {
    printf("  [error]Maybe packet is already received. \n\n");
    return;
  }
  if (packet.seqnum == expectseqNum[0]) //If the data came in order
  {
    tolayer5(0, packet.payload); //send data to layer5
    printf("  [success]Got packet\n\n");
    expectseqNum[0]++;             //expect order increase.
    AckNum[0] = packet.seqnum + 1; //save Acknum data -> It will contain the data to send data.
    if (packet.acknum != -1)       //first data input don't need timer - anything send B.
    {
      if (base[0] < packet.acknum) // set base.
        base[0] = packet.acknum;
      if (base[0] == nextSeqnum[0]) //if Nothing was sent, stop timer.
      {
        stoptimer(0);
      }
      else //If there is a packet being sent, timer reset.
      {
        stoptimer(0);
        starttimer(0, TIME_OUT);
      }
    }
  }
  else //data came in inorder....
  {
    printf("  [error] The order of data is different from seqnum.\n\n");
    return;
  }
}

//Name: B_input()
//description: Same is A_input...
void B_input(struct pkt packet)
{
  printf("B-input:Got packet from B  [data]:%.*s, [acknum]:%d, [seqnum]:%d\n", 19, packet.payload, packet.acknum, packet.seqnum);
  if (make_CheckSum(packet) != packet.checksum)
  {
    printf("  [error]checkSum error.Maybe packet is corrupt.\n\n");
    return;
  }
  if (packet.acknum != -1 && (packet.acknum < base[1]))
  {
    printf("  [error]Maybe packet is already received. \n\n");
    return;
  }
  if (packet.seqnum == expectseqNum[1])
  {
    tolayer5(1, packet.payload);
    printf("  [success]Got packet\n\n");
    expectseqNum[1]++;
    AckNum[1] = packet.seqnum + 1;
    if (packet.acknum != -1)
    {
      if (base[1] < packet.acknum)
        base[1] = packet.acknum;
      printf("B-input:Save acknum = %d \n\n", AckNum[1]);
      if (base[1] == nextSeqnum[1])
      {
        stoptimer(1);
      }
      else
      {
        stoptimer(1);
        starttimer(1, TIME_OUT);
      }
    }
  }
  else
  {
    printf("  [error] The order of data is different from seqnum.\n\n");
    return;
  }
}

//Name: A_timerinterrupt()
//description: it is called when timer intterupt. so this function execute resend process
//Send all packets in the buffer. - At the same time, set the timer.
void A_timerinterrupt()
{
  struct pkt packet;          //for send packet
  int i = base[0] % WIN_SIZE; //change base -> window buffer index. for Iterate through window
  printf("A-timerinterrupt: resend packets...\n");
  starttimer(0, TIME_OUT); //set timer.

  while (i != bufferNum[0]) //iterate window
  {
    // Resend packet to network
    packet = window_buffer[0][i];
    packet.acknum = AckNum[0];
    packet.checksum = make_CheckSum(packet);
    tolayer3(0, packet); //send packet
    printf("  data of sendpacket:%.*s\n", 19, packet.payload);
    i = (i + 1) % WIN_SIZE;
  }
  printf("\n");
}

//Name: B_timerinterrupt()
//description: Same is A_timerinterrupt...
void B_timerinterrupt()
{
  struct pkt packet;
  int i = base[1] % WIN_SIZE;
  printf("B-timerinterrupt: resend packets...\n");
  // Iterate through window
  starttimer(1, TIME_OUT);
  while (i != bufferNum[1])
  {
    // Resend packet to network
    packet = window_buffer[1][i];
    packet.acknum = AckNum[1];
    packet.checksum = make_CheckSum(packet);
    tolayer3(1, packet);
    printf("  sent data:%.*s\n", 19, packet.payload);
    // Iterate
    i = (i + 1) % WIN_SIZE;
  }
  printf("\n");
  // Set timer
}
/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() //this program call A to 0
{
  nextSeqnum[0] = 0;   //for sending
  base[0] = 0;         //for base
  expectseqNum[0] = 0; //for sender
  bufferNum[0] = 0;    //for buffer -> need resending process
  AckNum[0] = -1;      //to save Ackinfo ->It is contained in the data to be sent and sent. -> for piggyback
}
/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() //this program call B to 1
{
  nextSeqnum[1] = 0;
  base[1] = 0;
  expectseqNum[1] = 0;
  bufferNum[1] = 0;
  AckNum[1] = -1;
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify

******************************************************************/

struct event
{
  float evtime;       /* event time */
  int evtype;         /* event type code */
  int eventity;       /* entity where event occurs */
  struct pkt *pktptr; /* ptr to packet (if any) assoc w/ this event */
  struct event *prev;
  struct event *next;
};
struct event *evlist = NULL; /* the event list */

/* possible events: */
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2

#define OFF 0
#define ON 1
#define A 0
#define B 1

int TRACE = 1;   /* for my debugging */
int nsim = 0;    /* number of messages from 5 to 4 so far */
int nsimmax = 0; /* number of msgs to generate, then stop */
float time = (float)0.000;
float lossprob;    /* probability that a packet is dropped  */
float corruptprob; /* probability that one bit is packet is flipped */
float lambda;      /* arrival rate of messages from layer 5 */
int ntolayer3;     /* number sent into layer 3 */
int nlost;         /* number lost in media */
int ncorrupt;      /* number corrupted by media*/

void main()
{
  struct event *eventptr;
  struct msg msg2give;
  struct pkt pkt2give;

  int i, j;
  /* char c; // Unreferenced local variable removed */

  init();
  A_init();
  B_init();

  while (1)
  {
    eventptr = evlist; /* get next event to simulate */
    if (eventptr == NULL)
      goto terminate;
    evlist = evlist->next; /* remove this event from event list */
    if (evlist != NULL)
      evlist->prev = NULL;
    if (TRACE >= 2)
    {
      printf("\nEVENT time: %f,", eventptr->evtime);
      printf("  type: %d", eventptr->evtype);
      if (eventptr->evtype == 0)
        printf(", timerinterrupt  ");
      else if (eventptr->evtype == 1)
        printf(", fromlayer5 ");
      else
        printf(", fromlayer3 ");
      printf(" entity: %d\n", eventptr->eventity);
    }
    time = eventptr->evtime; /* update time to next event time */
    if (nsim == nsimmax)
      break; /* all done with simulation */
    if (eventptr->evtype == FROM_LAYER5)
    {
      generate_next_arrival(); /* set up future arrival */
      /* fill in msg to give with string of same letter */
      j = nsim % 26;
      for (i = 0; i < 20; i++)
        msg2give.data[i] = 97 + j;
      if (TRACE > 2)
      {
        printf("          MAINLOOP: data given to student: ");
        for (i = 0; i < 20; i++)
          printf("%c", msg2give.data[i]);
        printf("\n");
      }
      nsim++;
      if (eventptr->eventity == A)
        A_output(msg2give);
      else
        B_output(msg2give);
    }
    else if (eventptr->evtype == FROM_LAYER3)
    {
      pkt2give.seqnum = eventptr->pktptr->seqnum;
      pkt2give.acknum = eventptr->pktptr->acknum;
      pkt2give.checksum = eventptr->pktptr->checksum;
      for (i = 0; i < 20; i++)
        pkt2give.payload[i] = eventptr->pktptr->payload[i];
      if (eventptr->eventity == A) /* deliver packet by calling */
        A_input(pkt2give);         /* appropriate entity */
      else
        B_input(pkt2give);
      free(eventptr->pktptr); /* free the memory for packet */
    }
    else if (eventptr->evtype == TIMER_INTERRUPT)
    {
      if (eventptr->eventity == A)
        A_timerinterrupt();
      else
        B_timerinterrupt();
    }
    else
    {
      printf("INTERNAL PANIC: unknown event type \n");
    }
    free(eventptr);
  }

terminate:
  printf(" Simulator terminated at time %f\n after sending %d msgsfrom layer5\n", time, nsim);
}

void init() /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();

  printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
  printf("Enter the number of messages to simulate: ");
  scanf("%d", &nsimmax);
  printf("Enter  packet loss probability [enter 0.0 for no loss]:");
  scanf("%f", &lossprob);
  printf("Enter packet corruption probability [0.0 for no corruption]:");
  scanf("%f", &corruptprob);
  printf("Enter average time between messages from sender's layer5 [ >0.0]:");
  scanf("%f", &lambda);
  printf("Enter TRACE:");
  scanf("%d", &TRACE);

  srand(9999);      /* init random number generator */
  sum = (float)0.0; /* test random number generator for students */
  for (i = 0; i < 1000; i++)
    sum = sum + jimsrand(); /* jimsrand() should be uniform in [0,1] */
  avg = sum / (float)1000.0;
  if (avg < 0.25 || avg > 0.75)
  {
    printf("It is likely that random number generation on your machine\n");
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
  }

  ntolayer3 = 0;
  nlost = 0;
  ncorrupt = 0;

  time = (float)0.0;       /* initialize time to 0.0 */
  generate_next_arrival(); /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
  double mmm = RAND_MAX;     /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */
  x = (float)(rand() / mmm); /* x should be uniform in [0,1] */
  return (x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
  double x, log(), ceil();
  struct event *evptr;
  /* char *malloc(); // malloc redefinition removed */
  /* float ttime; // Unreferenced local variable removed */
  /* int tempint; // Unreferenced local variable removed */

  if (TRACE > 2)
    printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

  x = lambda * jimsrand() * 2; /* x is uniform on [0,2*lambda] */
                               /* having mean of lambda        */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtime = (float)(time + x);
  evptr->evtype = FROM_LAYER5;
  if (BIDIRECTIONAL && (jimsrand() > 0.5))
    evptr->eventity = B;
  else
    evptr->eventity = A;
  insertevent(evptr);
}

void insertevent(struct event *p)
{
  struct event *q, *qold;

  if (TRACE > 2)
  {
    printf("            INSERTEVENT: time is %lf\n", time);
    printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
  }
  q = evlist; /* q points to header of list in which p struct inserted */
  if (q == NULL)
  { /* list is empty */
    evlist = p;
    p->next = NULL;
    p->prev = NULL;
  }
  else
  {
    for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
      qold = q;
    if (q == NULL)
    { /* end of list */
      qold->next = p;
      p->prev = qold;
      p->next = NULL;
    }
    else if (q == evlist)
    { /* front of list */
      p->next = evlist;
      p->prev = NULL;
      p->next->prev = p;
      evlist = p;
    }
    else
    { /* middle of list */
      p->next = q;
      p->prev = q->prev;
      q->prev->next = p;
      q->prev = p;
    }
  }
}

void printevlist()
{
  struct event *q;
  /* int i; // Unreferenced local variable removed */
  printf("--------------\nEvent List Follows:\n");
  for (q = evlist; q != NULL; q = q->next)
  {
    printf("Event time: %f, type: %d entity:%d\n", q->evtime, q->evtype, q->eventity);
  }
  printf("--------------\n");
}

/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB) /* A or B is trying to stop timer */
{
  struct event *q; /* ,*qold; // Unreferenced local variable removed */

  if (TRACE > 2)
    printf("          STOP TIMER: stopping timer at %f\n", time);
  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
  for (q = evlist; q != NULL; q = q->next)
    if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
    {
      /* remove this event */
      if (q->next == NULL && q->prev == NULL)
        evlist = NULL;          /* remove first and only event on list */
      else if (q->next == NULL) /* end of list - there is one in front */
        q->prev->next = NULL;
      else if (q == evlist)
      { /* front of list - there must be event after */
        q->next->prev = NULL;
        evlist = q->next;
      }
      else
      { /* middle of list */
        q->next->prev = q->prev;
        q->prev->next = q->next;
      }
      free(q);
      return;
    }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}

void starttimer(int AorB, float increment) /* A or B is trying to stop timer */
{

  struct event *q;
  struct event *evptr;
  /* char *malloc(); // malloc redefinition removed */

  if (TRACE > 2)
    printf("          START TIMER: starting timer at %f\n", time);
  /* be nice: check to see if timer is already started, if so, then  warn */
  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
  for (q = evlist; q != NULL; q = q->next)
    if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB))
    {
      printf("Warning: attempt to start a timer that is already started\n");
      return;
    }

  /* create future event for when timer goes off */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtime = (float)(time + increment);
  evptr->evtype = TIMER_INTERRUPT;
  evptr->eventity = AorB;
  insertevent(evptr);
}

/************************** TOLAYER3 ***************/
void tolayer3(int AorB, struct pkt packet) /* A or B is trying to stop timer */
{
  struct pkt *mypktptr;
  struct event *evptr, *q;
  /* char *malloc(); // malloc redefinition removed */
  float lastime, x, jimsrand();
  int i;

  ntolayer3++;

  /* simulate losses: */
  if (jimsrand() < lossprob)
  {
    nlost++;
    if (TRACE > 0)
      printf("          TOLAYER3: packet being lost\n");
    return;
  }

  /* make a copy of the packet student just gave me since he/she may decide */
  /* to do something with the packet after we return back to him/her */
  mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
  mypktptr->seqnum = packet.seqnum;
  mypktptr->acknum = packet.acknum;
  mypktptr->checksum = packet.checksum;
  for (i = 0; i < 20; i++)
    mypktptr->payload[i] = packet.payload[i];
  if (TRACE > 2)
  {
    printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
           mypktptr->acknum, mypktptr->checksum);
    for (i = 0; i < 20; i++)
      printf("%c", mypktptr->payload[i]);
    printf("\n");
  }

  /* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype = FROM_LAYER3;      /* packet will pop out from layer3 */
  evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;         /* save ptr to my copy of packet */
                                    /* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
  lastime = time;
  /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
  for (q = evlist; q != NULL; q = q->next)
    if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
      lastime = q->evtime;
  evptr->evtime = lastime + 1 + 9 * jimsrand();

  /* simulate corruption: */
  if (jimsrand() < corruptprob)
  {
    ncorrupt++;
    if ((x = jimsrand()) < .75)
      mypktptr->payload[0] = 'Z'; /* corrupt payload */
    else if (x < .875)
      mypktptr->seqnum = 999999;
    else
      mypktptr->acknum = 999999;
    if (TRACE > 0)
      printf("          TOLAYER3: packet being corrupted\n");
  }

  if (TRACE > 2)
    printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
}

void tolayer5(int AorB, char datasent[20])
{
  int i;
  if (TRACE > 2)
  {
    printf("          TOLAYER5: data received: ");
    for (i = 0; i < 20; i++)
      printf("%c", datasent[i]);
    printf("\n");
  }
}
