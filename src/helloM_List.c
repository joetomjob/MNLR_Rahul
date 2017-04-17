/*
 *  helloMessage.c
 *
 *  contains various method calls to receive and send IP, MPLR messages
 *
 *  Created on: Mar 11, 2015
 *  Updated on: Sep 15, 2015
 *      Author: Tejas Padliya - tsp3859@rit.edu
 */

#include <sys/types.h>
#include <netdb.h>
#include <netinet/if_ether.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <ifaddrs.h>
#include <unistd.h>


#include <net/ethernet.h>
#include <signal.h>
#include <ctype.h>

#include "helloList.h"
#include "tierList.h"
#include "genEnvironment.h"

#include "errorControl.h"
#include "printPacketDetails.h"
#include "stringMessages.h"

#include "fwdAlgorithmHelper.h"
#include "baseConversion.h"

#include "endNetworkUtils.h"



extern int ctrlSend(char eth[], char pay[]);
extern int ctrlLabelSend(int,char eth[], char pay[]);


extern int dataSend(char etherPort[], unsigned char ipPacket[], char destTier[],
		char srcTier[], int ipPacketSize);

extern int endNetworkSend(char[], uint8_t *, int);

extern int dataFwd(char etherPort[20], unsigned char MPLRPacket[],
		int MPLRPacketSize);

extern int dataDecapsulation(char etherPort[20],
		unsigned char MPLRDecapsPacket[], int MPLRDecapsSize);

extern int packetForwardAlgorithm(char currentTier[], char desTier[]);
extern int isFWDFieldsSet();

extern int isEnvSet();
extern int isTierSet();
extern int setTierInfo(char tierValue[]);
extern char* getTierInfo();
extern int setControlIF();

extern void printIPPacketDetails(unsigned char ipPacket[], int nIPSize);
extern void printMPLRPacketDetails(unsigned char mplrPacket[], int nSize);

void checkEntriesToAdvertise();
void checkForLinkFailures(struct addr_tuple *, int);
bool isInterfaceActive(struct in_addr, int);
void getMyTierAddresses();

char *interfaceList[10];
int interfaceListSize;

char specificLine[100];
char payLoadTier[100];

char tierDest[12] = "0.0";

// Structure to keep track of failed EndIPs
struct addr_tuple *failedEndIPs_head = NULL;

long long int MPLRCtrlSendCount = 0;
long long int MPLRDataSendCount = 0;
long long int MPLRDataFwdCount = 0;
long long int MPLRCtrlSendRound = 0;
long long int ipReceivedCount = 0;
long long int MPLRCtrlReceivedCount = 0;
long long int MPLRDataReceivedCount = 0;
long long int MPLRMsgVReceivedCount = 0;
long long int MPLROtherReceivedCount = 0;
long long int MPLRDataReceivedCountForMe = 0;
long long int MPLRDecapsulated = 0;
long long int errorCount = 0;

int totalIPError = 0;
int finalARGCValue = -100;
int endNode = -1;
FILE *fptr; //global variable. TO enable logs to be written to files 
int myTierValue = -1;
int tierAddrCount = 0;
int enableLogScreen = 1; //flag to set whether logs need to be shown in screen
int enableLogFiles = 1; //flag to set whether logs need to be shown in file
bool recvdLabel = false;
int MAX_NUM_HOPS = 2;

struct labels{
    char label[10];
    struct labels *next;
};

// The labels allocated to each child node.
struct labels *allocatedLabels = NULL;

// To keep the track of the children to give new names.
int myChildCount = 0;

int freqCount(char str[], char search);

void signal_callbackZ_handler(int signum);
char* macAddrtoString(unsigned char* addr, char* strMAC, size_t size);

int trimAndDelNewLine();
char *strrmc(char *str, char ch);

int packetStats();
void printInputStats();

int setInterfaces();
int freeInterfaces();
int generateChildLabel(char* myEtherPort, int childTier,struct labels** labelList);
void addLabelsList(struct labels* labelList,char label[]);
void joinChildTierParentUIDInterface(char childLabel[],char myTierAddress[],char myEtherPort[]);
void printMyLabels();
boolean updateEndDestinationTierAddrHC(char tempIP[]);
int convertStringToInteger(char* num);


extern int myTotalTierAddress;
/**
 * _get_MACTest(int,char[])
 *
 * method to send and receive MPLR, IP messages in one thread with timeout mechanism
 *
 * @param conditionVal (int) - condition for execution
 * @param specialParam (char[]) - optinal parameters : separated by #
 *
 * @return status (int) - method return value
 */

