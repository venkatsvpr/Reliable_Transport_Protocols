#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/
#define A_SIDE 0
#define B_SIDE 1

#define TRUE 1
#define FALSE 0
int seq_num;

struct msg message_buffer[3000];
static int start_buffer_pos;
static int end_buffer_pos;

struct pkt resend_buffer[3000];
static float resend_timer[3000];
static int resend_ack[3000];

struct pkt rec_buffer[3000];
static int rec_buffer_valid[3000];

static int cc_count = 40;

int base;
int window_size;
int expected_pkt;

void Send_Packet_A_Side(int seq ,struct msg message)
{
//	printf ("<Send A:%s>\r\n",message.data);
	struct pkt packet;
	memset(&packet,0,sizeof(packet));

	packet.checksum = seq +packet.acknum;
	packet.seqnum = seq;
	for (int i=0; i < 20; i++)
	{
		packet.checksum += (int) message.data[i];
	}

	strcpy(packet.payload,message.data);
	tolayer3(A_SIDE,packet);

	if(seq==base)
		starttimer(A_SIDE,cc_count);
	
	resend_buffer[seq]  = packet;
	resend_timer[seq] = get_sim_time()+cc_count;

	return;
}

void Send_Packet_B_Side(int seq)
{
//	printf ("<Send B:%d>\r\n",seq);
	struct pkt packet;
	memset(&packet,0,sizeof(packet));

	packet.acknum = seq;
	packet.checksum = packet.seqnum + packet.acknum;
#if 1
	for (int i=0; i < 20; i++)
	{
		packet.checksum += (int) packet.payload[i];
	}
#endif
	
	tolayer3(B_SIDE,packet);
	return;
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{

	if (seq_num  >= base+window_size)
	{
		/* Buffered send */
		message_buffer[end_buffer_pos] = message;
		end_buffer_pos++;
		return;
	}
	
	Send_Packet_A_Side(seq_num, message);	
	seq_num++;
	return;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	int t_checksum = packet.seqnum ;
#if 1
	for (int i=0; i <20; i++)
	{
		t_checksum += packet.payload[i];
	}
#endif
	t_checksum += packet.acknum;

	if (t_checksum != packet.checksum)
	{
//		printf ("Checksum mistmatch \r\n");
		return;
	}
	
	if((packet.acknum<base) || (packet.acknum>=base+window_size))
		return;

	resend_timer[packet.acknum]=0;
	resend_ack[packet.acknum]=1;	
	
	if(base==packet.acknum)
	{
		for(int i=base;i<seq_num;i++)
		{	if(resend_ack[i]!=1)
				break;
			base++;
		}
	}
	
	while((seq_num<base+window_size) && (start_buffer_pos < end_buffer_pos))
        {
                Send_Packet_A_Side(seq_num,message_buffer[start_buffer_pos]);
		start_buffer_pos++;		
                seq_num++;
        }

	if (base == seq_num)
	{
		stoptimer(A_SIDE);
	}

	return;
}

/* called when A's timer goes off */
float find_min_time()
{
	float  min_time  = get_sim_time()+cc_count;
	for (int i= base; i <seq_num; i++)
	{
		if((resend_ack[i]==0) && (resend_timer[i]< min_time))
		{
			min_time = resend_timer[i];
		}
	}

	return (min_time);
}		
void A_timerinterrupt()
{
//	printf ("<Timer interrupt trigger at %f>\r\n",get_sim_time());
	for (int i= base; i <seq_num; i++)
	{
		if ((resend_timer[i] == get_sim_time()) && (resend_ack[i]==0))
		{
//			printf ("<Send A:%s>\r\n", resend_buffer[i].payload);
			tolayer3(A_SIDE, resend_buffer[i]);
			resend_timer[i] = get_sim_time()+cc_count;		
			break;
		}
	}

	if ((find_min_time()) > 0)
	{
		starttimer(A_SIDE, find_min_time()-get_sim_time());
	}
	return;
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	memset(&message_buffer,0,sizeof(message_buffer));
	memset(&resend_buffer,0,sizeof(resend_buffer));
	memset(&resend_timer,0,sizeof(resend_timer));
	memset(&resend_ack,0,sizeof(resend_ack));

	start_buffer_pos = 0;
	end_buffer_pos = 0;
	base = 0;
	seq_num = 0;
	window_size =getwinsize();
	return;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
	{

//	printf ("<At B>%s:pktsqm:%d expected%d\r\n",packet.payload,packet.seqnum,expected_pkt);
	struct pkt send_pkt;
	memset(&send_pkt,0,sizeof(send_pkt));
	
	int checksum = packet.seqnum + packet.acknum;
	for (int i=0; i<20; i++)
	{
		checksum  += (int)packet.payload[i];
	}	

	if (checksum  != packet.checksum)
	{
		printf ("Checksum mismstah B \r\n");
		return;
	}

	if((packet.seqnum>=expected_pkt+window_size) || (packet.seqnum<expected_pkt-window_size))
		return;
	
	Send_Packet_B_Side(packet.seqnum);

	if (packet.seqnum != expected_pkt)
	{
		rec_buffer[packet.seqnum] = packet;
		rec_buffer_valid[packet.seqnum]=1;
	}
	else
	{	
		int n=expected_pkt+window_size;
		tolayer5(B_SIDE,packet.payload);
		expected_pkt++;
		
		
		for (int i=expected_pkt; i<n; i++)
		{
			if (rec_buffer_valid[i]==0)
				break;

			tolayer5(B_SIDE,rec_buffer[i].payload);
			expected_pkt++;
		}
	}
//	printf("\n EXP Number : %d",expected_pkt++);
	return;
}		
	
/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	memset(&rec_buffer , 0, sizeof(rec_buffer));
	memset(&rec_buffer_valid , 0, sizeof(rec_buffer_valid));
	window_size =getwinsize();
	expected_pkt = 0;
	return;
}
