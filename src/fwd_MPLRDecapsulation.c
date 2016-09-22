/*
 * fwd_MPLRDecapsulation.c
 *
 *  Created on: Aug 08, 2015
 *  Updated on: Aug 31, 2015
 *      Author: Tejas Padliya - tsp3859@rit.edu
 */

#include "sendAndFwd.h"

int dataDecapsulation(char etherPort[20], unsigned char MPLRDecapsPacket[],
		int MPLRDecapsSize) {

	int payLoad_Size = -1;
	int frame_Size = -1;

	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;

	int tx_len = 0;
	char ifName[IFNAMSIZ];

	int tPH = 0;
	for (; tPH < MPLRDecapsSize; tPH++) {
		//printf("fwd_MPLRDecapsulation.c %d : %02x \n ", tPH,
		//		MPLRDecapsPacket[tPH] & 0xff);
	}
	//printf("\n");

	//printf("TEST: Decapsulation Packet size (Payload) %d\n", MPLRDecapsSize);

	uint8_t header[HEADER_SIZE];

	strcpy(ifName, etherPort);

	// Setting frame size
	payLoad_Size = MPLRDecapsSize;
	frame_Size = HEADER_SIZE + payLoad_Size;

	unsigned char payLoad[payLoad_Size];
	memcpy(payLoad, MPLRDecapsPacket, payLoad_Size);

	//printf("\n");

	// creating frame
	uint8_t frame[frame_Size];
	memset(frame, '\0', frame_Size);

	struct ether_header *eh = (struct ether_header *) header;
	struct sockaddr_ll socket_address;

	// Open RAW socket to send on
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		perror("ERROR: Socket Error");
	}

	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
		perror("ERROR: SIOCGIFINDEX - Misprint Compatibility");

	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
		perror(
				"ERROR: SIOCGIFHWADDR - Either interface is not correct or disconnected");

	// Initializing the Ethernet Header
	memset(header, 0, HEADER_SIZE);

	/*
	 *  Ethernet Header - 14 bytes
	 *
	 *  6 bytes - Destination MAC Address
	 *  6 bytes - Source MAC Address
	 *  2 bytes - EtherType
	 *
	 */

	eh->ether_shost[0] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[5];

	eh->ether_dhost[0] = MY_DEST_MAC0;
	eh->ether_dhost[1] = MY_DEST_MAC1;
	eh->ether_dhost[2] = MY_DEST_MAC2;
	eh->ether_dhost[3] = MY_DEST_MAC3;
	eh->ether_dhost[4] = MY_DEST_MAC4;
	eh->ether_dhost[5] = MY_DEST_MAC5;

	eh->ether_type = htons(0x0800);

	tx_len += sizeof(struct ether_header);

	// Copying header to frame
	memcpy(frame, header, 14);

	// Copying the payLoad to the frame
	memcpy(frame + 14, payLoad, payLoad_Size);

	// Printing initial frame
	//printf(
	//		"TEST: FWD-Decaps - Frame: %02x:%02x:%02x:%02x:%02x:%02x  %02x:%02x:%02x:%02x:%02x:%02x  %02x:%02x  %02x:\n",
	//		frame[0], frame[1], frame[2], frame[3], frame[4], frame[5],
	//		frame[6], frame[7], frame[8], frame[9], frame[10], frame[11],
	//		frame[12], frame[13], frame[14]);

	// Index of the network device
	socket_address.sll_ifindex = if_idx.ifr_ifindex;

	// Address length - 6 bytes
	socket_address.sll_halen = ETH_ALEN;

	// Destination MAC Address
	socket_address.sll_addr[0] = MY_DEST_MAC0;
	socket_address.sll_addr[1] = MY_DEST_MAC1;
	socket_address.sll_addr[2] = MY_DEST_MAC2;
	socket_address.sll_addr[3] = MY_DEST_MAC3;
	socket_address.sll_addr[4] = MY_DEST_MAC4;
	socket_address.sll_addr[5] = MY_DEST_MAC5;

	/*

	 // For testing purpose

	 int testIndex = 0;
	 int testSize=0;
	 testSize=tx_len + payLoad_Size;

	 for (; testIndex < testSize; testIndex++) {
	 printf("MPLR Decaps Pack in fwd_MPLRDecapsulation.c %d: %02x \n ",testIndex ,frame[testIndex] & 0xff);
	 }
	 printf("\n");

	 printf("TEST: DecapsulationPacketSize size %d\n", testSize);


	 */

	//printf("TEST: Before sendto() - fwd_MPLRDecapsulation.c \n");

	// Send packet (Decapsulation)
	if (sendto(sockfd, frame, tx_len + payLoad_Size, 0,
			(struct sockaddr*) &socket_address, sizeof(struct sockaddr_ll)) < 0)
		//printf("ERROR: Send failed\n");

	close(sockfd);
	return 0;
}
