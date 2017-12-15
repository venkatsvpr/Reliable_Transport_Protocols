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

struct msg message_buffer[2000];
static int start_buffer_pos;
static int end_buffer_pos;
static int cc_count=15;
struct pkt CopyPacket;
int seq_num;

int Send_A_Progress;
int Send_B_Progress;
void Send_Packet_A_Side(int seq ,struct msg message)
{
	struct pkt packet;
	memset(&packet,0,sizeof(packet));

	packet.checksum = seq + packet.acknum;
	packet.seqnum = seq;
	for (int i=0; i < 20; i++)
	{
		packet.checksum += (int) message.data[i];
	}
	Send_A_Progress = TRUE;
	strcpy(packet.payload,message.data);
	starttimer(A_SIDE,cc_count);
	printf ("<Sending from A:%s:seq:%d:checsum:%d\r\n",packet.payload,packet.seqnum,packet.checksum);
	tolayer3(A_SIDE,packet);
	CopyPacket = packet;
	return;
}

void Send_Packet_B_Side(int seq)
{
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

	/* Buffered send */
	if (Send_A_Progress == TRUE)
	{
		message_buffer[end_buffer_pos] = message;
		end_buffer_pos++;
		return;
	}
	Send_Packet_A_Side(seq_num, message);	
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
		printf ("Checksum mistmatch \r\n");
		return;
	}

	if (seq_num != packet.acknum)
	{
		printf ("Seq num mismsatch Expected:%d received %d checksum:%d \r\n",seq_num,packet.acknum,packet.checksum);
		return;
	}

	stoptimer(A_SIDE);
	if (seq_num == 0)
	{
		seq_num = 1;
	}
	else if (seq_num == 1)
	{
		seq_num = 0;
	}

	if (start_buffer_pos < end_buffer_pos)
	{
		Send_Packet_A_Side(seq_num,message_buffer[start_buffer_pos]);
		start_buffer_pos++;
	}
	else
	{
		Send_A_Progress = FALSE;
	}
	return;
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf ("<Resending on timeout from A:%s:seq:%d\r\n",CopyPacket.payload, CopyPacket.seqnum);
	tolayer3(A_SIDE, CopyPacket);
	starttimer(A_SIDE, cc_count);
	return;
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	seq_num = 0;
	Send_A_Progress = FALSE;
	memset(&CopyPacket,0,sizeof(CopyPacket));
	memset(&message_buffer,0,sizeof(message_buffer));
	start_buffer_pos = 0;
	end_buffer_pos = 0;
	return;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
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

	printf ("Receiving on B :%s:seq:%d:checksum:%d\r\n",packet.payload, packet.seqnum,packet.checksum);
	Send_Packet_B_Side(packet.seqnum);
	if (packet.seqnum == Send_B_Progress)
	{
		tolayer5 (B_SIDE, packet.payload);
		if (Send_B_Progress == TRUE)
		{
			Send_B_Progress = FALSE;
		}
		else if (Send_B_Progress == FALSE)
		{
			Send_B_Progress = TRUE;
		}
	}
	return;
}		
	
/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	Send_B_Progress = FALSE;
	return;
}
