/* 
Name: Mi Su
Student Id: W1611208

Description: a program that enables client sending 5 packets to server at one time based on UDP protocl. In this program,
the client can choose to send all 5 correct packets or choose to send 1 correct and 4 error packets. When receiving correct
packet, server will send a ACK packet, while receiving error packet, the server will send reject pakcet with corresponding
error information. Specially, when the packet sent is out of sequence, the server will not increment expected segment number
until the client send a correct packet with revised segment number.

Assumption: the client only resend packet when time out happens(receiving no response within 3 seconds); the client won't 
resend packet when receiving reject packet except for the out of sequence reject packet; the client can resend up to 3 times
for time out situation (the first time sending not count as resending); sending the revised packet because of out of sequence 
error won't be counted as resending in time out situation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <netdb.h>
#include <stdbool.h>

#define SP_ID 0xFFFF //start of packet ID
#define EP_ID 0xFFFF //end of packet ID
#define CLIENT_ID 0x36 //client ID
#define MAX_LENGTH 0xFF // max length for payload
#define DATA 0xFFF1 //packet type: data
#define ACK 0xFFF2  //packet type: ACK
#define REJECT 0xFFF3 //packet type: REJECT
#define REJ_SEQUENCE 0xFFF4 // packet out of sequence
#define REJ_LENGTH 0xFFF5 // packet length mismatch
#define REJ_END 0xFFF6 // end of packet missing
#define REJ_DUPLICATE 0xFFF7 // duplicate packet

//define the data packet type that client send to server
typedef struct dataPacket {
    short start_id;
    char client_id;
    short data;
    char segment_num;
    char length;
    char payload[255];
    short end_id;
}dataPacket;

//define the response packet type(ACK, REJECT) that server send to client
typedef struct resPacket {
    short start_id;
    char client_id;
    short type; // type: ACK or REJECT
    short rej_code;
    char segment_num;
    short end_id;
}response;


int main(int argc, char *argv[])
{
    if (argc < 2) { //check there is port number provided or not
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    int port = atoi(argv[1]);

    int sockfd;
    dataPacket data;
    struct sockaddr_in server_addr, client_addr; //declare server address, client address
    socklen_t addr_size = sizeof(client_addr); //address size

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a socket at server side
    if (sockfd < 0) { 
        perror("ERROR opening socket");
        exit(1);
    }

    //set up server address    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    //bind the address to socket
    if (bind(sockfd, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
    printf("Done with binding\n");
    printf("Listening on port %d for incoming messages...\n\n", port);

    int last_seg = -1; //initialize last seg num as -1, as the first packet seg num is 0
    int exp_seg = 0; //initialize expected segment number
    response res;

    //receive packets one by one from client
    while(true) {
        int recv = recvfrom(sockfd, &data, sizeof(dataPacket), 0, (struct sockaddr *)&client_addr, &addr_size);
        if (recv > 0) {
            printf("Server: Received payload message: %s \n", data.payload);
        } else {
            printf("Server: Error Receive!\n");
        }

        //initialize a response packet to send to user
        res.start_id = SP_ID;
        res.end_id = EP_ID;
        res.type = REJECT; //assume the default type is reject for convenience
        res.client_id = CLIENT_ID;
        res.segment_num = data.segment_num;
        
        //error detection and handling
        int crt_seg = data.segment_num;
        if (crt_seg == exp_seg + 1) { //out of sequence error
           res.rej_code = REJ_SEQUENCE;
           printf("Server: Error! Packet segment_num %d out of sequence! Expect segment num %d\n\n", crt_seg, exp_seg);
           sendto(sockfd, &res, sizeof(response), 0, (struct sockaddr *)&client_addr, addr_size);
           continue; //will not increment expected segment number(exp_seg) until receive correct packet
        } 
        else if (data.length != (char)sizeof(data.payload)) { //length mismatch error
           res.rej_code = REJ_LENGTH;
           printf("Server: Error! Packet %d has a length mismatch!\n\n", crt_seg);
        } 
        else if (data.end_id != (short)EP_ID) { //END ID of packet missing
           res.rej_code = REJ_END;
           printf("Server: Error! Packet %d misses end ID or has an invalid end ID!\n\n", crt_seg);
        } 
        else if (crt_seg == exp_seg - 1) { //duplicate packet error
           res.rej_code = REJ_DUPLICATE;
           printf("Server: Error! Packet segment_num %d duplicated with previous packet! Expect segment num %d\n\n", crt_seg, exp_seg);
        } 
        else { //No error found, send ACK packet
           res.type = ACK; 
           printf("Server: ACK! Packet %d Acknowledged!\n\n", crt_seg);
        }

        //send response packet to client
        sendto(sockfd, &res, sizeof(response), 0, (struct sockaddr *)&client_addr, addr_size);
        exp_seg++; //update expected segment num 
    }       
    
    close(sockfd);
    return 0; 
}