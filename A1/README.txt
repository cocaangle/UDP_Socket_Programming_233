Name: Mi Su
Student Id: W1611208

Description: a program that enables client sending 5 packets to server at one time based on UDP protocl. In this program,
the client can choose to send all 5 correct packets or choose to send 1 correct and 4 error packets. When receiving correct
packet, server will send a ACK packet, while receiving error packet, the server will send reject pakcet with corresponding
error information. Specially, when the packet sent is out of sequence, the server will not increment expected segment number
until the client send a correct packet with revised segment number.

Assumption: 
1. The client only resend packet when time out happens(receiving no response within 3 seconds); the client won't 
resend packet when receiving reject packet except for the out of sequence reject packet
2. The client can resend up to 3 times for time out situation (the first time sending not count as resending); 
3. Sending the revised packet because of out of sequence error won't be counted as "resending" as in time out situation.

How to compile and run the program? (instructions for macOS)
1. Open your terminal, go to the directory where the program file existis
2. Compile server.c by running "/usr/bin/clang -o server server.c"
3. Compile client.c by running "/usr/bin/clang -o client client.c"
4. Running compiled server file by inputing command line "./server 50000", here 50000 is the port number, you can also choose
any other valid port number
5. Open another terminal to run compiled client file by inputing command line "./client localhost 50000"
6. Now in the client window, the user will be prompted to input test case number: input 0 for testing sending 5 correct packets;
input 1 for testing sending 1 correct and 4 error packets
7. To test time out sending error, the user could run compiled client file at first

Here are the test data for testing different error handling: 
(when user input 1 for sending 1 correct and 4 error packets)
packet 0 : correct packet
packet 1: length set as 0 for testing length mismatch error handling
packet 2: segment number set as 3 for testing out of sequence error handling
packet 3: end_id set as 0 for testing invalid end Id error handling
packet 4: segment number set as 3 for testing duplicated error handling
