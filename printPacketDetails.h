/*
 * printPacketDetails.h
 *
 *  Created on: Sep 09, 2015
 *      Author: tsp3859
 */

#ifndef PRINTPACKETDETAILS_H_

	// TODO
	// Implement generic parser in two parts
		// a. print IP Packet
		// b. print MPLR Packet

void printIPPacketDetails(unsigned char ipPacket[], int nIPSize) {

	// TODO
	// Modernize

	int j = 0;
	// Recent changes below removed -14
	for (; j < nIPSize; j++) {

//	printf("TEST: %d  \n", ipHeadWithPayload[j]);
		printf("IP Content : %02x  \n", ipPacket[j] & 0xff);
	}

	printf("\n");

	u_int8_t ipVHL;

	memcpy(&ipVHL, ipPacket, 1);
	printf("          IPVHL d : %d \n", ipVHL);
	printf("        IPVHL 02x : %02x \n", ipVHL);

	// TODO
	// Fix length
	int ipVersion = ipVHL / 10;
	int ipheaderLength = ipVersion * (ipVHL % 10);
	int protoType = (signed char) ipPacket[9];

	printf("       IP Version : %02x \n", ipVersion);
	printf("            IP HL : %d \n", ipheaderLength);
	printf("   Protocol Value : %d  \n", protoType);

	if (protoType == 1)
		printf("  Protocol Type : ICMP \n");
	else if (protoType == 2)
		printf("  Protocol Type : IGMP \n");
	else if (protoType == 6)
		printf("  Protocol Type : TCP \n");
	else if (protoType == 17)
		printf("  Protocol Type : UDP \n");
	else if (protoType == 91)
		printf("  Protocol Type : LARP \n");
	else if (protoType == 54)
		printf("  Protocol Type : NARP \n");
	else
		printf("  Protocol Type : OTHER \n");

	unsigned char ipSourceTemp[7];
	sprintf(ipSourceTemp, "%u.%u.%u.%u", ipPacket[12], ipPacket[13],
			ipPacket[14], ipPacket[15]);

	unsigned char ipDestTemp[7];
	sprintf(ipDestTemp, "%u.%u.%u.%u", ipPacket[16], ipPacket[17], ipPacket[18],
			ipPacket[19]);

	printf("       IP Source : %s  \n", ipSourceTemp);
	printf("  IP Destination : %s  \n", ipDestTemp);

}

void printMPLRPacketDetails(unsigned char mplrPacket[], int nSize) {

	// TODO
	// Modernize

	int j = 0;
	for (; j < nSize - 14; j++) {

		printf("Content : %02x  \n", mplrPacket[j] & 0xff);
	}

	printf("\n");

}



#endif /* PRINTPACKETDETAILS_H_ */
