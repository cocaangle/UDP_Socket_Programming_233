/* 
Name: Mi Su
Student Id: W1611208

Description: a program that enables client sending packets to server for requesting access to celluar
network. The server will verify the client's identification by checking the txt database information.
If the server can't find client's subscriber num in the database, or if subscriber num found but tech
type mismatch, the server will respond by sending Not Exit message; if subscriber not paid, the server
will respond by sending Not Paid message; otherwise, the server will send a permit to access message.

Assumption: 1. the client only resend packet when time out happens(receiving no response within 3 seconds),
the client can resend up to 3 times for time out situation, if still no response from server, an error
message will be generated by the client. 2. Assume server response packet tech type is the same as client
sending packet, if the subscriber number not found, the response packet tech type will be set as 0x00
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
#define PACKET_NUM 5 //number of packet to send is 5
#define SP_ID 0xFFFF //start of packet ID
#define EP_ID 0xFFFF //end of packet ID
#define CLIENT_ID 0x48 //client ID (the max is 0xFF)
#define MAX_LENGTH 0xFF // max length for payload

#define ACC_PER 0xFFF8 //define different types for packets
#define NOT_PAID 0xFFF9
#define NOT_EXIST 0xFFFA
#define ACC_OK 0xFFFB

#define G2 0x02 //define different types of technologies
#define G3 0x03
#define G4 0x04
#define G5 0x05

//define the structure for data packet
typedef struct dataPacket {
    short start_id;
    char client_id;
    short type;
    char segment_num;
    char length;
    char tech;
    unsigned long sub_num; //subscriber phone number
    short end_id;
}dataPacket;

int main(int argc, char *argv[]) {

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

    //initialize 5 data packets to send
    dataPacket packets[PACKET_NUM];

    //populate subscribe number and technology with random test data
    packets[0].sub_num = 4085546805; //not exist because of sub_num not found
    packets[0].tech = G2;
    packets[1].sub_num = 4086668821; //not paid
    packets[1].tech = G3;
    packets[2].sub_num = 4086808821; //acc_ok
    packets[2].tech = G4;
    packets[3].sub_num = 4087320501; //not exist because of tech not match
    packets[3].tech = G5;
    packets[4].sub_num = 4085876566; //acc_ok
    packets[4].tech = G3;

    for (int i = 0; i < PACKET_NUM; i++) {
        packets[i].client_id = CLIENT_ID;
        packets[i].start_id = SP_ID;
        packets[i].end_id = EP_ID;
        packets[i].type = ACC_PER;
        packets[i].segment_num = i;
        packets[i].length = sizeof(packets[i].sub_num) + sizeof(packets[i].tech);
    }

    printf("\n*************************************************************\n\n");
    
    //send packets to server and wait for server response
    dataPacket res;
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
                recvfrom(sockfd, &res, sizeof(dataPacket), 0, (struct sockaddr *)&server_addr, &addr_size);
                if (res.type == (short)ACC_OK) {//permit message
                    printf("Server: Subscriber %lu permitted to access the network\n\n", res.sub_num);
                    break; //break inner while loop to continue send next packet
                } 
                else { 
                    if (res.type == (short)NOT_EXIST && res.tech == 0x00) {//subscriber number not found
                        printf("Server: Subscriber %lu not found on database\n\n", res.sub_num);
                        break;
                    } else if (res.type == (short)NOT_EXIST) {//if tech != 0x00, tech mismatch
                        printf("Server: Subscriber %lu technology mismatch with database\n\n", res.sub_num);
                        break;
                    } else if (res.type == (short)NOT_PAID) {//not paid mesage
                        printf("Server: Subscriber %lu has not paid\n\n", res.sub_num);
                        break;
                    } else {//will never reach here, just for debugging
                        printf("Client: Unknown response from server\n\n");
                        exit(1);
                    }
                }
            }
        }
        if (counter > 3) {
            printf("Client: Server does not respond after resending access request for subscriber %lu 3 times!\n", dataPkt.sub_num); //generate error message after resending 3 times
            printf("Terminate program now!\n");
            break; //break the while loop and close the socket to terminate the program
        } else {
            i++;
        }       
    }

    close(sockfd);
    return 0;
}