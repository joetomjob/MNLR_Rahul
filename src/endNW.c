#include "endNetworkUtils.h"

struct addr_tuple *tablehead = NULL;

void clearEntryState() {
	struct addr_tuple *current;
	for (current = tablehead; current != NULL; current = current->next) {
		current->isNew = false;
	}
}

char * allocate_strmem(int len) {
	void *tmp;

	if (len <= 0) {
		fprintf(stderr,
				"ERROR: Cannot allocate memory because len = %i in allocate_strmem().\n",
				len);
		exit(EXIT_FAILURE);
	}

	tmp = (char *) malloc(len * sizeof(char));
	if (tmp != NULL) {
		memset(tmp, 0, len * sizeof(char));
		return (tmp);
	} else {
		fprintf(stderr,
				"ERROR: Cannot allocate memory for array allocate_strmem().\n");
		exit(EXIT_FAILURE);
	}
}

uint8_t * allocate_ustrmem(int len) {
	void *tmp;

	if (len <= 0) {
		fprintf(stderr,
				"ERROR: Cannot allocate memory because len = %i in allocate_ustrmem().\n",
				len);
		exit(EXIT_FAILURE);
	}

	tmp = (uint8_t *) malloc(len * sizeof(uint8_t));
	if (tmp != NULL) {
		memset(tmp, '\0', len * sizeof(uint8_t));
		return (tmp);
	} else {
		fprintf(stderr,
				"ERROR: Cannot allocate memory for array allocate_ustrmem().\n");
		exit(EXIT_FAILURE);
	}
}

int *
allocate_intmem(int len) {
	void *tmp;

	if (len <= 0) {
		fprintf(stderr,
				"ERROR: Cannot allocate memory because len = %i in allocate_intmem().\n",
				len);
		exit(EXIT_FAILURE);
	}

	tmp = (int *) malloc(len * sizeof(int));
	if (tmp != NULL) {
		memset(tmp, 0, len * sizeof(int));
		return (tmp);
	} else {
		fprintf(stderr,
				"ERROR: Cannot allocate memory for array allocate_intmem().\n");
		exit(EXIT_FAILURE);
	}
}

void add_entry_LL(struct addr_tuple *node) {
	if (tablehead == NULL) {
		tablehead = node;
	} else {
		struct addr_tuple *current = tablehead;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = node;
	}
}

struct addr_tuple* add_matched_entry(struct addr_tuple *node,
		struct addr_tuple *matched) {
	struct addr_tuple *ptr = matched;
	struct addr_tuple *temp = NULL;

	temp = (struct addr_tuple*) calloc(1, sizeof(struct addr_tuple));
	memcpy(temp, node, sizeof(struct addr_tuple));
	temp->next = NULL;

	if (ptr == NULL) {
		matched = temp;
	} else {
		while (ptr->next != NULL) {
			ptr = ptr->next;
		}
		ptr->next = temp;
	}
	return matched;
}

// delete entries in the table.
bool delete_entry_LL_IP(struct in_addr ip) {
        bool hasDeletions = false;

        if (tablehead == NULL) {
                return false;
        } else {
                struct addr_tuple *current = tablehead;
                struct addr_tuple *prev = NULL;
                while (current != NULL) {
                        if (ip.s_addr == current->ip_addr.s_addr) {
                                hasDeletions = true;
                                //printf("Removing %s \n", inet_ntoa(current->ip_addr));
                                if (tablehead == current) {
                                        tablehead = tablehead->next;
                                        free(current);
                                        current = tablehead;
                                        continue;
                                } else {
                                        prev->next = current->next;
                                        free(current);
                                        current = prev;
                                }
                        }
                        prev = current;
                        current= current->next;
                }
        }
        //printf("hasDeletiongsd%d\n", hasDeletions);
        //print_entries_LL();
        return hasDeletions;
}

// match the longest prefix
struct addr_tuple *find_entry_LL(struct in_addr *ip, char *tierAddr) {
	if (tablehead == NULL) {
		return NULL;
	} else {
		struct addr_tuple *current = tablehead;
		while (current != NULL) {
			struct in_addr temp;
			memcpy(&temp, ip, sizeof(struct in_addr));

			temp.s_addr = ntohl(temp.s_addr);
			temp.s_addr = ((temp.s_addr >> (32 - current->cidr))
					<< (32 - current->cidr));
			temp.s_addr = htonl(temp.s_addr);

			if ((current->ip_addr.s_addr == temp.s_addr)
					&& (strncmp(tierAddr, current->tier_addr, strlen(tierAddr))
							== 0)) {
				return current;
			}
			current = current->next;
		}
		return NULL;
	}
}

void print_entries_LL() {
	struct addr_tuple *current;

	printf("Tier Address\t\tIP Address\n");
	for (current = tablehead; current != NULL; current = current->next) {
		printf("%s\t\t\t%s/%u\n", current->tier_addr,
				inet_ntoa(current->ip_addr), current->cidr);
	}
}