int _get_MACTest(struct addr_tuple *myAddr, int numTierAddr) {

	if(enableLogScreen)
		printf("\n MNLR started  ... \n");
	if(enableLogFiles)
		fprintf(fptr,"\n MNLR started  ... \n");

	time_t time0 = time(0); //store the start time in time0, so that later we can compare
	time_t time1; //declare a new timer. time1 will be used later to compare with time0 to compare timings 

	int sock, n; //declare 2 integer variables for sock
	int sockIP, nIP; //declare 2 integer variables for socketIp and nIO

	char buffer[2048]; //
	char bufferIP[2048];

	unsigned char *ethhead = NULL;
	unsigned char tempIP[1500];
	struct ether_addr ether;

	char recvOnEtherPort[5];
	char recvOnEtherPortIP[5];
	char MACString[18];

	struct sockaddr_ll src_addr;
	struct sockaddr_ll src_addrIP;

    // Creating the MNLR CONTROL SOCKET HERE
	if ((sock = socket(AF_PACKET, SOCK_RAW, htons(0x8850))) < 0) {
		errorCount++;
		perror("ERROR: MNLR Socket ");
		printf("\n ERROR: MNLR Socket ");

	}

    // Creating the MNLR IP SOCKET HERE
	if ((sockIP = socket(AF_PACKET, SOCK_RAW, htons(0x0800))) < 0) {
		errorCount++;
		perror("ERROR: IP Socket ");
		printf("\n ERROR: IP Socket ");

	}

	char ctrlPayLoadA[200];
	memset(ctrlPayLoadA, '\0', 200);
	uint8_t numOfAddr = (uint8_t) getCountOfTierAddr();
	memcpy(ctrlPayLoadA, &numOfAddr, 1); //memory of numAddr is assigned to ctrlPayLoadA
	int cpLength = 1;
//    uint8_t numHops = 0;
//    memcpy(ctrlPayLoadA+ctrlPayLoadA, &numHops, 1);
//    cpLength++;


	int q = 0;

	//size of the tier address and tier Address is copied to cntrlPayLoadA in the below for loop
	for (; q < getCountOfTierAddr(); q++) {

		char tempAddrA[20];
		memset(tempAddrA, '\0', 20);
		strcpy(tempAddrA, getTierAddr(q)); //copy the qth tier address to tempAddrA
		freeGetTierAddr(); //free the tier address field

		//printf("TEST: (helloM_List) ctrl packet - tempAddrA : %s  \n",
		//		tempAddrA);

		uint8_t localTierSizeA = strlen(tempAddrA);

		//printf("TEST:(helloM_List) ctrl packet - localTierSizeA: %u  \n",
		//		localTierSizeA);

		memcpy(ctrlPayLoadA + cpLength, &localTierSizeA, 1);
		cpLength = cpLength + 1;
		memcpy(ctrlPayLoadA + cpLength, tempAddrA, localTierSizeA);
		cpLength = cpLength + localTierSizeA;

        // Add the hopcount as 1
        uint8_t hopCount = 1;
        memcpy(ctrlPayLoadA + cpLength, &hopCount, 1);
        cpLength = cpLength + 1;
	}

	// Send should initiate before receive
	setInterfaces(); //interfaceList is being set by this function

	int loopCounter1 = 0;

	//this for loop sends the control message as well as keeps track of the no of control messages sent
	for (; loopCounter1 < interfaceListSize; loopCounter1++) {
		ctrlSend(interfaceList[loopCounter1], ctrlPayLoadA); //ctrlSend method is used to send Ethernet frame
		MPLRCtrlSendCount++; //keeps track of the no of control messages sent
	}
	MPLRCtrlSendRound++;

	time0 = time(0);
	freeInterfaces();
	interfaceListSize = 0;

    printMyLabels();
    printf("\n Processing messages now...\n");
    // Repeats the steps from now on
	while (1) {

        //printMyLabels();

		int flag = 0;
		int flagIP = 0;
		time1 = time(0);

		double timeDiff = difftime(time1, time0);

		char ctrlPayLoadB[200];
        char ctrlPayLoadNHopMessage[200];

        // Checking if we have end node link failure and also to check if any new entries are added to the table.
		// check for link failures.
		if (!endNode) { // only does when node is an end node.
			checkForLinkFailures(myAddr, numTierAddr);
			// if we have new failed IPS Advertise.
			if (failedEndIPs_head != NULL) {
				setInterfaces(); //interfaceList is being set by this function
				int loopCounter2 = 0;
				uint8_t *mplrPayload = allocate_ustrmem (IP_MAXPACKET);
				int mplrPayloadLen = 0;
				//buildPayloadRemoveAdvbuildPayloadRemoveAdvtsts 
				mplrPayloadLen = buildPayloadRemoveAdvts(mplrPayload, failedEndIPs_head); 
				if (mplrPayloadLen) {
					for (; loopCounter2 < interfaceListSize; loopCounter2++) {
						// MPLR TYPE 5.
						endNetworkSend(interfaceList[loopCounter2], mplrPayload, mplrPayloadLen);
					}
				}
				free(mplrPayload);
				//print_entries_LL();
				freeInterfaces();
				interfaceListSize = 0;
			}
		}

                // check if there are entries to be advertised.
        checkEntriesToAdvertise();
 
 		//Send hello packets after every 10 seconds
		if (timeDiff >= 10) {

			// test to print tier IP entries
			//print_entries_LL();

			memset(ctrlPayLoadB, '\0', 200);
			uint8_t numOfAddrB = getCountOfTierAddr();
			memcpy(ctrlPayLoadB, &numOfAddrB, 1);
			cpLength = 1;

			int qq = 0;
			for (; qq < getCountOfTierAddr(); qq++) {

				char tempAddrB[20];
				memset(tempAddrB, '\0', 20);
				strcpy(tempAddrB, getTierAddr(qq));
				freeGetTierAddr();

				printf("TEST:(helloM_List) ctrl packet - tempAddrB : %s  \n",tempAddrB);

				//int localTierSizeB = strlen(tempAddrB);
				uint8_t localTierSizeB = strlen(tempAddrB);

				//printf(
				//		"TEST:(helloM_List) ctrl packet - localTierSizeB : %u  \n",
				//		localTierSizeB);

				memcpy(ctrlPayLoadB + cpLength, &localTierSizeB, 1);
				cpLength = cpLength + 1;
				memcpy(ctrlPayLoadB + cpLength, tempAddrB, localTierSizeB);
				cpLength = cpLength + localTierSizeB;
                uint8_t hopCount = 1;
                memcpy(ctrlPayLoadB + cpLength, &hopCount, 1);
                cpLength = cpLength + 1;

			}
			
			printf("\n My test , ctrlPayLoadB = %s ",ctrlPayLoadB);

			// Send on multiple interface in a loop
			setInterfaces(); //interfaceList is being set by this function

			int loopCounter2 = 0;

            printf("\n Printing the hello message : ");
            int j = 0;
            for (; j < strlen(ctrlPayLoadB); j++) {
                //	Printing MPLR Data Packet
                printf("%02x ",ctrlPayLoadB[j] & 0xff);
            }
            printf("\n");


			for (; loopCounter2 < interfaceListSize; loopCounter2++) {

				ctrlSend(interfaceList[loopCounter2], ctrlPayLoadB); //send control messages
				MPLRCtrlSendCount++;

			}

			delete(); //free the memory allocated

			time0 = time(0); //reset time0
			freeInterfaces(); //
			interfaceListSize = 0;
			MPLRCtrlSendRound++;
			//printf("TEST: Inside timeDiff block sendCount: %lld\n",
				//	MPLRCtrlSendCount);

            // Here, we are checking the entries and if the entry is not it is not from the
            // same interface, the entries are send. Otherwise it is skipped.
            // THis is done in-order to prevent the race condition
            setInterfaces(); //interfaceList is being set by this function
            int counter = 0;
            for(; counter < interfaceListSize; counter++) {

                printf("Creating NHop message for %s",interfaceList[counter] );
                struct nodeHL *temp;
                temp = headHL;

                //Initializing the N HOP Message
                memset(ctrlPayLoadNHopMessage, '\0', 200);
                int numEntriesHopMessage = 0;
                int nHOPMessageLength = 1;

                while (temp != NULL) {
                    printf("\n%s ---> %s ---> %d\n", temp->tier, temp->port, temp->numHops);

                    int hopCount = temp->numHops;

//                    printf("Comapring hop count and interfaces");
//                    printf("\nhopCount = %d \n", hopCount);
//                    printf("temp->port=%s interfaceListSize[counter]=%s", temp->port,
//                           interfaceList[counter]);
                    // Check if the hello message received is within the limit of maximum number of hops
                    if (hopCount < MAX_NUM_HOPS && strcmp(temp->port, interfaceList[counter]) != 0) {
                        printf("\n Adding the above entry in the list");
                        // counting the number of address entries here
                        numEntriesHopMessage++;
                        // Copying the entry here
                        uint8_t lengthOfTierAddrTempUint = (uint8_t) strlen(temp->tier);
                        memcpy(ctrlPayLoadNHopMessage + nHOPMessageLength, &lengthOfTierAddrTempUint, 1);
                        printf("\nnHOPMessageLength = %d, lengthOfTierAddrTempUint = %x", nHOPMessageLength,
                               *(ctrlPayLoadNHopMessage + nHOPMessageLength));
                        nHOPMessageLength++;

                        memcpy(ctrlPayLoadNHopMessage + nHOPMessageLength, temp->tier,
                               lengthOfTierAddrTempUint);




                        printf("\nnHOPMessageLength = %d, temp->tier = %x", nHOPMessageLength,
                               *(ctrlPayLoadNHopMessage + nHOPMessageLength));
                        nHOPMessageLength = nHOPMessageLength + lengthOfTierAddrTempUint;

                        uint8_t hopCountTemp = (uint8_t)(hopCount + 1);
                        printf("\n$$$$ hopCount=%d hopCountTemp=%d", hopCount, hopCountTemp);
                        memcpy(ctrlPayLoadNHopMessage + nHOPMessageLength, &hopCountTemp, 1);
                        printf("\nnHOPMessageLength = %d, hopCount = %x", nHOPMessageLength,
                               *(ctrlPayLoadNHopMessage + nHOPMessageLength));
                        nHOPMessageLength++;
                    }
                    temp = temp->next;
                }

                printf("\nnumEntriesHopMessage = %d for Interface = %s\n", numEntriesHopMessage,interfaceList[counter]);



                if (numEntriesHopMessage > 0) {


                    uint8_t count = numEntriesHopMessage;
                    memcpy(ctrlPayLoadNHopMessage, &count, 1);
                    ctrlSend(interfaceList[counter], ctrlPayLoadNHopMessage);

                    printf("\n Printing the N Hop HEllo message : ");
                    int j = 0;
                    for (; j < nHOPMessageLength; j++) {
                        //	Printing MPLR Data Packet
                        printf("%02x ",ctrlPayLoadNHopMessage[j] & 0xff);
                    }
                    printf("\n");


                }
            }
		}

		socklen_t addr_len = sizeof src_addr;
		socklen_t addr_lenIP = sizeof src_addrIP;

        // Recieve messages from the IP socket created.
		nIP = recvfrom(sockIP, bufferIP, 2048, MSG_DONTWAIT,
				(struct sockaddr*) &src_addrIP, &addr_lenIP); //inbuilt function to recieve information from socket. If there is any message in socket, we recieve a avalue greater than -1 and if there is no message, -1 will be returned.

        // if no messages are available in the IP socket.
		if (nIP == -1) {
			flagIP = 1; //if no message is there in socket, nIP is -1 and flagIP is set to 1
		}

        // if some messages are available in the IP socket.
		if (flagIP == 0) { //if message is recived from socket flagIP remains 0 and we enter the loop

			unsigned int tcIP = src_addrIP.sll_ifindex;

			if_indextoname(tcIP, recvOnEtherPortIP); //if_indextoname() function returns the name of the network interface corresponding to the interface index ifindex

			// Fix for GENI , Ignoring messages from control interface
			char* ctrlInterface = "eth0";
			// printf("\n recvOnEtherPortIP = %s",recvOnEtherPortIP);
			
			// printf("\n ctrlInterface = %s",ctrlInterface);
			// printf("\n strlen(recvOnEtherPortIP) = %d",strlen(recvOnEtherPortIP));
			// printf("\n strlen(ctrlInterface) = %d",strlen(ctrlInterface));

			//printf("\n (strcmp(recvOnEtherPortIP, ctrlInterface))= %d \n",strcmp(recvOnEtherPortIP, ctrlInterface));

			if (strcmp(recvOnEtherPortIP, ctrlInterface) == 0) { //eth0 is the control interface. We do not need process any messages on eth0
				//printf("\n The message is from control interface. Hence Ignoring...");
				continue;
			}


			if (ctrlIFName != NULL) {

				if ((strncmp(recvOnEtherPortIP, ctrlIFName, strlen(ctrlIFName))
						!= 0)) {

					ipReceivedCount++;
					if(enableLogScreen){
						printf("TEST: IP Packet Received \n");
						printf("\n");
					}
					if(enableLogFiles){
						fprintf(fptr,"TEST: IP Packet Received \n");
						fprintf(fptr,"\n");
					}
					
					unsigned char *ipHeadWithPayload;
					int ipPacketSize = nIP - 14;
					ipHeadWithPayload = (unsigned char*) malloc(ipPacketSize);
					memset(ipHeadWithPayload, '\0', ipPacketSize);
					memcpy(ipHeadWithPayload, &bufferIP[14], ipPacketSize);

					//printf("\n");
					setInterfaces(); //interfaceList is being set by this function

					unsigned char ipDestTemp[7];
					memset(ipDestTemp, '\0', 7);
					sprintf(ipDestTemp, "%u.%u.%u.%u", ipHeadWithPayload[16],
							ipHeadWithPayload[17], ipHeadWithPayload[18],
							ipHeadWithPayload[19]);
					if(enableLogScreen)
						printf("IP Destination : %s  \n", ipDestTemp);
					if(enableLogFiles)
						fprintf(fptr,"IP Destination : %s  \n", ipDestTemp);

	
					unsigned char ipSourceTemp[7];
					memset(ipSourceTemp, '\0', 7);
					sprintf(ipSourceTemp, "%u.%u.%u.%u", ipHeadWithPayload[12],
							ipHeadWithPayload[13], ipHeadWithPayload[14],
							ipHeadWithPayload[15]);
					if(enableLogScreen)
						printf("IP Source  : %s  \n", ipSourceTemp);
					if(enableLogFiles)
						fprintf(fptr,"IP Source  : %s  \n", ipSourceTemp);

					if(enableLogScreen)
						printf("Calling Forwarding Algorithm - DataSend\n");
					if(enableLogFiles)
						fprintf(fptr,"Calling Forwarding Algorithm - DataSend\n");

					int packetFwdStatus = -1;

					if (isTierSet() == 0) {
						if(enableLogScreen)
							printf("%s: isTierSet() == 0",__FUNCTION__);
						if(enableLogFiles)
							fprintf(fptr,"%s: isTierSet() == 0",__FUNCTION__);

						boolean checkDestStatus =
								updateEndDestinationTierAddrHC(ipDestTemp);

						if (checkDestStatus == false) {
							errorCount++;
							if(enableLogScreen)
								printf("ERROR: End destination tier address not available \n");
							if(enableLogFiles)
								fprintf(fptr,"ERROR: End destination tier address not available \n");
						}

                        printf("\n Calling packetForwardAlgorithm linenumber =%d",__LINE__);
						packetFwdStatus = packetForwardAlgorithm(tierAddress,
								tierDest); //forward the packets using packetforwarding algorithm

					} else {
						if(enableLogScreen)
							printf("ERROR: Tier info was not set \n");
						if(enableLogFiles)
							fprintf(fptr,"ERROR: Tier info was not set \n");

						packetFwdStatus = packetForwardAlgorithm(tierAddress,
								tierDest);//forward the packets using packetforwarding algorithm

					}
					if(enableLogScreen)
						printf("%s: packetFwdStatus = %d \n",__FUNCTION__,packetFwdStatus);
					if(enableLogFiles)
						fprintf(fptr,"%s: packetFwdStatus = %d \n",__FUNCTION__,packetFwdStatus);

					if (packetFwdStatus == 0) { //if we are able to move forward, get inside the loop
						if(enableLogScreen)
							printf("%s: packetFwdStatus == 0",__FUNCTION__);
						if(enableLogFiles)
							fprintf(fptr,"%s: packetFwdStatus == 0",__FUNCTION__);

						if ((strlen(fwdTierAddr) == strlen(tierAddress))
								&& (strcmp(fwdTierAddr, tierAddress) == 0)) {
							if(enableLogScreen)
								printf("TEST: Received IP packet -(it's for me only, no forwarding needed) \n");
							if(enableLogFiles)
								fprintf(fptr,"TEST: Received IP packet -(it's for me only, no forwarding needed) \n");

						} else {
							if(enableLogScreen)
								printf("TEST: Recieved IP packet is not for me \n");
							if(enableLogFiles)
								fprintf(fptr,"TEST: Recieved IP packet is not for me \n");

							if (isFWDFieldsSet() == 0) {
								if(enableLogScreen){
									printf("TEST: Forwarding IP Packet as MNLR Data Packet (Encapsulation) \n");
									printf("TEST: Sending on interface: %s \n",fwdInterface);
								}
								if(enableLogFiles){
									fprintf(fptr,"TEST: Forwarding IP Packet as MNLR Data Packet (Encapsulation) \n");
									fprintf(fptr,"TEST: Sending on interface: %s \n",fwdInterface);
								}
								dataSend(fwdInterface, ipHeadWithPayload,
										tierDest, tierAddress, ipPacketSize); //send the data

								MPLRDataSendCount++;
							}
						}
					}

					freeInterfaces();
					interfaceListSize = 0;

				}
			} else {
				ipReceivedCount++;

				if(enableLogScreen){
					printf("TEST: IP Packet Received \n");
					printf("\n");
				}
				if(enableLogFiles){
					fprintf(fptr,"TEST: IP Packet Received \n");
					fprintf(fptr,"\n");
				}

				unsigned char *ipHeadWithPayload;
				int ipPacketSize = nIP - 14;
				ipHeadWithPayload = (unsigned char*) malloc(ipPacketSize);
				memset(ipHeadWithPayload, '\0', ipPacketSize);
				memcpy(ipHeadWithPayload, &bufferIP[14], ipPacketSize);

				if(enableLogScreen)
					printf("\n");
				if(enableLogFiles)
					fprintf(fptr,"\n");

				setInterfaces(); //interfaceList is being set by this function


				unsigned char ipDestTemp[7];
				sprintf(ipDestTemp, "%u.%u.%u.%u", ipHeadWithPayload[16],
						ipHeadWithPayload[17], ipHeadWithPayload[18],
						ipHeadWithPayload[19]);
				if(enableLogScreen)
					printf("[2]IP Destination : %s  \n", ipDestTemp);
				if(enableLogFiles)
					fprintf(fptr,"[2]IP Destination : %s  \n", ipDestTemp);
 				
				 unsigned char ipSourceTemp[7];
                                 memset(ipSourceTemp, '\0', 7);
                                 sprintf(ipSourceTemp, "%u.%u.%u.%u", ipHeadWithPayload[12],
                                                        ipHeadWithPayload[13], ipHeadWithPayload[14],
                                                        ipHeadWithPayload[15]);
                                 if(enableLogFiles)
                                 	printf("IP Source  : %s  \n", ipSourceTemp);
                                 if(enableLogFiles)
									fprintf(fptr,"IP Source  : %s  \n", ipSourceTemp);

				if(enableLogScreen)
					printf("Calling Forwarding Algorithm - DataSend\n");
				if(enableLogFiles)
					fprintf(fptr,"Calling Forwarding Algorithm - DataSend\n");

				// TESTING FWD ALGORITHM 2 - Case: IP Packet Received, Control Interface not set

				int packetFwdStatus = -1;

				if (isTierSet() == 0) {

					boolean checkDestStatus = updateEndDestinationTierAddrHC(
							ipDestTemp);

					if (checkDestStatus == false) {
						errorCount++;
						if(enableLogScreen)
							printf("ERROR: End destination tier address not available \n");
						if(enableLogFiles)
							fprintf(fptr,"ERROR: End destination tier address not available \n");
					}
                    printf("\n Calling packetForwardAlgorithm linenumber =%d",__LINE__);
					packetFwdStatus = packetForwardAlgorithm(tierAddress,
							tierDest);

				} else {
					if(enableLogScreen)
						printf("ERROR: Tier info was not set\n");
					if(enableLogFiles)
						fprintf(fptr,"ERROR: Tier info was not set\n");
                    printf("\n Calling packetForwardAlgorithm linenumber =%d",__LINE__);
					packetFwdStatus = packetForwardAlgorithm(tierAddress,
							tierDest);

				}

				if (packetFwdStatus == 0) {

					if ((strlen(fwdTierAddr) == strlen(tierAddress))
							&& (strcmp(fwdTierAddr, tierAddress) == 0)) {

						if(enableLogScreen)
							printf("TEST: Received IP packet -(it's for me only, no forwarding needed) \n");
						if(enableLogFiles)
							fprintf(fptr,"TEST: Received IP packet -(it's for me only, no forwarding needed) \n");

					} else {

						if (isFWDFieldsSet() == 0) {
							if(enableLogScreen){
								printf("TEST: Forwarding IP Packet as MNLR Data Packet (Encapsulation) \n");
								printf("Sending on interface: %s \n", fwdInterface);
							}	
							if(enableLogFiles){
								fprintf(fptr,"TEST: Forwarding IP Packet as MNLR Data Packet (Encapsulation) \n");
								fprintf(fptr,"Sending on interface: %s \n", fwdInterface);
							}
							dataSend(fwdInterface, ipHeadWithPayload, tierDest,
									tierAddress, ipPacketSize);

							MPLRDataSendCount++;

						}
					}
				}

				freeInterfaces();
				interfaceListSize = 0;
		

			}

		}

        // Receive messages from the control socket.
		n = recvfrom(sock, buffer, 2048, MSG_DONTWAIT,
				(struct sockaddr*) &src_addr, &addr_len); //recvfrom - receive a message from a socket

        if (n == -1) {
            //printf("\n No messages in the control socket. Time out!");
			flag = 1;
		}

        // if some message is present in the control socket.
		if (flag == 0) {

			unsigned int tc = src_addr.sll_ifindex;

			if_indextoname(tc, recvOnEtherPort); //mappings between network interface names and indexes

            printf("\n Control message recvd from %s",recvOnEtherPort);

			ethhead = (unsigned char *) buffer;

			if (ethhead != NULL) {
//
//				 printf("\n--------------------------------------"
//				 	"\n   MAC Destination : "
//				 		"%02x:%02x:%02x:%02x:%02x:%02x\n", ethhead[0],
//				 		ethhead[1], ethhead[2], ethhead[3], ethhead[4],
//				 		ethhead[5]);
//
//				 printf("        MAC Origin : "
//				 		"%02x:%02x:%02x:%02x:%02x:%02x\n", ethhead[6],
//				 		ethhead[7], ethhead[8], ethhead[9], ethhead[10],
//				 		ethhead[11]);
//				 printf("              Type : %02x:%02x \n", ethhead[12],
//				 		ethhead[13]);
//				 printf("               MSG : %d \n", ethhead[14]);
//				 printf("\n");

				MPLROtherReceivedCount++;

				uint8_t checkMSGType = (ethhead[14]);

				printf("\n%s : checkMSGType=%d\n",__FUNCTION__,checkMSGType);

                // Checking for different type of MNLR messages
                // 0x01 = Hello Message
                // 0x02 = Encapsulated IP Message
                // 0x05 = IP to Tier Address Mappping message

                // Below ones are the newly added ones for auto naming.
                // 0x06 = Join request message
                // 0x07 = Labels available message
                // 0x08 = Labels accepted message

                if (checkMSGType == 1) {

//                    printf("\n\n MESSAGE BEING RECIEVED\n");
//                    int j = 0;
//                    for (; j < strlen(ethhead); j++) {
//                        //	Printing MPLR Data Packet
//                        printf("MNLR Content : %d : %02x  \n", j,ethhead[j] & 0xff);
//                    }

                    int checksubMSGType = (ethhead[15]);

                    //printf("\n checksubMSGType= %d",checksubMSGType);


                        printf("\n");
                        printf("MPLR Ctrl Message received \n");
                        MPLRCtrlReceivedCount++;
                        MPLROtherReceivedCount--;

                        int tierAddrTotal = (ethhead[15]);

                        printf("  No. of Tier Addr : %d\n", tierAddrTotal);

                        //Print multiple tier address based on length
                        int lengthIndex = 16;
                        int z = 0;




                        for (; z < tierAddrTotal; z++) {

                            int lengthOfTierAddrTemp = 0;
                            lengthOfTierAddrTemp = (ethhead[lengthIndex]);
                            printf(" Length : %d \n",lengthOfTierAddrTemp);

                            unsigned char tierAddrTemp[lengthOfTierAddrTemp];
                            memset(tierAddrTemp, '\0', lengthOfTierAddrTemp);
                            lengthIndex = lengthIndex + 1;
                            memcpy(tierAddrTemp, &ethhead[lengthIndex],
                                   lengthOfTierAddrTemp);
                            lengthIndex = lengthIndex + lengthOfTierAddrTemp;

                            int hopCount = (int)(ethhead[lengthIndex]);
                            printf("\n Port=%s Address=%s HopCount=%d",recvOnEtherPort,tierAddrTemp,hopCount);
			
							printMyNeighbourTable();
                            int isNewPort = 0;


			
                            isNewPort = insert(tierAddrTemp, recvOnEtherPort, hopCount);
                            printf("\n After update, ");

                            printMyNeighbourTable();

                            if (z == 0) {

                                // the instant we know there is new port we start advertising our tierAdd<->IP table.
                                if (isNewPort) {

                                    // If new port we have to advertise our tierAdd<->IPAdd table.
                                    uint8_t *mplrPayload = allocate_ustrmem(
                                            IP_MAXPACKET);
                                    int mplrPayloadLen = 0;

                                    mplrPayloadLen = buildPayload(mplrPayload,
                                                                  COMPLETE_TABLE, 0);

                                    if (mplrPayloadLen) {
                                        endNetworkSend(recvOnEtherPort, mplrPayload,
                                                       mplrPayloadLen); //method to send MSG TYPE V
                                        //                                    printf("\n CHeck - recvOnEtherPort=%s",recvOnEtherPort);
                                    }
                                    free(mplrPayload);
                                }

                            }
							

                        }

                        unsigned char *ethhead2;
                        ethhead2 = (unsigned char *) (ethhead + 6);


				} // checkMsgType == 1 is over here

				if (checkMSGType == 2) {

					//printf("\n");
					printf("\n MNLR Data Message received checkMSGType=%d\n",checkMSGType);
					MPLRDataReceivedCount++;
					MPLROtherReceivedCount--;

					// TODO
					// (below line) to be implemented properly
					//printMPLRPacketDetails(abc,xyz);

					printf("Printing full MNLR Data packet \n");

					int j = 0;
					for (; j < n - 14; j++) {
                        //	Printing MPLR Data Packet
					   // printf("MNLR Content : %d : %02x  \n", j,buffer[j] & 0xff);
					}

					//printf("\n");

					// printing destination tier length
					unsigned char desLenTemp[2];
					sprintf(desLenTemp, "%u", ethhead[15]);
					uint8_t desLen = atoi(desLenTemp);
					//printf("  Destination Tier Length  : %d \n", desLen);

					// printing destination tier
					char destLocal[20];
					memset(destLocal, '\0', 20);
					memcpy(destLocal, &ethhead[16], desLen);

					//printf("  Destination Tier (Local) : %s \n", destLocal);

					char finalDes[20];
					memset(finalDes, '\0', 20);
					memcpy(finalDes, &destLocal, desLen);

					//printf("  Destination Tier (Final) : %s \n", finalDes);

					// printing source tier length
					int tempLoc = 16 + desLen;
					unsigned char srcLenTemp[2];
					sprintf(srcLenTemp, "%u", ethhead[tempLoc]);

					uint8_t srcLen = atoi(srcLenTemp);
					//printf("        Source Tier Length : %d \n", srcLen);

					// printing source tier
					tempLoc = tempLoc + 1;
					char srcLocal[20];
					memset(srcLocal, '\0', 20);
					memcpy(srcLocal, &ethhead[tempLoc], srcLen);

					//printf("      Source Tier (Local) : %s \n", srcLocal);

					char finalSrc[20];
					memset(finalSrc, '\0', 20);
					memcpy(finalSrc, &srcLocal, srcLen);

					//printf("      Source Tier (Final) : %s \n", finalSrc);

					//printf("\n");
					tempLoc = tempLoc + srcLen;

					// TESTING FWD ALGORITHM 3 - Case: MPLR Data Packet Received

					int packetFwdStatus = -1;

					if (isTierSet() == 0) {
                        printf("\n Calling packetForwardAlgorithm linenumber =%d finalDest=%s",__LINE__,finalDes);
						packetFwdStatus = packetForwardAlgorithm(tierAddress,
								finalDes);

					} else {
						errorCount++;
						//printf("ERROR: Tier info was not set \n");
                        printf("\n Calling packetForwardAlgorithm linenumber =%d",__LINE__);
						packetFwdStatus = packetForwardAlgorithm(tierAddress,
								finalDes);

					}

					if (packetFwdStatus == 0) {

						if ((strlen(fwdTierAddr) == strlen(tierAddress))
								&& (strcmp(fwdTierAddr, tierAddress) == 0)) {

							//printf("    Fwd Tier Addr : %s \n", fwdTierAddr);
							//printf("   DestFromPacket : %s \n", finalDes);
							//printf(
							//		"TEST: Received MPLR Data packet -(it's for me only, Decapsulation) \n");
							//MPLRDataReceivedCountForMe++;

							// TODO

							int decapsPacketSize = n - tempLoc;

							unsigned char decapsPacket[decapsPacketSize];
							memset(decapsPacket, '\0', decapsPacketSize);
							memcpy(decapsPacket, &buffer[tempLoc],
									decapsPacketSize);

							unsigned char tempIPTemp[7];
							sprintf(tempIPTemp, "%u.%u.%u.%u", decapsPacket[16],
									decapsPacket[17], decapsPacket[18],
									decapsPacket[19]);

							struct in_addr *nwIPTemp = getNetworkIP(tempIPTemp);
							char *portNameTemp = findPortName(nwIPTemp);

							// check for null
							// if not null, call dataDecapsulation
							if (portNameTemp != NULL) {
								//printf("Decaps at port %s \n", portNameTemp);

								dataDecapsulation(portNameTemp, decapsPacket,
										decapsPacketSize);

								MPLRDecapsulated++;
							} else {

								//printf("ERROR: MPLR Decapsulation failed \n");
								errorCount++;
							}

							//
							/* old logic
							 *
							 if ((strlen(fwdTierAddr) == strlen("1.3"))
							 && (strcmp(fwdTierAddr, "1.3") == 0)) {

							 dataDecapsulation("eth3", decapsPacket,
							 decapsPacketSize);
							 MPLRDecapsulated++;

							 } else {
							 if ((strlen(fwdTierAddr) == strlen("1.1"))
							 && (strcmp(fwdTierAddr, "1.1") == 0)) {

							 dataDecapsulation("eth4", decapsPacket,
							 decapsPacketSize);
							 MPLRDecapsulated++;

							 } else {

							 printf(
							 "ERROR: MPLR Decapsulation failed \n");
							 errorCount++;

							 }
							 }

							 */

						} else {

							if (isFWDFieldsSet() == 0) {

								int MPLRPacketSize = n - 14;
								// 	MPLRPacketSize = MPLRPacketSize + 1;          // check
								unsigned char MPLRPacketFwd[MPLRPacketSize];
								memset(MPLRPacketFwd, '\0', MPLRPacketSize);
								memcpy(MPLRPacketFwd, &buffer[14],
										MPLRPacketSize);

								//printf("TEST: Forwarding MPLR Data Packet \n");
								//printf("Forwarding on interface: %s \n",
										//fwdInterface);
								dataFwd(fwdInterface, MPLRPacketFwd,
										MPLRPacketSize);
								MPLRDataFwdCount++;
							}
						}
					} else {

						//printf("ERROR: MPLR Forwarding failed %d \n",
						//			packetFwdStatus);
						errorCount++;
					}

				} // CheckMsgType == 2 is over here

				if (checkMSGType == 5) {

					//printf("TEST: MPLR Message V received \n");
					MPLRMsgVReceivedCount++;
					MPLROtherReceivedCount--;

					// Add the data entries in the payload if not already present.
					uint8_t totalEntries = ethhead[15];

					// Action to be performed
					uint8_t action = ethhead[16];

					int index = 17;
					int hasDeletions = 0;
					while (totalEntries > 0) {
						uint8_t tierLen = ethhead[index];
						uint8_t tierAddr[tierLen + 1];

						memset(tierAddr, '\0', tierLen + 1);
						index++;        // pass the length byte

						memcpy(tierAddr, &ethhead[index], tierLen);

						index += tierLen;
						tierAddr[tierLen] = '\0';

						//printf("TierLen :%u TierAddr: %s\n", tierLen, tierAddr);

						uint8_t ipLen = ethhead[index];
						struct in_addr ipAddr;

						index++; // pass the length byte

						memcpy(&ipAddr, &ethhead[index],
								sizeof(struct in_addr));

						index = index + sizeof(struct in_addr);

						uint8_t cidr = ethhead[index];

						index++; // pass the length of cidr

						if (action == MESSAGE_TYPE_ENDNW_ADD) {
							//printf("\nipLen :%u ipAddr: %s cidr: %u\n", ipLen, inet_ntoa(ipAddr), cidr);

							struct addr_tuple *a = (struct addr_tuple*) malloc(
									sizeof(struct addr_tuple));
							memset(a, '\0', sizeof(struct addr_tuple));
							strcpy(a->tier_addr, tierAddr);

							if (find_entry_LL(&ipAddr, tierAddr) == NULL) {
								// insert info about index from which the packet has been received from.
								a->if_index = src_addr.sll_ifindex;
								a->isNew = true;
								memcpy(&a->ip_addr, &ipAddr,
										sizeof(struct in_addr));
								a->cidr = cidr;
								add_entry_LL(a);

								//print_entries_LL();
							}
						} else if (action == MESSAGE_TYPE_ENDNW_UPDATE) {

							// Still not implemented, can be done by recording interface index maybe

						} else if (action == MESSAGE_TYPE_ENDNW_REMOVE) {

                                                        if (delete_entry_LL_IP(ipAddr)) {
                                                                hasDeletions++;
                                                        }

						} else {

							//printf("WRONG MSG_ACTION: THIS SHOULD NOT HAPPEN");
						}
						totalEntries--;

					} // end of while

					// Send this frame out of interfaces other than where it came from
					if (hasDeletions && (action == MESSAGE_TYPE_ENDNW_REMOVE)) {

						uint8_t *mplrPayload = allocate_ustrmem(IP_MAXPACKET);
						int mplrPayloadLen = 0;
						int z = 0;
						for (z = 14; z < index; z++, mplrPayloadLen++) {
							mplrPayload[mplrPayloadLen] = ethhead[z];
						}
						setInterfaces(); //interfaceList is being set by this function

						if (mplrPayloadLen) {

							int ifs2 = 0;

							for (; ifs2 < interfaceListSize; ifs2++) {

								// dont send update, if it is from the same interface.
								if (strcmp(recvOnEtherPort, interfaceList[ifs2])
										!= 0) {

									endNetworkSend(interfaceList[ifs2],
											mplrPayload, mplrPayloadLen);
								}
							}
						}

						free(mplrPayload);
						freeInterfaces();
						interfaceListSize = 0;
						//print_entries_LL();
					}

				}	// MSG type 5 closed


                if (checkMSGType == MESSAGE_TYPE_JOIN) {
                    printf("\n Recieved MESSAGE_TYPE_JOIN ");
                    //sleep(1);
                    // check for the tierValue.
                    // if the tierValue is < my TierValue , ignore the message
                    // else
                    // generate the label based on the interface it recived the message from
                    // Send the label to the node
                    // Should get a label accepted message in return

                    uint8_t tierValueRequestedNode = ethhead[15];
                    printf("\n myTierValue = %d ", myTierValue);
                    printf("\n tierValueRequestedNode = %d ", tierValueRequestedNode);
                    if (myTierValue < tierValueRequestedNode) {
                        printf("\n MESSAGE_TYPE_JOIN request recvd from a node at lower tier ");

                        printf("\n Generating the label for the node");
                        printf("\n Interface from which the request recvd = %s", recvOnEtherPort);

                        struct labels* labelList;
                        //labelList = NULL;
                        int numbNewLabels = generateChildLabel(recvOnEtherPort, tierValueRequestedNode, &labelList);
                        printf("\n %s : Labels are generated\n",__FUNCTION__);
                        //Send this labelList to recvOnEtherPort

                        // Generate the payload for sending this message.

                        // Form the NULL join message here
                        char labelAssignmentPayLoad[200];
                        int cplength = 0;
                        // Clearing the payload
                        memset(labelAssignmentPayLoad, '\0', 200);

                        printf("\n %s : Creating the payload to send the new labels %s\n",__FUNCTION__,labelList->label);
                        printf("\n %s : Setting the number of labels in the payload \n",__FUNCTION__);
                        // Setting the number of labels being send
                        uint8_t numberOfLabels = (uint8_t) numbNewLabels; // Need to modify
                        memcpy(labelAssignmentPayLoad + cplength, &numberOfLabels, 1);
                        printf("\n %s : labelAssignmentPayLoad(string) = %s",__FUNCTION__,labelAssignmentPayLoad);
                        cplength++;

                        struct labels* temp = labelList;
                        while(temp!=NULL) {
                            printf("\n label list is not null\n");
                            printf("\n current label in the message = %s\n",temp->label);
                            printf("\n %s : Setting the label length in the payload \n", __FUNCTION__);
                            // Setting the label length being send
                            uint8_t labelLength = (uint8_t) strlen(temp->label); // Need to modify
                            memcpy(labelAssignmentPayLoad + cplength, &labelLength, 1);
                            cplength++;

                            printf("\n %s : Setting the label in the payload \n", __FUNCTION__);
                            // Setting the labels being send
                            // Need to modify
                            memcpy(labelAssignmentPayLoad + cplength, temp->label, labelLength);
                            cplength = cplength + labelLength;
                            temp = temp->next;
                            printf("\n %s : GETTING THE NEXT LABEL \n", __FUNCTION__);

                        }
                        printf("\n MESSAGE BEING SEND = %s",labelAssignmentPayLoad);
                        printf("\n MESSAGE LENGTH = %d",(int)strlen(labelAssignmentPayLoad));
                        int i;
                        for(i= 0 ; i < 200; i++){
                            printf("%c",labelAssignmentPayLoad[i]);
                        }

                      //  printf("\n MESSAGE BEING SEND = %s",labelAssignmentPayLoad);

                        printf("\n Sending MESSAGE_TYPE_LABELS_AVAILABLE  to all the interface, "
                                       "interfaceListSize = %d payloadSize=%d \n", interfaceListSize,
                               (int) strlen(labelAssignmentPayLoad));
                        // Send MESSAGE_TYPE_LABELS_AVAILABLE  (Message Type, Tier Value) to all other nodes
                        ctrlLabelSend(MESSAGE_TYPE_LABELS_AVAILABLE, recvOnEtherPort, labelAssignmentPayLoad);
                    }
                }


				if (checkMSGType == MESSAGE_TYPE_LABELS_AVAILABLE) {
					printf("\n Received MESSAGE_TYPE_LABELS_AVAILABLE \n");

					int numberLabels = ethhead[15];
					printf("\n Number of Labels available = %d\n", numberLabels);
					int i = 0;
					struct labels *acceptedList;


					// Accepting each label here
					int messagePointer = 16;
					for(;i<numberLabels;i++){

						int labelLength = ethhead[messagePointer];
						printf("\n Label Length = %d\n", labelLength);

						messagePointer++;

						char label[10];
                        memset(label,'\0', 10);
						memcpy(label,ethhead+messagePointer,labelLength);
                        printf("\n Label  =%s Label length= %d\n", label,(int)strlen(label));

						messagePointer = messagePointer + labelLength;

						if(!recvdLabel) {
							recvdLabel = true;
							//set the label here
							/* For Default tier address */
							setTierInfo(label);
						}

						// pass it to tier address list
						insertTierAddr(label);

				/*		tierAddr[0] = malloc(1 + strlen(label));
						strcpy(tierAddr[0], label);
						tierAddrCount++;*/

						printf("\nAdding the label to the list\n");

						//creating a list of accepted labels
						if(!acceptedList){
							printf("\nAdding the first label to the list\n");
							acceptedList  = (struct labels*) malloc(sizeof(struct labels));
							memcpy(acceptedList->label,label,strlen(label));
							acceptedList->next = NULL;
						}
						else {
							printf("\nAdding all other labels to the list\n");
							struct labels *newLabel = (struct labels *) malloc(sizeof(struct labels));
							memcpy(newLabel->label, label, strlen(label));
							newLabel->next = acceptedList;
							acceptedList = newLabel;
						}

					}

					// Generate Labels Accepted Message
					int cplength = 0;
					char labelAssignmentPayLoad[200];

					// Clearing the payload
					memset(labelAssignmentPayLoad,'\0', 200);

					// Set tbe labels here .. TO be done
					uint8_t numLabels = (uint8_t) numberLabels;
					memcpy(labelAssignmentPayLoad+cplength, &numLabels, 1);
					cplength++;

					struct labels *temp =acceptedList;
					// Copy all other labels to the labels accepted message
					while(!temp){

						char label[10];
						memcpy(label,temp->label,strlen(temp->label));

						// Set tbe labellength here
						uint8_t labelLength = strlen(label);
						memcpy(labelAssignmentPayLoad+cplength, &numLabels, 1);
						cplength++;

						// Set tbe label here
						memcpy(labelAssignmentPayLoad+cplength,label , labelLength);
						cplength = cplength + labelLength;
					}

					// Sending labels accepted message
					ctrlLabelSend(MESSAGE_TYPE_LABELS_ACCEPTED,recvOnEtherPort, labelAssignmentPayLoad);

                    printMyLabels();

				}

				if (checkMSGType == MESSAGE_TYPE_LABELS_ACCEPTED) {
					printf("\n Received MESSAGE_TYPE_LABELS_ACCEPTED \n");
				}


            }

        }
    }
	return 0;
}

