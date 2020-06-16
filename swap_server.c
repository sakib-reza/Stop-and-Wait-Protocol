/*
*	swap_server.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define	MAXLINE	128	// maximum characters to receive and send at once
#define MAXFRAME 256

extern int swap_accept(unsigned short port);
extern int swap_disconnect(int sd);
extern int sdp_send(int sd, char *buf, int length);
extern int sdp_receive(int sd, char *buf);
extern int sdp_receive_with_timer(int sd, char *buf, unsigned int expiration);
extern unsigned short checksum(unsigned char buf[], int length);

int session_id = 0;
int R = 0;	// frame number to receive

int swap_wait(unsigned short port)
{
	/*
	*	if the session is already open, then return error
	*/

	if (session_id != 0)
		return -1;

	/*
	*	accept a connection
	*/

	session_id = swap_accept(port);	// in sdp.o

	/*
	*	return a ssession id
	*/

	return session_id;
}

int swap_read(int sd, char *buf)
{
	int	n;
	char frame[MAXFRAME], message[MAXFRAME], sent_check[6], ACK[] = "0", server_check[6];
	unsigned short server_c;

	if (session_id == 0 || sd != session_id)
		return -1;

	/*
	*	receive a frame without a timer
	*/
	bzero(&frame, sizeof(frame));	// Removes some weird errors sometimes
	n = sdp_receive(sd, frame);		

	// Takes the full frame received and splits it into 2
	// The checksum will always be 5 characters long cause its 2 bytes long
	// So the last 5 bits are put into sent_check and everything else goes into message
	snprintf(message, strlen(frame)-4, "%s", frame);		
	char *ptr = frame;
	snprintf(sent_check, 6, "%s", ptr + strlen(frame) - 5);

	server_c = checksum(message, strlen(message));
	snprintf(server_check, 6,"%d", server_c);

	/*
	*	several cases
	*/
	if (n == -2)
	{
		printf("Server has been disconnected from the client. Message could not be recived.\n");
		return 0;
	}
	else if (n == -1)
		printf("There has been an error in transmission. Message could not be recived.\n");
	else
	{	
		printf("\nA message has been received from the client.\n");
		printf("The frame received from the server is: %s\n", frame);
		printf("The checksum received is: %s\n", sent_check);
		printf("The checksum calculated is: %s\n", server_check);
		if (strcmp(sent_check, server_check) == 0)		// Compares the two checksum values
		{												// If they are the same, ACK = '1' and is sent back
			ACK[0] = '1';								// Otherwise ACK = '0' and is sent back
			if(sdp_send(sd, ACK, 1) == -1)
				printf("There was an error in sending the ACK.\n");
			else
			{
				printf("ACK has been sent to client.\n");
			}
		} else
		{
			ACK[0] = '0';
			printf("The checksum values are different; attempting to send negative ACK to client.\n");
			if(sdp_send(sd, ACK, 1) == -1)
				printf("There was an error in sending the negative ACK.\n");
			else
			{
				printf("Negative ACK has been sent to client.\n");
			}
		}
		
	}

	/*
	*	copy the data field in the frame into buf, and return the length
	*/
	printf("A message has been received %d time(s): ", R = R+1); 	// Using the global variable count the number of times 
	strcpy(buf, message);											// the message was sent
	return strlen(buf);
}

void swap_close(int sd)
{
	if (session_id == 0 || sd != session_id)
		return;

	else
		session_id = 0;

	swap_disconnect(sd);	// in sdp.o
}
