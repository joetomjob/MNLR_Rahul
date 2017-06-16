/*
 * helloList.h
 *
 *  Created on: Mar 29, 2015
 *  Updated on: Apr 02, 2015
 *      Author: tsp3859
 */

#ifndef HELLOLIST_H_
#define HELLOLIST_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#include "boolean.h"
#include "fwdAlgorithmHelper.h"

void update(char inTier[20], char inPort[20]);
int find(char inTier[20], char inPort[20]);
boolean containsTierAddress(char testStr[20]);
boolean setByTierPartial(char inTier[20], boolean setFWDFields);
boolean setByTierOnly(char inTier[20], boolean setFWDFields);
boolean setByTierManually(char inTier[20], boolean setFWDFields);

struct nodeHL {
	char tier[20];          // tier value
	char port[20];  	    // received interface
	time_t lastUpdate;      // last updated time
	struct nodeHL *next;      // next node
}*headHL;


/**
 * append()
 *
 * method to add node after previous node, called by insert()
 *
 * @param inTier (char[]) - tier value
 * @param inPort (char[]) - interface value
 *
 */
void append(char inTier[20], char inPort[20]) {

	struct nodeHL *temp, *right;
	temp = (struct nodeHL *) malloc(sizeof(struct nodeHL));

	strcpy(temp->tier, inTier);
	strcpy(temp->port, inPort);
	temp->lastUpdate = time(0);

	right = (struct nodeHL *) headHL;
	while (right->next != NULL)
		right = right->next;
	right->next = temp;
	right = temp;
	right->next = NULL;
	//printf("TEST: Node appended successfully %s\n", temp->tier);
}

/**
 * add()
 *
 * method to add node after previous node, called by insert()
 *
 * @param inTier (char[]) - tier value
 * @param inPort (char[]) - interface value
 *
 */
void add(char inTier[20], char inPort[20]) {

	struct nodeHL *temp;
	temp = (struct nodeHL *) malloc(sizeof(struct nodeHL));
	strcpy(temp->tier, inTier);
	strcpy(temp->port, inPort);
	temp->lastUpdate = time(0);

	if (headHL == NULL) {
		headHL = temp;
		headHL->next = NULL;
	} else {
		temp->next = headHL;
		headHL = temp;
	}
	//printf("TEST: Node added successfully %s\n", temp->tier);
}

/**
 * insert()
 *
 * method to add node into a list (duplicate entry-safe)
 *
 * @param inTier (char[]) - tier value
 * @param inPort (char[]) - interface value
 *
 * @return isEntryNew
 */
int insert(char inTier[20], char inPort[20]) {

	struct nodeHL *temp;
	temp = headHL;

	int isEntryNew = 0;

	if (temp == NULL) {

		add(inTier, inPort);
		isEntryNew = 1;

	} else {

		int checkNode = find(inTier, inPort);

		if (checkNode == 1) {

			append(inTier, inPort);
			isEntryNew = 1;

		} else {

			update(inTier, inPort);
		}

	}

	return isEntryNew;
}

/**
 * find()
 *
 * method to check whether a node is present or not in list
 *
 * @param inTier (char[]) - tier value
 * @param inPort (char[]) - interface value
 *
 * @return returnVal (int) - 0 for present otherwise 1
 */
int find(char inTier[20], char inPort[20]) {

	int returnVal = 1;

	struct nodeHL *fNode = headHL;

	// traverse the list
	// testing
	while (fNode != NULL) {
		//while (fNode->next != NULL) {

		// Target Node
		// Length Check
		// Value check

		if (strlen(fNode->tier) == strlen(inTier)) {
			if (strncmp(fNode->tier, inTier, strlen(inTier)) == 0) {

				if (strlen(fNode->port) == strlen(inPort)) {

					//}
					if (strncmp(fNode->port, inPort, strlen(inPort)) == 0) {

						returnVal = 0;
						break;
					}
				}

			}

		}

		fNode = fNode->next;
	}

	return returnVal;
}

/**
 * update()
 *
 * method to update the timer information of a node
 *
 * @param inTier (char[]) - tier value
 * @param inPort (char[]) - interface value
 */