/**
 * pingHello()
 *
 * method to call _get_MACTest
 *
 * @return status (int) - method return value
 */

int pingHello() {

	signal(SIGTSTP, signal_callbackZ_handler);

	//_get_MACTest(0, NULL);
	return 0;
}

/**
 * setInterfaces() //interfaceList is being set by this function
 *
 * method to set active interfaces in a interfaceList
 *
 * @return status (int) - method return value
 */
int setInterfaces() {

	if (isEnvSet() != 0)
		setControlIF();

	struct ifaddrs *ifaddr, *ifa;
	int family;

	int countInterface = 0;
	char temp[20];

	if (getifaddrs(&ifaddr) == -1) { /*The getifaddrs() function creates a linked list of structures
       describing the network interfaces of the local system, and stores the
       address of the first item of the list in *ifap.*/
		errorCount++;
		perror("ERROR: getifaddrs");
		exit(EXIT_FAILURE);
	}

	int init = 0;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		family = ifa->ifa_addr->sa_family;

		if (family == AF_PACKET) {

			strcpy(temp, ifa->ifa_name);

			// testing whether interface is up or not
			if ((ifa->ifa_flags & IFF_UP) == 0) {

			}

			else {

				if ((ifa->ifa_flags & IFF_LOOPBACK) == 0) {

					if (ctrlIFName != NULL) {

						if (strlen(temp) == strlen(ctrlIFName)) {

							if ((strncmp(temp, ctrlIFName, strlen(ctrlIFName))
									!= 0)) {

								countInterface = countInterface + 1;
								interfaceList[init] = malloc(1 + strlen(temp));
								strcpy(interfaceList[init], temp);
								init = init + 1;

							}

						} else {

							countInterface = countInterface + 1;
							interfaceList[init] = malloc(1 + strlen(temp));
							strcpy(interfaceList[init], temp);
							init = init + 1;

						}

					} else {

						countInterface = countInterface + 1;
						interfaceList[init] = malloc(1 + strlen(temp));
						strcpy(interfaceList[init], temp);
						init = init + 1;

					}
				}
			}

		}
	}

	interfaceListSize = countInterface;

	freeifaddrs(ifaddr);
	return 0;
}

