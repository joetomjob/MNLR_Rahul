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
extern int setControlIF();

extern void printIPPacketDetails(unsigned char ipPacket[], int nIPSize);
extern void printMPLRPacketDetails(unsigned char mplrPacket[], int nSize);

void checkEntriesToAdvertise();
void checkForLinkFailures(struct addr_tuple *, int);
bool isInterfaceActive(struct in_addr, int);

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

int freqCount(char str[], char search);

void signal_callbackZ_handler(int signum);
char* macAddrtoString(unsigned char* addr, char* strMAC, size_t size);

int trimAndDelNewLine();
char *strrmc(char *str, char ch);

int packetStats();
void printInputStats();

int setInterfaces();
int freeInterfaces();

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

	time_t time0 = time(0);
	time_t time1;

	int sock, n;
	int sockIP, nIP;

	char buffer[2048];
	char bufferIP[2048];

	unsigned char *ethhead = NULL;
	unsigned char tempIP[1500];
	struct ether_addr ether;

	char recvOnEtherPort[5];
	char recvOnEtherPortIP[5];
	char MACString[18];

	struct sockaddr_ll src_addr;
	struct sockaddr_ll src_addrIP;

	if ((sock = socket(AF_PACKET, SOCK_RAW, htons(0x8850))) < 0) {
		errorCount++;
		perror("ERROR: MPLR Socket ");

	}

	if ((sockIP = socket(AF_PACKET, SOCK_RAW, htons(0x0800))) < 0) {
		errorCount++;
		perror("ERROR: IP Socket ");

	}

	char ctrlPayLoadA[200];
	memset(ctrlPayLoadA, '\0', 200);
	uint8_t numOfAddr = (uint8_t) getCountOfTierAddr();
	memcpy(ctrlPayLoadA, &numOfAddr, 1);
	int cpLength = 1;

	int q = 0;
	for (; q < getCountOfTierAddr(); q++) {

		char tempAddrA[20];
		memset(tempAddrA, '\0', 20);
		strcpy(tempAddrA, getTierAddr(q));
		freeGetTierAddr();

		//printf("TEST: (helloM_List) ctrl packet - tempAddrA : %s  \n",
		//		tempAddrA);

		uint8_t localTierSizeA = strlen(tempAddrA);

		//printf("TEST:(helloM_List) ctrl packet - localTierSizeA: %u  \n",
		//		localTierSizeA);

		memcpy(ctrlPayLoadA + cpLength, &localTierSizeA, 1);
		cpLength = cpLength + 1;
		memcpy(ctrlPayLoadA + cpLength, tempAddrA, localTierSizeA);
		cpLength = cpLength + localTierSizeA;

	}

	// Send should initiate before receive
	setInterfaces();

	int loopCounter1 = 0;
	for (; loopCounter1 < interfaceListSize; loopCounter1++) {
		ctrlSend(interfaceList[loopCounter1], ctrlPayLoadA);
		MPLRCtrlSendCount++;
	}
	MPLRCtrlSendRound++;

	time0 = time(0);
	freeInterfaces();
	interfaceListSize = 0;

	while (1) {

		int flag = 0;
		int flagIP = 0;
		time1 = time(0);

		double timeDiff = difftime(time1, time0);

		char ctrlPayLoadB[200];

                // Checking if we have end node link faliure and also to check if any new entries are added to the table.
		// check for link failures.
		if (!endNode) { // only does when node is an end node.
			checkForLinkFailures(myAddr, numTierAddr);
			// if we have new failed IPS Advertise.
			if (failedEndIPs_head != NULL) {
				setInterfaces();
				int loopCounter2 = 0;
				uint8_t *mplrPayload = allocate_ustrmem (IP_MAXPACKET);
				int mplrPayloadLen = 0;
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

				//printf("TEST:(helloM_List) ctrl packet - tempAddrB : %s  \n",
				//		tempAddrB);

				//int localTierSizeB = strlen(tempAddrB);
				uint8_t localTierSizeB = strlen(tempAddrB);

				//printf(
				//		"TEST:(helloM_List) ctrl packet - localTierSizeB : %u  \n",
				//		localTierSizeB);

				memcpy(ctrlPayLoadB + cpLength, &localTierSizeB, 1);
				cpLength = cpLength + 1;
				memcpy(ctrlPayLoadB + cpLength, tempAddrB, localTierSizeB);
				cpLength = cpLength + localTierSizeB;

			}

			// Send on multiple interface in a loop
			setInterfaces();

			int loopCounter2 = 0;

			for (; loopCounter2 < interfaceListSize; loopCounter2++) {

				ctrlSend(interfaceList[loopCounter2], ctrlPayLoadB);
				MPLRCtrlSendCount++;

			}

			delete();

			time0 = time(0);
			freeInterfaces();
			interfaceListSize = 0;
			MPLRCtrlSendRound++;
			//printf("TEST: Inside timeDiff block sendCount: %lld\n",
				//	MPLRCtrlSendCount);
		}

		socklen_t addr_len = sizeof src_addr;
		socklen_t addr_lenIP = sizeof src_addrIP;

		nIP = recvfrom(sockIP, bufferIP, 2048, MSG_DONTWAIT,
				(struct sockaddr*) &src_addrIP, &addr_lenIP);

		if (nIP == -1) {
			flagIP = 1;
		}

		if (flagIP == 0) {

			unsigned int tcIP = src_addrIP.sll_ifindex;

			if_indextoname(tcIP, recvOnEtherPortIP);

			if (ctrlIFName != NULL) {

				if ((strncmp(recvOnEtherPortIP, ctrlIFName, strlen(ctrlIFName))
						!= 0)) {

					ipReceivedCount++;

					//printf("TEST: IP Packet Received \n");

					//printf("\n");

					unsigned char *ipHeadWithPayload;
					int ipPacketSize = nIP - 14;
					ipHeadWithPayload = (unsigned char*) malloc(ipPacketSize);
					memset(ipHeadWithPayload, '\0', ipPacketSize);
					memcpy(ipHeadWithPayload, &bufferIP[14], ipPacketSize);

					//printf("\n");
					setInterfaces();

					unsigned char ipDestTemp[7];
					memset(ipDestTemp, '\0', 7);
					sprintf(ipDestTemp, "%u.%u.%u.%u", ipHeadWithPayload[16],
							ipHeadWithPayload[17], ipHeadWithPayload[18],
							ipHeadWithPayload[19]);
					//printf("IP Destination : %s  \n", ipDestTemp);

					//printf("Calling Forwarding Algorithm - DataSend\n");

					int packetFwdStatus = -1;

					if (isTierSet() == 0) {

						boolean checkDestStatus =
								updateEndDestinationTierAddrHC(ipDestTemp);

						if (checkDestStatus == false) {
							errorCount++;
							//printf(
							//		"ERROR: End destination tier address not available \n");
						}

						packetFwdStatus = packetForwardAlgorithm(tierAddress,
								tierDest);

					} else {
						//printf("ERROR: Tier info was not set \n");

						packetFwdStatus = packetForwardAlgorithm(tierAddress,
								tierDest);

					}

					if (packetFwdStatus == 0) {

						if ((strlen(fwdTierAddr) == strlen(tierAddress))
								&& (strcmp(fwdTierAddr, tierAddress) == 0)) {

							//printf(
							//		"TEST: Received IP packet -(it's for me only, no forwarding needed) \n");

						} else {

							if (isFWDFieldsSet() == 0) {

								//printf(
								//		"TEST: Forwarding IP Packet as MPLR Data Packet (Encapsulation) \n");
								//printf("TEST: Sending on interface: %s \n",
									//	fwdInterface);
								dataSend(fwdInterface, ipHeadWithPayload,
										tierDest, tierAddress, ipPacketSize);

								MPLRDataSendCount++;
							}
						}
					}

					freeInterfaces();
					interfaceListSize = 0;

				}
			} else {
				ipReceivedCount++;

				//printf("TEST: IP Packet Received \n");

				//printf("\n");

				unsigned char *ipHeadWithPayload;
				int ipPacketSize = nIP - 14;
				ipHeadWithPayload = (unsigned char*) malloc(ipPacketSize);
				memset(ipHeadWithPayload, '\0', ipPacketSize);
				memcpy(ipHeadWithPayload, &bufferIP[14], ipPacketSize);

				//printf("\n");

				setInterfaces();

				unsigned char ipDestTemp[7];
				sprintf(ipDestTemp, "%u.%u.%u.%u", ipHeadWithPayload[16],
						ipHeadWithPayload[17], ipHeadWithPayload[18],
						ipHeadWithPayload[19]);
				//printf("IP Destination : %s  \n", ipDestTemp);

				//printf("Calling Forwarding Algorithm - DataSend\n");

				// TESTING FWD ALGORITHM 2 - Case: IP Packet Received, Control Interface not set

				int packetFwdStatus = -1;

				if (isTierSet() == 0) {

					boolean checkDestStatus = updateEndDestinationTierAddrHC(
							ipDestTemp);

					if (checkDestStatus == false) {
						errorCount++;
						//printf(
						//		"ERROR: End destination tier address not available \n");
					}

					packetFwdStatus = packetForwardAlgorithm(tierAddress,
							tierDest);

				} else {
					//printf("ERROR: Tier info was not set\n");

					packetFwdStatus = packetForwardAlgorithm(tierAddress,
							tierDest);

				}

				if (packetFwdStatus == 0) {

					if ((strlen(fwdTierAddr) == strlen(tierAddress))
							&& (strcmp(fwdTierAddr, tierAddress) == 0)) {

						//printf(
					//			"TEST: Received IP packet -(it's for me only, no forwarding needed) \n");

					} else {

						if (isFWDFieldsSet() == 0) {

							//printf(
						//			"TEST: Forwarding IP Packet as MPLR Data Packet (Encapsulation) \n");
							//printf("Sending on interface: %s \n", fwdInterface);
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

		n = recvfrom(sock, buffer, 2048, MSG_DONTWAIT,
				(struct sockaddr*) &src_addr, &addr_len);
		if (n == -1) {

			flag = 1;

		}

		if (flag == 0) {

			unsigned int tc = src_addr.sll_ifindex;

			if_indextoname(tc, recvOnEtherPort);

			ethhead = (unsigned char *) buffer;

			if (ethhead != NULL) {

				//printf("\n--------------------------------------"
				//		"\n   MAC Destination : "
				//		"%02x:%02x:%02x:%02x:%02x:%02x\n", ethhead[0],
				//		ethhead[1], ethhead[2], ethhead[3], ethhead[4],
				//		ethhead[5]);

				//printf("        MAC Origin : "
				//		"%02x:%02x:%02x:%02x:%02x:%02x\n", ethhead[6],
				//		ethhead[7], ethhead[8], ethhead[9], ethhead[10],
				//		ethhead[11]);
				//printf("              Type : %02x:%02x \n", ethhead[12],
				//		ethhead[13]);
				//printf("               MSG : %d \n", ethhead[14]);
				//printf("\n");

				MPLROtherReceivedCount++;

				uint8_t checkMSGType = (ethhead[14]);

				if (checkMSGType == 1) {
					//printf("\n");
					//printf("MPLR Ctrl Message received \n");
					MPLRCtrlReceivedCount++;
					MPLROtherReceivedCount--;

					int tierAddrTotal = (ethhead[15]);

					//printf("  No. of Tier Addr : %d\n", tierAddrTotal);

					// Print multiple tier address based on length
					int lengthIndex = 16;
					int z = 0;

					for (; z < tierAddrTotal; z++) {

						int lengthOfTierAddrTemp = 0;
						lengthOfTierAddrTemp = (ethhead[lengthIndex]);
						//printf("            Length : %d \n",
						//		lengthOfTierAddrTemp);

						unsigned char tierAddrTemp[lengthOfTierAddrTemp];
						memset(tierAddrTemp, '\0', lengthOfTierAddrTemp);
						lengthIndex = lengthIndex + 1;
						memcpy(tierAddrTemp, &ethhead[lengthIndex],
								lengthOfTierAddrTemp);
						lengthIndex = lengthIndex + lengthOfTierAddrTemp;

						//printf("              Data : %s\n", tierAddrTemp);
						int isNewPort = 0;

						isNewPort = insert(tierAddrTemp, recvOnEtherPort);

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
											mplrPayloadLen);
								}
								free(mplrPayload);
							}

						}

					}

					unsigned char *ethhead2;
					ethhead2 = (unsigned char *) (ethhead + 6);

					//printf(
					//		" MAC Origin Copied : %02x:%02x:%02x:%02x:%02x:%02x \n",
					//		ethhead2[0], ethhead2[1], ethhead2[2], ethhead2[3],
					//		ethhead2[4], ethhead2[5]);
					//printf("\n");

				} // checkMsgType == 1 is over here

				if (checkMSGType == 2) {

					//printf("\n");
					//printf("TEST: MPLR Data Message received \n");
					MPLRDataReceivedCount++;
					MPLROtherReceivedCount--;

					// TODO
					// (below line) to be implemented properly
					//printMPLRPacketDetails(abc,xyz);

					//printf("Printing full MPLR Data packet \n");

					int j = 0;
					for (; j < n - 14; j++) {

						//	Printing MPLR Data Packet
						//printf("MPLR Content : %d : %02x  \n", j,
						//		buffer[j] & 0xff);
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

						packetFwdStatus = packetForwardAlgorithm(tierAddress,
								finalDes);

					} else {
						errorCount++;
						//printf("ERROR: Tier info was not set \n");

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
						setInterfaces();

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
 * setInterfaces()
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

	if (getifaddrs(&ifaddr) == -1) {
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

	char *tierAddr[20];
	char *ipAddress[16];
	int cidrs[100] = { 0 };
	char *portName[10];

	int tierAddrCount = 0;
	int endNWCount = 0;

	int numActiveEndIPs = 0;
	struct in_addr ips[100];

	boolean tierSpecial = false;

	//system("sudo service ntp restart");

	if (argc > 1) {

		argc--;
		argv++;

		while (argc > 0) {

			char *opt = argv[0];

			if (opt[0] != '-') {

				break;
			}

			opt = argv[0];

			// Checking for -T
			if (strcmp(opt, "-T") == 0) {

				argc--;
				argv++;

				//printf("-T Detected \n");
				int initA = 0;

				do {

					char *next = argv[0];

					//printf("Before : %s  \n", next);

					if (next[0] == '-') {
						break;
					}

					/* For Default tier address */
					if (tierSpecial == false) {

						setTierInfo(next);
						tierSpecial = true;

					}

					//printf("Tier Address : %s  \n", next);

					// pass it to tier address list
					insertTierAddr(next);

					tierAddr[initA] = malloc(1 + strlen(next));
					strcpy(tierAddr[initA], next);

					initA++;
					tierAddrCount++;

					argc--;
					argv++;

				} while (argc > 0);

			}

			// Checking for -N
			if (argc > 0) {
				opt = argv[0];

				if (strcmp(opt, "-N") == 0) {

					argc--;
					argv++;

					//printf("-N Detected \n");

					int edgeNode = atoi(argv[0]);
					int initB = 0;

					argc--;
					argv++;

					if (edgeNode == 0) {

						// Edge Node

						endNode = 0;
						int iterationNCount = 0;

						do {

							char *nextN = argv[0];

							//printf("Before : %s  \n", nextN);

							if (nextN[0] == '-') {

								if (iterationNCount == 0) {

									totalIPError++;
									//printf("ERROR: \n");
								}

								break;
							}

							//printf("IP Address : %s  \n", nextN);

							// pass it to other - IP
							ipAddress[initB] = malloc(1 + strlen(nextN));
							strcpy(ipAddress[initB], nextN);

							argc--;
							argv++;

							char *nextN2 = argv[0];

							//printf("Before : %s  \n", nextN2);

							if (nextN2[0] == '-') {

								totalIPError++;
								//printf("ERROR: \n");
								break;
							}

							//printf("CIDR : %s  \n", nextN2);

							// pass it to other - CIDR
							cidrs[initB] = atoi(nextN2);

							argc--;
							argv++;

							char *nextN3 = argv[0];

							//printf("Before : %s  \n", nextN3);

							if (nextN3[0] == '-') {

								totalIPError++;
								//printf("ERROR: \n");
								break;
							}

							//printf("Port Name : %s  \n", nextN3);

							// pass it to other - Port name
							portName[initB] = malloc(1 + strlen(nextN3));
							strcpy(portName[initB], nextN3);

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

				&& !(strcmp(opt, "-N") == 0)

				) {

					argc--;
					argv++;

					totalIPError++;

					//printf("ERROR: Invalid parameter \n");

				}
			}

		}
	} else {

		totalIPError++;
		//printf("ERROR: Not enough parameters \n");

	}

	finalARGCValue = argc;

	// Copy Pranav's assignment here

	int z = 0;
	for (z = 0; z < endNWCount; z++) {
		//printf("T->%s cidr %d\n", ipAddress[z], cidrs[z]);
		inet_pton(AF_INET, ipAddress[z], &ips[z]);
		ips[z].s_addr = ntohl(ips[z].s_addr);
		ips[z].s_addr = ((ips[z].s_addr >> (32 - cidrs[z])) << (32 - cidrs[z]));
		ips[z].s_addr = htonl(ips[z].s_addr);
	}

	// if only end node then advertise updates, i.e. put entrie TierAdd<->IPaddtable
	if (endNode == 0) {
		struct addr_tuple myAddr[endNWCount * (tierAddrCount)];
		//printf("T0->%d ->%d\n", endNWCount, tierAddrCount);

		int index1, index2;
		int numTemp = 0;
		// if there are multiple IPS then do number of end IPS * tier addresses entries here.
		for (index1 = 0; index1 < tierAddrCount; index1++) {
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

		int i = 0;
		for (i = 0; i < tierAddrCount; i++) {
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
        setInterfaces();
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

bool isInterfaceActive(struct in_addr ip, int cidr) {
        // find all interfaces on the node.
        struct ifaddrs *ifaddr, *ifa;

        if (getifaddrs(&ifaddr)) {
                perror("Error: getifaddrs() Failed\n");
                printf("Is Interface Active\n");
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