void update(char inTier[20], char inPort[20]) {

	// to be updated
	struct nodeHL *uNode = headHL;

	// traverse the list
	// testing
	while (uNode != NULL) {

		//while (uNode->next != NULL) {

		// Target Node
		// Length Check
		// Value check
		// Update timer

		if (strlen(uNode->tier) == strlen(inTier)) {
			if (strncmp(uNode->tier, inTier, strlen(inTier)) == 0) {

				if (strlen(uNode->port) == strlen(inPort)) {

					//}
					if (strncmp(uNode->tier, inTier, strlen(inTier)) == 0) {

						uNode->lastUpdate = time(0);
						//printf("TEST: lastUpdate updated %s\n", uNode->tier);
						break;
					}
				}

			}

		}
		uNode = uNode->next;

	}

}

/**
 * delete()
 *
 * method to delete node from neighbor based on timeout mechanism
 *
 * @return status (int) - method return value
 */
int delete() {

	struct nodeHL *deletedTierAddr = NULL;
	struct nodeHL *temp, *prev;
	temp = headHL;

	while (temp != NULL) {

		time_t currentTime = time(0);
		double delTimeDiff = difftime(currentTime, temp->lastUpdate);
		//printf("TEST: delTimeDiff: %f \n", delTimeDiff);

		// If last updated local time is more than desired time
		if (delTimeDiff >= 30) {
			//printf("TEST: Inside Time diff delete block (>30)\n");

			// if node to be removed is head
			if (temp == headHL) {
				//printf("TEST: Head node removed value was %s\n", temp->tier);
				headHL = temp->next;

				//free(temp);
				//return 1;
			} else {
				prev->next = temp->next;
				//printf("TEST: other node removed value was %s\n", temp->tier);
				//free(temp);
				//return 1;
			}
		}
		prev = temp;
		temp = temp->next;

	}
	return 1;
}

/**
 * displayNeighbor()
 *
 * method to print neighbour list entries
 *
 * @param inTier (char[]) - tier value
 *
 */
//void displayNeighbor(struct node *r) {
void displayNeighbor() {

	struct nodeHL *r;
	r = headHL;
	if (r == NULL) {
		//printf("No Neighbors\n");
		return;
	}
	//printf("My Neighbors\n");
	int i = 1;
	while (r != NULL) {
		//printf("Tier Address %d - Length %zd : %s : Port: %s\n", i,
				//strlen(r->tier), r->tier, r->port);
		i = i + 1;
		r = r->next;
	}
	//printf("\n");
}

/**
 * count()
 *
 * method to count number of neighbours
 *
 * @return localCount (int) - count of neighbour nodes
 */
int count() {
	struct nodeHL *n;
	int localCount = 0;
	n = headHL;
	while (n != NULL) {
		n = n->next;
		localCount++;
	}
	return localCount;
}

/**
 * contains(char[])
 *
 * whether there is a tier address in neighbor table or not
 *
 * @return true or false
 */
boolean containsTierAddress(char testStr[20]) {

	//printf("Inside containsTierAddress - helloList.h \n");
	boolean check = false;

	struct nodeHL *fNode = headHL;

	if (fNode == NULL) {

		printf("ERROR: Neighbor List is empty (Isolated Node)\n");
		printf("TEST: Before return check %d \n", check);
		return check;
	}

	// traverse the list
	// testing
	while (fNode != NULL) {
		//while (fNode->next != NULL) {

		if ((strlen(fNode->tier) == strlen(testStr))
				&& ((strncmp(fNode->tier, testStr, strlen(testStr)) == 0))) {
			check = true;
			break;

		} else {
			fNode = fNode->next;
		}

	}
	//printf("TEST: Before return check %d \n", check);
	return check;
}

/**
 * setByTierPartial(char[],boolean)
 *
 * If there is a tier address partial match, it will set forwarding fields
 * Should only be used in an uplink scenario
 *
 * @param inTier       - tier address (partial) to be probed
 * @param setFWDFields - if true will set forwarding fields
 *
 * @return true or false
 */
boolean setByTierPartial(char inTier[20], boolean setFWDFields) {

	//printf("Inside setByTierPartial - helloList.h \n");
	boolean returnVal = false;

	struct nodeHL *fNode = headHL;

	if (fNode == NULL) {

		printf("ERROR: Failed to set FWD Tier Address (Isolated Node)\n");
		return returnVal;
	}

	// traverse the list
	// testing
	while (fNode != NULL) {
		//while (fNode->next != NULL) {

		// Target Node
		// Length Check
		// Value check

		//	if (strlen(fNode->tier) == strlen(inTier)) {
		if (strncmp(fNode->tier, inTier, strlen(inTier)) == 0) {

			if (setFWDFields == true) {

				fwdTierAddr = (char *) malloc(20);
				memset(fwdTierAddr, '\0', 20);
				strcpy(fwdTierAddr, fNode->tier);

				// set fwd tier port
				fwdInterface = (char *) malloc(20);
				memset(fwdInterface, '\0', 20);
				strcpy(fwdInterface, fNode->port);

				returnVal = true;

			}

			returnVal = true;
			break;

		}

		//	}

		fNode = fNode->next;
	}

	return returnVal;
}