/**
 * freeInterfaces()
 *
 * method to free interfaceList
 *
 * @return status (int) - method return value
 */
int freeInterfaces() {
	int loopCounter1 = 0;
	for (; loopCounter1 < interfaceListSize; loopCounter1++) {
		free(interfaceList[loopCounter1]);
	}
	interfaceListSize = 0;
	return 0;
}

/*
 *  EntryPoint - main()
 */
int main(int argc, char **argv) {
/*The 2 arguments are argc, which is the no of arguments and argv consists of the arguments
an example of an argument (argv) is ./run -T 2.1.2 -N 0 10.10.5.2 24 eth1. 7 components are present in this argument
1) -T - stands for tier label 2)2.1.2 - tier label address 3)-N - stands for Node 4)0 - 0 specifies edge node and 1 specifies non edge node
5)10.10.5.2 - ip address of the ip node 6)24 - subnet mask 7)eth1 - interface through which the ip node is connected to edge node. 
*/
//The argument comes as a single string. we need to split and save them to following variables
	char *tierAddr[20]; //this character array store the tier address 
	char *ipAddress[16]; //this character array store the ip address of ip node
	int cidrs[100] = { 0 }; //subnet mask eg: 24
	char *portName[10]; //interface no or port no eg : \eth0
	
	//The argument comes as a pointer of pointers. we need to split and save them to following variables
	int endNWCount = 0;
	int numActiveEndIPs = 0;
	struct in_addr ips[100];
	boolean tierSpecial = false;

	// RVP For Logging purpose
	char* filename = "./testLogs.txt"; //The filename to which logs is being saved.
	fptr = fopen(filename,"w");
	if(enableLogFiles) fprintf(fptr," Writing logs to file ... ");
	fflush(fptr); 

	// Checking for the validity of the arguments
	// Previous - 	sudo ./run -T 2.1.2 -N 0 10.10.5.2 24 eth3
	// Next - 	Root - 	sudo ./run -L 1 -T 1.1 -N 1
	// Next - 	Root - 	sudo ./run -L 2 -N 0 10.10.5.2 24 eth3

    // to be deleted RVP
   // printf("\n argv[0]= %s",argv[0] );


	if (argc > 1) { //if the number of arguments is greater than 1, we will go forward

		argc--; //decrement the counter
		argv++; //increment the memory location. first value in argv is "./run". now we incremented to "-T"
		while (argc > 0) { //the loop will continue until the number of arguments is greater than 1. 
			//We decrement the argc by 1 each time we process an element present in argv 

			char *opt = argv[0]; //declare a pointer and assign the first pointer in "argv" to "opt". {*argv = argv[0]}. "-T"  assigned to opt

			if (opt[0] != '-') { //We will check for first element in "-T". If the first element is not a "-", then this means the opt does not contain "-T", then we will exit the while loop. It should start with -T.... etc

				break;
			}
			opt = argv[0];
            //printf("\n OPT = %s",opt);
			// Checking for -L
			if (strcmp(opt, "-T") == 0) {
				argc--;
				argv++;
				myTierValue = convertStringToInteger(argv[0]);
				argc--; //argument counter decremented 
				argv++; // moved to next srting after "-T". In our example it will be "2.1.2"
			}
			else{
				printf(" Error : Invalid Argument , Tier Value is not passed ");
				exit(0);
			}

			printf("\n My Tier Value = %d", myTierValue);
			fprintf(fptr,"\n My Tier Value = %d", myTierValue);
			
			// Checking only if the current node belongs to Tier 1 
			// All other nodes will be named automatically based on the Tier 1 node.
			// Below code tries to get all the multiple address of the nodes and add it into a list.
			// One if the  address is set as the default or the special address.

			if(myTierValue == 1)
			{
                opt = argv[0];
				// Checking for -T
				if (strcmp(opt, "-L") == 0) {
					argc--;
					argv++;
					int initA = 0;

                    // Loop to extract the multiple labels and then setting the first label as the default label.
					do {
						char *next = argv[0];
						if (next[0] == '-') {
							break;
						}
						/* For Default tier address */
						if (tierSpecial == false) {
							setTierInfo(next); //function to set the tier address of the node. we are passing the tier label address(eg 2.1.2) as the argument.
							tierSpecial = true; //once we set the tier address for that node, we update the tierSpecial flag as 1.
						}
						// pass it to tier address list
						insertTierAddr(next); //each node can have many tier label since it may derive from more than one parent node. All the values are stored in a list.
						tierAddr[initA] = malloc(1 + strlen(next));
						strcpy(tierAddr[initA], next);
						initA++;
						tierAddrCount++;
						argc--;
						argv++;
					} while (argc > 0);
				}
			}
			
			// Checking whether the node is an edge node or an intermediate node.
			// Adding the interface and the edge node IP to the local lists.

			if (argc > 0) {
				opt = argv[0];
				if (strcmp(opt, "-N") == 0) {
					argc--;
					argv++;
					//printf("-N Detected \n");
					int edgeNode = atoi(argv[0]); //atoi converts string to integer. Here we checks what comes after the -N (0 or 1 comes after -N. 0 represent edge not and 1 represents not an edge node)
					int initB = 0;
					argc--;
					argv++;
					if (edgeNode == 0) { //if 0 comes aftrer -N, we are having an edge node and we enter this loop
						// Edge Node
						endNode = 0; //endNode is updated to 0, if we are at edge node
						int iterationNCount = 0;
						do {
							char *nextN = argv[0]; //here the ipaddress is stored in the pointer
							//printf("Before : %s  \n", nextN);
							if (nextN[0] == '-') {
								if (iterationNCount == 0) {
									totalIPError++;
									//printf("ERROR: \n");
								}
								break;
							}
							
							// pass it to other - IP
							ipAddress[initB] = malloc(1 + strlen(nextN)); //allocate memory for the ipaddress[initB]
							strcpy(ipAddress[initB], nextN); //copy the ipaddress of the ip node to ipaddress[initB]
							argc--;
							argv++;

							char *nextN2 = argv[0]; //here the subnet mask is saved to the pointer 
							//printf("Before : %s  \n", nextN2);
							if (nextN2[0] == '-') {
								totalIPError++;
								//printf("ERROR: \n");
								break;
							}

							//printf("CIDR : %s  \n", nextN2);
							// pass it to other - CIDR
							cidrs[initB] = atoi(nextN2); //convert the subnet mask to integer and store in cidrs[initB]
							argc--;
							argv++;

							char *nextN3 = argv[0]; //interface is saved in the pointer
							//printf("Before : %s  \n", nextN3);
							if (nextN3[0] == '-') {
								totalIPError++;
								//printf("ERROR: \n");
								break;
							}

							//printf("Port Name : %s  \n", nextN3);
							// pass it to other - Port name
							portName[initB] = malloc(1 + strlen(nextN3)); //allocate memory for portName[initB]
							strcpy(portName[initB], nextN3); //copy the interface to portName[initB]
							initB++;
							endNWCount++;
							argc--;
							argv++;
							iterationNCount++;
						} while (argc > 0);
					}
					else {
						//  skip till '-' encountered
						while (argc > 0) {
							char *skipNext = argv[0];
							//printf("Skipping: %s  \n", skipNext);
							if (skipNext[0] == '-') {
								break;
							}
							argc--;
							argv++;
						}
					}
				}
			}

			// Checking for -V
			if (argc > 0) {
				opt = argv[0];
				if ((strcmp(opt, "-V") == 0)
						|| (strcmp(opt, "-version") == 0)) {
					argc--;
					argv++;
					//printf("-V Detected \n");
				}
			}

			if (argc > 0) {
				opt = argv[0];
				if (!(strcmp(opt, "-V") == 0) && !(strcmp(opt, "-version") == 0)
				&& !(strcmp(opt, "-T") == 0)
				&& !(strcmp(opt, "-N") == 0))  //if -N or -V or -version or -T does not come after label address of node, then it is an error. 
				{
					argc--;
					argv++;
					totalIPError++;
					//printf("ERROR: Invalid parameter \n");
				}
			}

		}
	} else { //if argc <=0 then error

		totalIPError++;
		printf("ERROR: Not enough parameters \n");
		exit(0);
	}
	finalARGCValue = argc;

	// Copy Pranav's assignment here
	// Converting IP$ or IPV6 addresses from text to binary format.
	// Doing it for all the edge IP addresses.

	int z = 0;
	for (z = 0; z < endNWCount; z++) {
		//printf("T->%s cidr %d\n", ipAddress[z], cidrs[z]);
		inet_pton(AF_INET, ipAddress[z], &ips[z]); //inet_pton convert ipaddress from text to binary. ips[z] will have the binary form of the ip address stored as text in ipaddress[z]
		ips[z].s_addr = ntohl(ips[z].s_addr); //The ntohl() function converts the unsigned integer netlong from network byte order to host byte order.
		ips[z].s_addr = ((ips[z].s_addr >> (32 - cidrs[z])) << (32 - cidrs[z])); //remove the last element from ipaddress. eg 10.10.5.2 will change to 10.10.5.0. we are finding the network address from the host address.
		ips[z].s_addr = htonl(ips[z].s_addr); //The htonl() function converts the unsigned integer hostlong from host byte order to network byte order.
	}

    if(myTierValue != 1) {
        setInterfaces();
        getMyTierAddresses(tierAddr);
    }


	// if only end node then advertise updates, i.e. put entrie TierAdd<->IPaddtable
	// Table -> myAddr
	// Tier address , IP address 
	// 
	if (endNode == 0) { //endNode is updated to 0, if we are at edge node, ie if edgeNode is 0
		struct addr_tuple myAddr[endNWCount * (myTotalTierAddress)];
		//printf("T0->%d ->%d\n", endNWCount, myTotalTierAddress);

		int index1, index2;
		int numTemp = 0;
		// if there are multiple IPS then do number of end IPS * tier addresses entries here.
		for (index1 = 0; index1 < myTotalTierAddress; index1++) {
			for (index2 = 0; index2 < endNWCount; index2++) {

				strcpy(myAddr[numTemp].tier_addr, tierAddr[index1]);
				//printf("T1->%s ->%s\n", myAddr[numTemp].tier_addr,
				//		tierAddr[index1]);
				myAddr[numTemp].ip_addr = ips[index2];
				char *temp11;
				//printf("T2->%s ->%u\n",
			//			inet_ntop(AF_INET, &myAddr[numTemp].ip_addr, temp11,
			//					INET_ADDRSTRLEN), myAddr[numTemp].cidr);
				myAddr[numTemp].cidr = (uint8_t) cidrs[index2];
				strcpy(myAddr[numTemp].etherPortName, portName[index2]);
				//printf("T3->%s ->%s\n", myAddr[numTemp].etherPortName,
				//		portName[index2]);

				// Add code for port number

				//printf("ethname%s\n", myAddr[numTemp].tier_addr);
				numTemp++;

			}
		}

		// Populate the address table.
		// Can be written as a function.
		int i = 0;
		for (i = 0; i < myTotalTierAddress; i++) {
			struct addr_tuple *a = (struct addr_tuple*) calloc(1,
					sizeof(struct addr_tuple));
			strncpy(a->tier_addr, myAddr[i].tier_addr,
					strlen(myAddr[i].tier_addr));

			// source entry so making it -1, it will help while checking if address belongs to our own or not.
			a->if_index = -1;
			a->isNew = true;
			a->cidr = myAddr[i].cidr;
			strcpy(a->etherPortName, myAddr[i].etherPortName);
			memcpy(&a->ip_addr, &myAddr[i].ip_addr, sizeof(struct in_addr));
			add_entry_LL(a);
		}

		// Activating protocol to be ready
		pingHello();
		_get_MACTest(myAddr, numTemp);
	} else {
		// Activating protocol to be ready
		pingHello();
		_get_MACTest(NULL, 0);
		// testing print method
		//print_entries_LL();
	}
	// Copied Pranav's assignment before

	// ex-input format ./a.out 2 1.1 1.11

	return 0;
}

