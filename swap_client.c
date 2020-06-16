/*
*	swap_client.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define	MAXLINE 128	// maximum characters to receive and send at once
#define	MAXFRAME 256

extern int swap_connect(unsigned int addr, unsigned short port);
extern int swap_disconnect(int sd);
extern int sdp_send(int sd, char *buf, int length);
extern int sdp_receive(int sd, char *buf);
extern int sdp_receive_with_timer(int sd, char *buf, unsigned int expiration);
extern unsigned short checksum(unsigned char buf[], int length);

int session_id = 0;
int S = 0;	// frame number sent

int swap_open(unsigned int addr, unsigned short port)
{
	int	sockfd;		// sockect descriptor
	struct	sockaddr_in	servaddr;	// server address
	char	buf[MAXLINE];
	int	len, n;

	/*
	*	if the session is already open, then return error
	*/

	if (session_id != 0)
		return -1;

	/*
	*	connect to a server
	*/

	session_id = swap_connect(addr, port);	// in sdp.o

	/*
	*	return the seesion id
	*/

	return session_id;
}

int swap_write(int sd, char *buf, int length)
{
	/*	Took me a long time to figure out what was going on, and even longer
		to understand why frames were being dropped, timer was expiring, all
		that, but this was a very fun lab afterwords, definitely got me used 
		using C again.
	*/

	int n, m;
	char frame[MAXFRAME], ACK[] = "0", check[6];
	unsigned short c;


	if (session_id == 0 || sd != session_id)
		return -1;

	/*
	*	send a DATA frame
	*/
	bzero(&frame, sizeof(frame));	// Fixes some weird buggy stuff
	strcpy(frame, buf);				// Copies the string into frame

	c = checksum(buf, length);		// Calculates the checksum and stores it as a string in check
	snprintf(check, 6,"%d", c);

	strcat(frame, check);			// Creates the full frame from the message in buf and the checksum value

	n = sdp_send(sd, frame, length+5);	// Gotta add 5 to length to compensate for the checksum

	if (n == -1)
		printf("There was an error sending to the client.\n");
	else
	{
		printf("Message sent to the server.\n");
		printf("The checksum value being sent is: '%s'\n", check);
		printf("The frame being sent is: '%s'\n", frame);
	}

	/*
	*	read a frame with a timer
	*/
	m = sdp_receive_with_timer(sd, ACK, 5000);		// 5000 ms wait to receive an ACK from the server, tbh this is too long

	/*
	*	several different cases including disconnection
	*/
	if (m == -3)
		printf("The timer has expired. ACK could not be received.\n");
	else if (m == -2)
		printf("The session has disconnected.\n");
	else if (m == -1)
		printf("There was an error in transmission.\n");
	else
	{
		if (ACK[0] == '1')		// If the ACK has been sent back all is good in the world
			printf("Message has been sent successfully.\n");
		else
			printf("Negative ACK received; The checksum values did not match.\n");
		
	}

	/*
	*	return the length sent
	*/
	printf("The message has been sent %d time(s).\n\n", S = S+1);	// Using a global variable to keep track
	return n;

}

void swap_close(int sd)
{
	if (session_id == 0 || sd != session_id)
		return;

	else
		session_id = 0;

	swap_disconnect(sd);	// in sdp.o
}