/**
 * setByTierOnly(char[],boolean)
 *
 * If there is a tier address complete match, it will set forwarding fields
 * smart method detects error before forwarding packet
 *
 * @param inTier       - tier address to be probed
 * @param setFWDFields - if true will set forwarding fields
 *
 * @return true or false
 */
boolean setByTierOnly(char inTier[20], boolean setFWDFields) {

	//printf("Inside setByTierOnly - helloList.h \n");
	boolean returnVal = false;

	struct nodeHL *fNode = headHL;

	if (fNode == NULL) {

		printf("ERROR: Failed to set FWD Tier Address (Isolated Node)\n");
		return returnVal;
	}

	// traverse the list
	// testing
	while (fNode != NULL) {
		//while (fNode->next != NULL) {

		// Target Node
		// Length Check
		// Value check

		if ((strlen(fNode->tier) == strlen(inTier))
				&& ((strncmp(fNode->tier, inTier, strlen(inTier)) == 0))) {

			if (setFWDFields == true) {

				fwdTierAddr = (char *) malloc(20);
				memset(fwdTierAddr, '\0', 20);
				strcpy(fwdTierAddr, fNode->tier);

				// set fwd tier port
				fwdInterface = (char *) malloc(20);
				memset(fwdInterface, '\0', 20);
				strcpy(fwdInterface, fNode->port);

				returnVal = true;

			}

			returnVal = true;
			break;

			//	}

		}

		fNode = fNode->next;
	}

	return returnVal;
}

// TODO
/**
 * setByTierManually(char[],boolean)
 *
 * a Manual method to set fwd fields
 * does not perform pre-check
 * AVOID IT
 *
 * Currently Used by:
 *  1. Forwarding Algorithm - Case A
 * @param inTier       - tier address to be set directly
 * @param setFWDFields - if true will set forwarding fields
 *
 * @return true or false
 */
boolean setByTierManually(char inTier[20], boolean setFWDFields) {

	//printf("Inside setByTierManually - helloList.h \n");
	boolean returnVal = false;

	if (setFWDFields == true) {

		// set fwd tier address
		fwdTierAddr = (char *) malloc(20);
		memset(fwdTierAddr, '\0', 20);
		strcpy(fwdTierAddr, inTier);

		// set fwd tier port
		fwdInterface = (char *) malloc(20);
		memset(fwdInterface, '\0', 20);
		strcpy(fwdInterface, "xxx");

		returnVal = true;

	}

	return returnVal;
}



/**
 * printNeighbourTable()
 *
 * print the neighbour table
 *
 * @return void
 */

void printNeighbourTable() {

	struct nodeHL *fNode = headHL;
	char* temp;
	if (fNode == NULL) {
		printf("ERROR: Neighbor List is empty (Isolated Node)\n");
		return;
	}
	// traverse the list
	// testing
	printf("\n*************** Neighbor Table *************");
	while (fNode != NULL) {
		temp  = fNode->tier;		
		printf("\n ------- %s --------",temp);
		fNode = fNode->next;
	}
	return;
}

/**
 * findParntLongst(char[],char[])
 *
 * return the longest matching parent adress in the table 
 *
 * @return void

 */
 void findParntLongst(char* myTierAdd,char* parentTierAdd) 
 {
	struct nodeHL *fNode = headHL;
	char* temp;
	if (fNode == NULL) {
		printf("ERROR: Neighbor List is empty (Isolated Node)\n");
		return;
	}
	
	//initializing the longest matching length to 0
	int longestMtchLength = 0;

	while (fNode != NULL) {
		temp  = fNode->tier;		
		
		if(strlen(myTierAdd) > strlen(temp)){

			int tempLen = findMatchedTeirAddrLength(myTierAdd,temp);
			if(tempLen > longestMtchLength){
				longestMtchLength = tempLen;
				strcpy(parentTierAdd, temp);
			}
		}
		fNode = fNode->next;
	}
	return;
 }