/**
 * freqCount(char[],char[])
 *
 * method to find count of particular character
 *
 * @param str (char[]) - string
 * @param search (char[]) - character to find occurrence
 *
 * @return count (int) - occurrence count of a character
 */
int freqCount(char str[], char search) {

	int i, count = 0;
	for (i = 0; str[i] != '\0'; ++i) {
		if (str[i] == search)
			count++;
	}

	return count;
}

// Function just to print the details based on the signal.

void signal_callbackZ_handler(int signum) {

	//printf("\n");
	//printf("TEST: Caught signal Z - %d \n", signum);
	//printf("\n");

	// Cleanup and close up stuff here

	// calling another function
	displayTierAddr();
	displayNeighbor();
	printInputStats();
	packetStats();

	// Terminate program
	exit(signum);
}

char* macAddrtoString(unsigned char* addr, char* strMAC, size_t size) {
	if (addr == NULL || strMAC == NULL || size < 18)
		return NULL;

	snprintf(strMAC, size, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1],
			addr[2], addr[3], addr[4], addr[5]);

	return strMAC;
}

int trimAndDelNewLine() {

	size_t len = strlen(specificLine);
	if (len > 0) {
		if (isspace(specificLine[len - 1]) || specificLine[len - 1] == '\n') {
			//printf("TEST: Inside trim: if of if:%c\n", specificLine[len - 1]);
			len = len - 1;
			specificLine[len] = '\0';
		}

	}
	return 0;
}

