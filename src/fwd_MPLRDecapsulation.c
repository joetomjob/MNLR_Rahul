/*
 * fwd_MPLRDecapsulation.c
 *
 *  Created on: Aug 08, 2015
 *  Updated on: Aug 31, 2015
 *      Author: Tejas Padliya - tsp3859@rit.edu
 */

#include "sendAndFwd.h"

#define HEADER_SIZE 14
#define IP_HEADER_LENGTH 20
#define UDP_HEADER_LENGTH 8

static int num_packets;

/*decapsulate the encapsulated message*/
int dataDecapsulation(char etherPort[20], unsigned char MPLRDecapsPacket[],
		int MPLRDecapsSize) {

	num_packets = 0;

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
			(struct sockaddr*) &socket_address, sizeof(struct sockaddr_ll)) < 0) //send a message on a socket
		printf("ERROR: Send failed\n");

	close(sockfd);
	return 0;
}


int dataDecapsulationUDP(char etherPort[20], unsigned char MPLRDecapsPacket[],
					  int MPLRDecapsSize){


	//joe
	unsigned char ipHeader[ IP_HEADER_LENGTH ];
	memcpy( ipHeader, MPLRDecapsPacket, IP_HEADER_LENGTH );


    /*printf( "IP Header Contents\n" );

    for( int i = 0; i < IP_HEADER_LENGTH; i++ )
    {
        printf( "%02x ", ipHeader[ i ] );
    }

    printf( "\n" );*/


	unsigned char udpHeader[ UDP_HEADER_LENGTH ];
	memcpy( udpHeader, &MPLRDecapsPacket[ IP_HEADER_LENGTH ], UDP_HEADER_LENGTH );


	/*printf( "UDP Header Contents\n" );

    for( int i = 0; i < UDP_HEADER_LENGTH; i++ )
    {
        printf( "%02x ", udpHeader[ i ] );
    }

    printf( "\n" );*/


    int dataSize = MPLRDecapsSize - IP_HEADER_LENGTH - UDP_HEADER_LENGTH;
    unsigned char dataContents[ dataSize ];
    memcpy( dataContents, &MPLRDecapsPacket[ IP_HEADER_LENGTH + UDP_HEADER_LENGTH ], dataSize );


    /*printf( "Data size = %d\n", dataSize );

    for( int i = 0; i < dataSize; i++ )
    {
        printf( "%02x ", dataContents[ i ] );
    }

    printf( "\n" );*/


    ///test
//	unsigned char ipPacket[MPLRDecapsSize - HEADER_SIZE];
//	memcpy(ipPacket, &MPLRDecapsPacket[HEADER_SIZE], MPLRDecapsSize - HEADER_SIZE );
//	struct iphdr *iph = (struct iphdr*)ipPacket;
//	printf("the protocol is : %d ",iph->protocol);
    ////test

    int port_number = udpHeader[ 2 ] << 8;
    port_number = port_number + udpHeader[ 3 ];
    printf( "port number is : %d ", port_number );

    if( port_number != 1234 && port_number != 69 && port_number != 21234 )
    {
        return 0;
    }

    int sockfdUDP;
    if( ( sockfdUDP = socket( AF_INET, SOCK_DGRAM, 0 ) ) == -1 )
    {
        printf("socket created\n");
    }
/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	struct sockaddr_in remaddr;
	int slen = sizeof( remaddr );

	unsigned char server[ 7 ];
	memset( server, '\0', 7 );
	sprintf( server, "%u.%u.%u.%u", ipHeader[ 16 ], ipHeader[ 17 ], ipHeader[ 18 ], ipHeader[ 19 ] );

	//printf( "Server = %s\n", server );

	//printf( "Port number = %d, %x\n", port_number, port_number );

	memset( ( char *) &remaddr, 0, sizeof( remaddr ) );

	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons( port_number );

	if( inet_aton( server, &remaddr.sin_addr ) == 0 )
	{
		fprintf( stderr, "inet_aton() failed\n" );
		exit( 1 );
	}

	printf( "Sending UDP datagram %d to %s port %d\n", num_packets++, server, port_number );

	if( sendto( sockfdUDP, dataContents, dataSize, 0, ( struct sockaddr * ) &remaddr, slen ) == -1 )
	{
		perror( "sendto" );
		exit( 1 );
	}

	close(sockfdUDP);

	return 0;


}