/**
 * findChildLongst(char[],char[])
 *
 * return the longest matching child adress in the table 
 *
 * @return void

 */

 void findChildLongst(char* desTierAdd,char* childTierAdd)
 {
	struct nodeHL *fNode = headHL;
	char* temp;
	if (fNode == NULL) {
		printf("ERROR: Neighbor List is empty (Isolated Node)\n");
		return;
	}
	
	//initializing the longest matching length to 0
	int longestMtchLength = 0;

	while (fNode != NULL) {
		temp  = fNode->tier;		
		printf("\n findChildLongst : Current Neighbour = %s \n",temp);
		if(strlen(temp) <= strlen(desTierAdd)){

			int tempLen = findMatchedTeirAddrLength(desTierAdd,temp);
			if(tempLen > longestMtchLength){
				longestMtchLength = tempLen;
				strcpy(childTierAdd, temp);
			}
		}
		fNode = fNode->next;
	}

	printf("\n findChildLongst : Result = %s \n",childTierAdd);
	return;
 }

/**
 * examineNeighbourTable(char[])
 *
 * return the longest matching adress in the table with the destination address
 *

 * @return void

 */
 int examineNeighbourTable(char* desTierAdd,char* longstMatchingNgbr) 
 {
 	int retVal = 1; //ERROR / FAILURE
	struct nodeHL *fNode = headHL;
	char* temp;
	if (fNode == NULL) {
		printf("ERROR: Neighbor List is empty (Isolated Node)\n");
		return;
	}
	
	//initializing the longest matching length to 0
	int longestMtchLength = 0;
	int tempLen = 0; 

	while (fNode != NULL) {
		temp  = fNode->tier;	
		printf("\n%s temp->%s desTierAdd-->%s",__FUNCTION__,temp,desTierAdd);
		printf("\n %s Checking the match for %s in desTierAdd=%s",__FUNCTION__,temp,desTierAdd);	
		tempLen = findMatchedTeirAddrLength(desTierAdd,temp);
		printf("\n %s Matched Length = %d",__FUNCTION__,tempLen);	
		if(tempLen > longestMtchLength){
			longestMtchLength = tempLen;
			strcpy(longstMatchingNgbr, temp);
			retVal = 0;
		}
		fNode = fNode->next;
	}

	return retVal;
 }


/**
 * findMatchedTeirAddrLength(char[],char[])
 *
 * find the matched length of two tier values
 *
 * @return length (int)
 */



 int findMatchedTeirAddrLength(char* add1 , char* add2){

 	int matchedLength = 0;
 	int posAdd1 = 0;
 	int posAdd2 = 0;
 	int val1 = 0;
 	int val2 = 0;
 	
 	printf("\n %s Enter : add1 = %s add2 = %s \n",__FUNCTION__,add1,add2);   
	while(add1[posAdd1++] != '.');
 	while(add2[posAdd2++] != '.');
		
 	// printf("\n posAdd1 = %d posAdd2 = %d \n",posAdd1,posAdd2);  

 	while( (add1[posAdd1] != '\0') && (add2[posAdd2] != '\0'))
 	{
		
 		// printf("\n posAdd1 = %d  \n",posAdd1);  
 		// printf("\n add1[posAdd1] =%c",add1[posAdd1]);
		while( (add1[posAdd1] != '.') && (add1[posAdd1] != '\0'))
 		{

 			// printf("\n add1[posAdd1] = %c  posAdd1=%d\n",add1[posAdd1],posAdd1);   
			val1 = (val1 * 10 )+  add1[posAdd1] - '0' ;
			posAdd1++;
 		}
		posAdd1++;

		
	 	// printf("\n posAdd2 = %d \n",posAdd2);  
 		// printf("\n add2[posAdd2] =%c",add2[posAdd2]);
 		while( (add2[posAdd2] != '.') && (add2[posAdd2] != '\0'))
 		{
 			// printf("\n add2[posAdd2] = %c  posAdd2=%d\n",add2[posAdd2],posAdd2);   
 			val2 = (val2 * 10 )+  add2[posAdd2] - '0' ;
			posAdd2++;
 		}
		posAdd2++;

 		// printf("\nval1 = %d val2 = %d\n",val1,val2);

 		// printf("\n  add1[posAdd1] =%c add2[posAdd2] =%c",add1[posAdd1],add2[posAdd2]);

 		if(val1 == val2)
 		{
 			matchedLength++;
 		}
 		else
 		{
 			break;
 		}
		
		if(add1[posAdd1] == '\0' || add2[posAdd2] == '\0')
			break;

 		val1 = val2 = 0;

 	} 
 	printf("\n %s :Exit- Matched Length = %d",__FUNCTION__,matchedLength);
 	return matchedLength;
 }




#endif