//removes the specified character 'ch' from 'str'
char *strrmc(char *str, char ch) {
	char *from, *to;
	from = to = str;
	while (*from) {
		if (*from == ch)
			++from;
		else
			*to++ = *from++;
	}
	*to = '\0';
	return str;
}
/*function to update the end tier address*/
boolean updateEndDestinationTierAddrHC(char tempIP[]) {

	boolean updateStatus = false;

	char *tempTier = updateEndTierAddr(tempIP);

	if (tempTier != NULL) {

		strcpy(tierDest, tempTier);
		updateStatus = true;
	}

	/*
	 *  // old logic
	 *
	 * if (strcmp(tempIP, "10.1.3.2") == 0) {
	 memset(tierDest, '\0', 12);
	 strcpy(tierDest, "1.1");
	 updateStatus = true;
	 } else {
	 if (strcmp(tempIP, "10.1.2.3") == 0) {
	 memset(tierDest, '\0', 12);
	 strcpy(tierDest, "1.3");
	 updateStatus = true;
	 }
	 }*/

	//printf("TEST: End Destination Tier Address : %s\n", tierDest);
	return updateStatus;
}

void printInputStats() {

	//printf("\n");
	//printf("\n");
	//printf("TEST: Printing Input Parameters related stats \n");
	//printf("TEST: Final value of ARGC : %d \n", finalARGCValue);
	//printf("TEST:     Total I/P Error : %d \n", totalIPError);
	//printf("\n");
}