int buildPayload(uint8_t *data, int msgLen, int checkIndex) {
	struct addr_tuple *current;

	int payloadLen = 3;
	int entries = 0;

	if (msgLen == 2) {
		for (current = tablehead; current != NULL; current = current->next) {

			if (current->isNew == true && checkIndex != current->if_index) {

				uint8_t tierLen = (uint8_t) strlen(current->tier_addr);
				data[payloadLen] = tierLen;
				payloadLen++;

				memcpy(&data[payloadLen], current->tier_addr, tierLen);
				payloadLen = payloadLen + tierLen;

				uint8_t ipLen = (uint8_t) sizeof(struct in_addr);
				data[payloadLen] = ipLen;
				payloadLen++;

				memcpy(&data[payloadLen], &current->ip_addr, ipLen);
				payloadLen = payloadLen + ipLen;

				data[payloadLen] = current->cidr;
				payloadLen++;
				entries++;
			}
		}
	} else {
		for (current = tablehead; current != NULL; current = current->next) {
			uint8_t tierLen = (uint8_t) strlen(current->tier_addr);
			data[payloadLen] = tierLen;
			payloadLen++;

			memcpy(&data[payloadLen], current->tier_addr, tierLen);
			payloadLen = payloadLen + tierLen;

			uint8_t ipLen = (uint8_t) sizeof(struct in_addr);
			data[payloadLen] = ipLen;
			payloadLen++;

			memcpy(&data[payloadLen], &current->ip_addr, ipLen);
			payloadLen = payloadLen + ipLen;

			data[payloadLen] = current->cidr;
			payloadLen++;
			entries++;
		}
	}

	if (entries > 0) {

		data[0] = (uint8_t) MESSAGE_TYPE_ENDNW;
		data[1] = entries;
		data[2] = MESSAGE_TYPE_ENDNW_ADD;
	} else {
		payloadLen = 0;
	}
	return payloadLen;
}

int buildPayloadRemoveAdvts(uint8_t *data, struct addr_tuple *failedIPS) {
        struct addr_tuple *current;
                                
        // To reserve byte for keeping track of the entries and message_type
        int payloadLen = 3;     
        int entries = 0;                
                                        
        for (current=failedIPS; (current != NULL) && (current->isNew == true) ; current = current->next) {
                uint8_t tierLen = (uint8_t)strlen(current->tier_addr);
                data[payloadLen] = tierLen;
                payloadLen++;           
                                                
                memcpy(&data[payloadLen], current->tier_addr, tierLen);
                payloadLen = payloadLen + tierLen;      
                                                
                uint8_t ipLen = (uint8_t)sizeof(struct in_addr);
                data[payloadLen] = ipLen;
                payloadLen++;           
                                        
                memcpy(&data[payloadLen], &current->ip_addr, ipLen );
                payloadLen = payloadLen + ipLen;
                        
                data[payloadLen] = current->cidr;

                current->isNew = false;
                payloadLen++;
                entries++;
        }

        // fill number of entries added to payload, do this only if entries exist.
        if (entries > 0) {
                // Message type 5, used for advertising TierAdd<->Ipaddress entries.
                data[0] = (uint8_t) MESSAGE_TYPE_ENDNW;
                data[1] = entries;
                data[2] = MESSAGE_TYPE_ENDNW_REMOVE;
        } else  {
                payloadLen = 0;
        }
        return payloadLen;
}

char* updateEndTierAddr(char destinationInterfaceIPAddr[]) {

	struct in_addr ip;
	if (inet_pton(AF_INET, destinationInterfaceIPAddr, &ip) == -1) {
		printf("Error: inet_pton() returned error");
	}

	struct addr_tuple *current = tablehead;
	while (current != NULL) {
		struct in_addr temp;
		memcpy(&temp, &ip, sizeof(struct in_addr));
		temp.s_addr = ntohl(temp.s_addr);

		temp.s_addr = (temp.s_addr >> (32 - current->cidr))
				<< (32 - current->cidr);

		temp.s_addr = htonl(temp.s_addr);
		if (temp.s_addr == current->ip_addr.s_addr) {

			//printf("updateEndTierAddr %s Found tier\n", current->tier_addr);
			return current->tier_addr;
		}

		current = current->next;
	}

	return NULL;
}

struct in_addr* getNetworkIP(char destinationInterfaceIPAddr[]) {

	struct in_addr ip;
	if (inet_pton(AF_INET, destinationInterfaceIPAddr, &ip) == -1) {
		printf("Error: inet_pton() returned error");
	}

	struct addr_tuple *current = tablehead;
	while (current != NULL) {
		struct in_addr temp;
		memcpy(&temp, &ip, sizeof(struct in_addr));

		temp.s_addr = ntohl(temp.s_addr);

		temp.s_addr = (temp.s_addr >> (32 - current->cidr))
				<< (32 - current->cidr);

		temp.s_addr = htonl(temp.s_addr);
		if (temp.s_addr == current->ip_addr.s_addr) {

			//printf("getNetworkIP MATCH  port %s\n", current->etherPortName);
			return &current->ip_addr;
		}

		current = current->next;
	}

	return NULL;
}

char* findPortName(struct in_addr *nwIP) {

	struct addr_tuple *current = tablehead;
	while (nwIP != NULL && current != NULL) {
		if (current->ip_addr.s_addr == nwIP->s_addr) {
			//printf("findPortName MATCH  port %s\n", current->etherPortName);
			return current->etherPortName;
		}

		current = current->next;
	}
	return NULL;
}
