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
static int cc_count = 20;
struct pkt resend_buffer[2000];
//static int resend_buffer_pos;

//struct pkt CopyPacket;
//int seq_num;
int base;
int window_size;
//int sent_seqnum;

//int last_recvd;
int expected_pkt;
void Send_Packet_A_Side(int seq ,struct msg message)
{
	printf ("<Send A:%s>\r\n",message.data);
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

	//sent_seqnum = seq;
	if (base == seq)
	{
		starttimer(A_SIDE,cc_count);
	}
	
	resend_buffer[seq]  = packet;
	//resend_buffer_pos++;
	return;
}

void Send_Packet_B_Side(int seq)
{
	printf ("<Send B:%d>\r\n",seq);
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
	printf ("<A Side>:%s\r\n",message.data);
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
		printf ("Checksum mistmatch \r\n");
		return;
	}
	
	base = packet.acknum + 1;

	while ((seq_num<base+window_size) && (start_buffer_pos < end_buffer_pos))
        {
                Send_Packet_A_Side(seq_num,message_buffer[start_buffer_pos]);
                start_buffer_pos++;
                seq_num++;
        }


	if (base == seq_num)
	{
		stoptimer(A_SIDE);
	}
	else
	{
		starttimer(A_SIDE,cc_count);
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	starttimer(A_SIDE,cc_count);
	printf ("<Timer interrupt trigger>\r\n");
	for (int i= base; i <seq_num; i++)
	{
		tolayer3(A_SIDE, resend_buffer[i]);
	}
	return;
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	memset(&message_buffer,0,sizeof(message_buffer));
	start_buffer_pos = 0;
	end_buffer_pos = 0;
	base = 0;
	seq_num = 0;
	window_size =getwinsize();
//	cc_count = 4 *window_size;
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

	if(packet.seqnum == expected_pkt)
	{
		printf ("<At B>%s\r\n",packet.payload);
		Send_Packet_B_Side(expected_pkt);
		tolayer5(B_SIDE,packet.payload);
               // last_recvd = packet.seqnum;
                expected_pkt++;
	}
	else if(expected_pkt!=0)
	{
		Send_Packet_B_Side(expected_pkt-1);
	}
	return;
}		
	
/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
//	last_recvd = 0;
	expected_pkt = 0;
	return;
}