int packetStats() {

	//printf(" \n");
	//printf("TEST:                    Current Node Tier Address : %s\n",
	//		tierAddress);
	//printf("TEST:                     Rounds of MPLR Ctrl sent : %lld\n",
	//		MPLRCtrlSendRound);
	//printf("TEST:                       MPLR Ctrl Packets sent : %lld\n",
	//		MPLRCtrlSendCount);
	//printf("TEST:                       MPLR Data Packets sent : %lld\n",
	//		MPLRDataSendCount);
	//printf("TEST:                  MPLR Data Packets forwarded : %lld\n",
	//		MPLRDataFwdCount);
	//printf("TEST:                          IP Packets received : %lld\n",
	//		ipReceivedCount);
	//printf("TEST:                   MPLR Ctrl Packets received : %lld\n",
	//		MPLRCtrlReceivedCount);
	//printf("TEST:                  MPLR MSG V Packets received : %lld\n",
	//		MPLRMsgVReceivedCount);
	//printf("TEST:                   MPLR Data Packets received : %lld\n",
	//		MPLRDataReceivedCount);
	//printf("TEST:    MPLR Data Packets received (Egress Node ) : %lld\n",
	//		MPLRDataReceivedCountForMe);
	//printf("TEST:                     MPLR Packet Decapsulated : %lld\n",
	//		MPLRDecapsulated);
	//printf("TEST:      MPLR Packet received (MSG Type not 1/2) : %lld\n",
	//		MPLROtherReceivedCount);
	//printf("TEST:                    Error Count (Since start) : %lld\n",
	//		errorCount);
	//printf("TEST:                       Control interface name : %s\n",
	//		ctrlIFName);
	return 0;
}

void checkEntriesToAdvertise() {
        setInterfaces(); //interfaceList is being set by this function
        int ifs = 0;
        // Whenever there is an update we have to advertise it to others.
        for (; ifs < interfaceListSize; ifs++) {

                // MPLR TYPE 5.
                // If new port we have to advertise our tierAdd<->IPAdd table.
                uint8_t *mplrPayload = allocate_ustrmem (IP_MAXPACKET);
                int mplrPayloadLen = 0;
                mplrPayloadLen = buildPayload(mplrPayload, ONLY_NEW_ENTRIES, (int)if_nametoindex(interfaceList[ifs]));
                if (mplrPayloadLen) {
                        endNetworkSend(interfaceList[ifs], mplrPayload, mplrPayloadLen);
                }
                free(mplrPayload);
        }
        freeInterfaces();
        interfaceListSize = 0;

        // Mark new entries as old now.
        clearEntryState();
}
/*
This function is used to populate the failed IP's to the variable "failedEndIPs_head". addr_tuple is being populated by the active interfaces.
*/
void checkForLinkFailures (struct addr_tuple *myAddr, int numTierAddr) {
        // Failed End Links IPS

        struct addr_tuple *current = failedEndIPs_head;
        struct addr_tuple *previous = NULL;

        // Check if any failed End IPS became active.
        while (current != NULL) {
                if (isInterfaceActive(current->ip_addr, current->cidr) ) {

                        // Add into the table as they are active.
                        struct addr_tuple *a = (struct addr_tuple*) calloc (1, sizeof(struct addr_tuple));
                        strcpy(a->tier_addr, current->tier_addr);
                        // insert info about index from which the packet has been received from.
                        a->if_index = -1;
                        a->isNew = true;
                        memcpy(&a->ip_addr, &current->ip_addr, sizeof(struct in_addr));
                        a->cidr = current->cidr;
                        add_entry_LL(a);
                        //print_entries_LL();

                        struct addr_tuple *freeptr;
                        if (current == failedEndIPs_head) {
                                failedEndIPs_head = current->next;
                                previous = NULL;
                                freeptr = current;
                                current = NULL;
                        } else {
                                previous->next = current->next;
                                freeptr = current;
                                current = current->next;
                        }
                        free(freeptr);
                        continue;
                }
                previous = current;
                current = current->next;
        }

        int i = 0;
        for (; i < numTierAddr; i++) {
                // and check to see if address is already added to failed IP list or not.
                struct addr_tuple *ptr = failedEndIPs_head;
                bool isInFailedList = false;

                while (ptr != NULL) {
                        if ((myAddr[i].ip_addr.s_addr == ptr->ip_addr.s_addr) && (strcmp(myAddr[i].tier_addr, ptr->tier_addr)==0) ) {
                                isInFailedList = true;
                                break;
                        }
                        ptr = ptr->next;
                }

                // if interface is not active, add to failed IP list.
                if ((!isInFailedList) && (!isInterfaceActive(myAddr[i].ip_addr, myAddr[i].cidr))) {
			struct addr_tuple *temp = (struct addr_tuple*) calloc (1, sizeof(struct addr_tuple));
                        memcpy(temp, &myAddr[i], sizeof(struct addr_tuple));
                        temp->isNew = true;
                        temp->next = NULL;
                        delete_entry_LL_IP(myAddr[i].ip_addr);
                        if (failedEndIPs_head == NULL) {
                                failedEndIPs_head = temp;
                                //printf("Will be removing this %s\n", inet_ntoa(failedEndIPs_head->ip_addr));
                        } else {
                                temp->next = failedEndIPs_head;
                                failedEndIPs_head = temp;
                        }
                }
        }

}
/*
This funtion is used to find all interfaces in a node and check whether interfaces are active
*/
bool isInterfaceActive(struct in_addr ip, int cidr) {
        // find all interfaces on the node.
        struct ifaddrs *ifaddr, *ifa;

        if (getifaddrs(&ifaddr)) { /* The getifaddrs() function creates a linked list of structures describing the network interfaces of the local system, and stores the address of the first item of the list in *ifap. */
                perror("Error: getifaddrs() Failed\n");
				if(enableLogScreen)
					printf("Is Interface Active\n");
                if(enableLogFiles)
					fprintf(fptr,"Is Interface Active\n");
                
		exit(0);
        }

        // loop through the list
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL) {
                        continue;
                }
                int family;
                family = ifa->ifa_addr->sa_family;

                // populate interface names, if interface is UP and if ethernet interface doesn't belong to control interface and Loopback interface.
                if (family == AF_INET && (strncmp(ifa->ifa_name, "lo", 2) != 0) && (ifa->ifa_flags & IFF_UP) != 0) {

                        struct sockaddr_in *ipaddr = ((struct sockaddr_in*) ifa->ifa_addr);
                        struct sockaddr_in *subnmask = ((struct sockaddr_in*) ifa->ifa_netmask);

                        if (ip.s_addr == (ipaddr->sin_addr.s_addr & subnmask->sin_addr.s_addr)) {
				freeifaddrs(ifaddr);
                                return true;
                        }

                }
        }
	freeifaddrs(ifaddr);
        return false;
}

/**
 * convertStringToInteger()
 *
 * Method which converts the passed string into integer.
 *
 * @return status (int) - method returns integer value of the string.
 */

int convertStringToInteger(char* num)
{
	int result=0,i=0;
	int len = strlen(num);
	for(i=0; i<len; i++){
		result = result * 10 + ( num[i] - '0' );
	}
	//printf("%d", result);
	return result;
}


