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
#define PACKET_NUM 5 //packet number to send is 5
#define BLEN 1024 //buffer size

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
    short type; //type: ACK or REJECT
    short rej_code;
    char segment_num;
    short end_id;
}response;


int main(int argc, char *argv[])
{
    if (argc < 3) { //check there is machine name(localhost) and port number provided or not
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    int port = atoi(argv[2]);

    int sockfd;
    dataPacket data;
    struct sockaddr_in server_addr; //declare server address
    socklen_t addr_size = sizeof(server_addr); //address size
    

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); //create a socket at client side
    if (sockfd < 0) { 
        perror("ERROR opening socket");
        exit(1);
    }

    //set up server address    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    //initialize pollfd for the timer
    struct pollfd pollsock;
    pollsock.fd = sockfd;
    pollsock.events = POLLIN; //POLLIN events: check whether the fd is ready for reading/receiving

    //initialize correct packets
    char buffer[BLEN];
    dataPacket packets[PACKET_NUM];
    for (int i = 0; i < PACKET_NUM; i++) {
        packets[i].client_id = CLIENT_ID;
        packets[i].start_id = SP_ID;
        packets[i].end_id = EP_ID;
        packets[i].data = DATA;
        packets[i].segment_num = i;
        sprintf(buffer, "This is a correct packet #%d", i); //add string to buffer
        strncpy(packets[i].payload, buffer, 255); //copy string from buffer to payload, max size: 255
        packets[i].length = sizeof(packets[i].payload);
    }
    
    //set up error packets 
    printf("\n*************************************************************\n");
    int select_case; //val represents different packet error, 0: 5 correct packets, 1: 1 correct and 4 error packets
    printf("Enter case number: (0: select 5 correct packets; 1: select 1 correct and 4 error packets) \n");
    scanf("%d", &select_case);
    if (select_case == 1) {

        //1 correct and 4 error packets were selected: packet 0 is correct
        packets[1].length = 0; //error packet 1: length mismatch
        sprintf(buffer, "This is length mismatch packet"); //add string to buffer
        strncpy(packets[1].payload, buffer, 255); //update wrong packet's payload

        packets[2].segment_num = 3; //error packet 2: out of sequence
        sprintf(buffer, "This is an out of sequence error packet");
        strncpy(packets[2].payload, buffer, 255);

        packets[3].end_id = 0; //error packet 3: invalid end id
        sprintf(buffer, "This is an invalid ending packet");
        strncpy(packets[3].payload, buffer, 255);

        packets[4].segment_num = packets[3].segment_num; //error packet 4: duplicate packet
        sprintf(buffer, "This is duplicated packet");
        strncpy(packets[4].payload, buffer, 255);
        printf("Client: 1 correct and 4 error packets are selected");
    } 
    else {

        //case 0 is choosing 5 correct packets
        printf("Client: 5 correct packets are selected.\n");
    } 
    
    printf("\n*************************************************************\n\n");
    
    //send packets to server and wait for server response
    response res;
    dataPacket dataPkt;
    int i = 0;
    int counter = 1;

    while (i < PACKET_NUM) {
        dataPkt = packets[i];
        printf("Client: Send packet %d to server. First sending.\n", i);
        if (sendto(sockfd, &dataPkt, sizeof(dataPacket), 0, (struct sockaddr *)&server_addr, addr_size) < 0) {
            perror("Client: Sending Error!\n");
            exit(1);
        }
        
        while (counter <= 3) { //resend at most 3 times(not include the 1st sending and revised packet sending)
            int poll_ans = poll(&pollsock, 1, 3000); //wait for server response up to 3 seconds
            if (poll_ans == 0) { //time out, retransmit
                printf("Client: Time out! Retransmit %d out of 3\n", counter);
                counter++;
                sendto(sockfd, &dataPkt, sizeof(dataPacket), 0, (struct sockaddr *)&server_addr, addr_size);
            } 
            else if (poll_ans == -1) { //poll error occurs
                perror("Client: Poll Error\n");
                exit(1);
            } 
            else { //normal receiving
                recvfrom(sockfd, &res, sizeof(response), 0, (struct sockaddr *)&server_addr, &addr_size);
                if (res.type == (short)ACK) {
                    printf("Server: ACK! Packet %d Acknowledged!\n\n", dataPkt.segment_num);
                    break; //break inner while loop to continue send next packet
                } 
                else { //reject type (assume response packet only has ACK and reject type)
                    if (res.rej_code == (short)REJ_SEQUENCE) {
                        printf("Server: Error! Packet segment_num %d out of sequence! Expect segment num %d\n\n", res.segment_num, i);
                        dataPkt.segment_num = i; //revise the segment number 
                        sprintf(buffer, "This is a correct packet with out of order segment number revised");
                        strncpy(dataPkt.payload, buffer, 255); //update packet's payload message to correct one 

                        //send the revised packet
                        printf("Client: Send packet with revised segment number %d to server.\n", i);
                        sendto(sockfd, &dataPkt, sizeof(dataPacket), 0, (struct sockaddr *)&server_addr, addr_size); 
                        continue;
                    } 
                    else if (res.rej_code == (short)REJ_DUPLICATE) {
                        printf("Server: Error! Packet segment_num %d duplicated with previous packet! Expect segment num %d\n\n", res.segment_num, i);
                        break;
                    } 
                    else if (res.rej_code == (short)REJ_END) {
                        printf("Server: Error! Packet %d misses end ID or has an invalid end ID!\n\n", res.segment_num);
                        break;
                    } 
                    else if (res.rej_code == (short)REJ_LENGTH) {
                        printf("Server: Error! Packet %d has a length mismatch!\n\n", res.segment_num);
                        break;
                    }
                }
            }
        }
        if (counter > 3) {
            printf("Client: Server does not respond!\n"); //generate error message after resending 3 times
            printf("Terminate program now!\n");
            break; //break the while loop and close the socket to terminate the program
        } else {
            i++;
        }       
    }

    close(sockfd);
    return 0;
}