/**
 * getMyTierAddresses()
 *
 * Method to obtain label for each node.
 * The label is received from nodes in the higher level by sending the JOIN requests.
 * The nodes at higher level will send a list of available labels which can be used by this node.
 * It will accept all the labels and sends the LABELS_ACCEPTED message back to the node from which
 * it recieved the labels.
 *
 * @return status (void) - method returns none.
 */

void getMyTierAddresses(char* tierAddr[])
{
    printf("\n Entering %s",__FUNCTION__);

    // Creating a socket here for auto addressing and related variables.
    int sock;
    struct sockaddr_ll src_addr;
    socklen_t addr_len = sizeof src_addr;
    char buffer[2048];
    unsigned char *ethhead = NULL;
    int n;
    char recvOnEtherPort[5];


    // Creating the MNLR CONTROL SOCKET HERE
    if ((sock = socket(AF_PACKET, SOCK_RAW, htons(0x8850))) < 0) {
        printf("\nERROR: MNLR Socket ");
    }
    printf("\n Created a socket for auto address label! ");

    // Checking whether label is set or not
    // If not set, will wait here till the label is set

	while(!recvdLabel)
	{
        int i =0;
        // Form the NULL join message here
        char labelAssignmentPayLoad[200];
        int cplength = 0;
        // Clearing the payload
        memset(labelAssignmentPayLoad,'\0', 200);

        // Setting the tierValue
        uint8_t tierValue = (uint8_t) myTierValue;
        memcpy(labelAssignmentPayLoad+cplength, &tierValue, 1);

        // Wait for 2 seconds
        sleep(2);

        printf("\n Sending NULL join request to all its interfaces, "
                       "interfaceListSize = %d payloadSize=%d",interfaceListSize,(int)strlen(labelAssignmentPayLoad));
        // Send NULL Join Message (Message Type, Tier Value) to all other nodes
        for (i =0;i < interfaceListSize; i++) {
            ctrlLabelSend(MESSAGE_TYPE_JOIN,interfaceList[i], labelAssignmentPayLoad);
        }

        printf("\n Waiting for MESSAGE_TYPE_LABELS_AVAILABLE messgae");
        // wait for addresses for some time
        n = recvfrom(sock, buffer, 2048, MSG_DONTWAIT,
                     (struct sockaddr*) &src_addr, &addr_len);
        if (n == -1) {
            printf("\n Timeout happened, NO MESSAGE_TYPE_LABELS_AVAILABLE\n");
        }
        else{

            unsigned int tc = src_addr.sll_ifindex;
            if_indextoname(tc, recvOnEtherPort);
            printf("\n Control message recvd from %s \n",recvOnEtherPort);
            ethhead = (unsigned char *) buffer;


            if (ethhead == NULL) {
                printf("\n AutoLabel recieved message is empty \n");
            }
            else{


                uint8_t checkMSGType = (ethhead[14]);
                printf("\n checkMSGType=%d \n",checkMSGType);



                 //   checkMSGType = (ethhead[15]);
                 //   printf("\n checkMSGType=%d \n",checkMSGType);

                    if (checkMSGType == MESSAGE_TYPE_LABELS_AVAILABLE) {
                        printf("\n Received MESSAGE_TYPE_LABELS_AVAILABLE \n");

                        int numberLabels = ethhead[15];
                        printf("\n Number of Labels available = %d", numberLabels);
                        int i = 0;
                        struct labels *acceptedList;


                        // Accepting each label here
                        int messagePointer = 16;
                        for(;i<numberLabels;i++){

                            int labelLength = ethhead[messagePointer];
                            printf("\n Label Length = %d\n", labelLength);

                            messagePointer++;

                            char label[10];
                            memcpy(label,ethhead+messagePointer,labelLength);
                            printf("\n Label  = %s  Label length= %d\n", label,(int)strlen(label));

                            messagePointer = messagePointer + labelLength;

                            if(!recvdLabel) {
                                recvdLabel = true;
                                //set the label here
                                /* For Default tier address */
                                setTierInfo(label);
                            }

                            // pass it to tier address list
                            insertTierAddr(label);

                            tierAddr[0] = malloc(1 + strlen(label));
                            strcpy(tierAddr[0], label);
                           // tierAddrCount++;

                            printf("\nAdding the label to the list\n");

                            //creating a list of accepted labels
                            if(!acceptedList){
                                printf("\nAdding the first label to the list\n");
                                acceptedList  = (struct labels*) malloc(sizeof(struct labels));
                                memcpy(acceptedList->label,label,strlen(label));
                                acceptedList->next = NULL;
                            }
                            else {
                                printf("\nAdding all other labels to the list\n");
                                struct labels *newLabel = (struct labels *) malloc(sizeof(struct labels));
                                memcpy(newLabel->label, label, strlen(label));
                                newLabel->next = acceptedList;
                                acceptedList = newLabel;
                            }


                        }

                        // Generate Labels Accepted Message
                        cplength = 0;

                        // Clearing the payload
                        memset(labelAssignmentPayLoad,'\0', 200);

                        // Set tbe labels here .. TO be done
                        uint8_t numLabels = (uint8_t) numberLabels;
                        memcpy(labelAssignmentPayLoad+cplength, &numLabels, 1);
                        cplength++;

                        struct labels *temp =acceptedList;
                        // Copy all other labels to the labels accepted message
                        while(!temp){

                            char label[10];
                            memcpy(label,temp->label,strlen(temp->label));

                            // Set tbe labellength here
                            uint8_t labelLength = strlen(label);
                            memcpy(labelAssignmentPayLoad+cplength, &numLabels, 1);
                            cplength++;

                            // Set tbe label here
                            memcpy(labelAssignmentPayLoad+cplength,label , labelLength);
                            cplength = cplength + labelLength;
                        }

                        // Sending labels accepted message
                        ctrlLabelSend(MESSAGE_TYPE_LABELS_ACCEPTED,recvOnEtherPort, labelAssignmentPayLoad);

                    }

                // Add timeout for wait
                // Get the addresses
                // Add it to the local table
                // Send the LABELS accepted update to the node from which it got the message
                // Break the loop,when you get labels from at-least two different nodes */

            }
        }

	}

    shutdown(sock,2);
    printf("\n Exiting %s",__FUNCTION__);
}

/**
 * addLabelsList()
 *
 * Add the passed label to the passed list
 *
 * @return status (void) - method returns none.
 */

void addLabelsList(struct labels* labelList,char label[]){

    printf("\nEnter %s, Label to be added : %s\n",__FUNCTION__,label);
    struct labels* newlabel = (struct labels*) malloc(sizeof(struct labels));
    memcpy(newlabel->label,label,strlen(label));
    newlabel->next = NULL;
    printf("\n Created the label %p \n",labelList);

    if(!labelList){
        printf("\n labellist is null now \n");
        labelList = newlabel;
    }
    else{
        struct labels* temp = labelList;
        while(temp->next!=NULL){
            temp = temp->next;
        }
        temp->next = newlabel;
    }
    printf("\nExit %s",__FUNCTION__);
}

/**
 * generateChildLabel()
 *
 * Generate the child labels with respect to all available labels for the current node.
 * Joins the Tier Value of the child , UID of each parent label and the interface to which the parent is connected.
 *
 * @return status (int) - method return the number of labels created.
 */

int generateChildLabel(char* myEtherPort, int childTier, struct labels** labelList) {

    printf("\n\n Inside %s", __FUNCTION__);
    printf("\n Child port = %s", myEtherPort);

    // Get each tier address and create the labels here

    char *myTierAddress = getTierInfo();
    int countNewLabels = 0;
    //printf("\n My TierAddress = %s", myTierAddress);

    // Creating the label for the child here
    char childLabel[10];
    memset(childLabel, '\0', 10);
    sprintf(childLabel, "%d", childTier);

    struct nodeTL *temp = headTL;
    *labelList = NULL;

    printf("\n Going through each of my tier addresses , primary address = %s\n",temp->tier);

    while (temp) {

        countNewLabels++;
        printf("\n Current TierAddress = %s \n", temp->tier);
        memset(childLabel, '\0', 10);
        sprintf(childLabel, "%d", childTier);
        // Creating the child Label here
        joinChildTierParentUIDInterface(childLabel, temp->tier, myEtherPort);

        struct labels* newLabel =  (struct labels*) malloc(sizeof(struct labels));

		printf("\n childLabel = %s  strlen(childLabel)=%d",childLabel,(int)strlen(childLabel));
        memcpy(newLabel->label,childLabel,strlen(childLabel)+1);
        printf("\n newLabel->label = %s  strlen(newLabel->label)=%d",childLabel,(int)strlen(newLabel->label));

        newLabel->next = *labelList;
		*labelList = newLabel;
        printf("\n Created the new child label = %s %p\n", newLabel->label,*labelList);
        temp = temp->next;
    }

    printf("\n Exit: %s , Number of labels generated = %d  labelList= %s\n",__FUNCTION__,countNewLabels,(*labelList)->label);
    return countNewLabels;

}

/**
 * joinChildTierParentUIDInterface()
 *
 * method to join the Tier Value of the child , UID of the parent and the interface to which the parent is connected.
 *
 * @return status (void) - method return none
 */

void joinChildTierParentUIDInterface(char childLabel[], char myTierAddress[],char myEtherPort[]){
    printf("\n My TierAddress = %s",myTierAddress);
    //Getting the UID of the label of hte current address.
    int i = 0;
    while(myTierAddress[i] != '.'){
        i++;
    }
    int curLengthChildLabel = strlen(childLabel);
    strcpy(childLabel+curLengthChildLabel, myTierAddress+i);
    printf("\n After appending the uid of the parent, childLabel = %s ",childLabel);

    char temp[10] = ".";
    strcpy(temp+1,myEtherPort+3);
    printf("\n temp = %s",temp);

    curLengthChildLabel = strlen(childLabel);
    strcpy(childLabel+curLengthChildLabel, temp);

    printf("\n After appending the port ID, ChildLabel = %s",childLabel);
}


/**
 * printMyLabels()
 *
 * method to print my labels.
 *
 * @return status (void) - method return none
 */

void printMyLabels(){

    struct nodeTL *temp = headTL;
    printf("\n My labels are: ");
    while(temp){
        printf(" %s ,",temp->tier);
        temp = temp->next;
    }
}

