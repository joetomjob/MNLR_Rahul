/*
 * fwdAlgorithm.c
 *
 *  Created on: May 1, 2015
 *      Author: tsp3859
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fwdAlgorithmHelper.h"
#include "boolean.h"
#include "tierUtils.h"

#define SUCCESS 0
#define ERROR 1

char *fwdInterface = NULL;
char *fwdTierAddr = NULL;
int fwdSet = -1;

extern boolean containsTierAddress(char testStr[20]);
extern int getTierValue(char strCheck[]);
extern boolean setByTierPartial(char inTier[20], boolean setFWDFields);
extern boolean setByTierOnly(char inTier[20], boolean setFWDFields);
extern boolean setByTierManually(char inTier[20], boolean setFWDFields);
extern int getUniqueChildIndex(char strCheck[]);
extern void findParntLongst(char* myTierAdd,char* parentTierAdd);
extern void printNeighbourTable();
extern int examineNeighbourTable(char* desTierAdd,char* longstMatchingNgbr);


int packetForwardAlgorithm(char currentTier[], char desTier[]);
boolean optimusForwardAlgorithm(char currentTier[], char desTier[]);
boolean algorithmOptimusG(char currentTier[], char desTier[],
		int tierValueCurrent);

//Functions created for util later to be moved
void getUID(char* curUID,char* currentTier);
void formNextUIDtoTransferInCase3B(char* nextTierAddress,char* currentTierAddress,boolean cond);
boolean checkIfDestUIDSubStringUID(char* destUID,char* myUID);

/**
 * packetForwardAlgorithm(char[],char[])
 *
 * method to perform packet forwarding
 *
 * @param currentTier (char[]) - current tier address
 * @param desTier     (char[]) - destination tier address
 *
 * @return returnValue   (int) - algorithm return value (-1/0/1)
 */
int packetForwardAlgorithm(char myTierAdd[], char desTierAdd[]) 
{

	printf("\n\nEntering packetForwardAlgorithm \n");

	int returnValue = ERROR;
	boolean checkOFA = false;


	// Case:1 If( Destination Label == My Label )
	if ((strlen(myTierAdd) == strlen(desTierAdd))
			&& ((strncmp(myTierAdd, desTierAdd, strlen(desTierAdd)) == 0)))
	{

		// Case:1 Current Tier  = Destination Tier
		printf("Case:1 My Tier [%s] = Destination Tier [%s] \n",myTierAdd,desTierAdd);
		boolean checkIfFWDSet =setByTierManually(desTierAdd,true);

		if (checkIfFWDSet == true)
		{
			printf("Packet send to the ipnode successfully"); 
			checkOFA = true;  
			fwdSet = SUCCESS; 
			returnValue = SUCCESS;
		}
		else
		{
			printf("Case:1:ERROR: Failed to set FWD Tier Address\n");
			fwdSet = ERROR;
			returnValue = ERROR;
		}
	}
	else 
	{
		printf("Case:1 [NOT TRUE]  My Tier [%s] = Destination Tier [%s] \n",myTierAdd,desTierAdd);
		// Check for Case 2 : if Destinaton label is in my neighbour table
		if (containsTierAddress(desTierAdd) == true)
		{
			// Case2 : Destinaton label is in my neighbour table
			printf("[TRUE] Case:2 Destinaton label is in my neighbour table \n");

			//Forward Packet to the port curresponding  to the Destination label
			returnValue = setNextTierToSendPacket(desTierAdd);

		}
		else
		{
 			printf("[FALSE] Case:2 Destinaton label is in my neighbour table \n");
			int myTierValue =  getTierVal(myTierAdd);
			int destTierValue = getTierVal(desTierAdd);
			
			//Case 3 : If ( (My Tier Value ==  Destination Tier Value)  && (Tier Value !=1) ) 

			if( (myTierValue == destTierValue) && (myTierValue != 1))
			{
				printf("Case:3 [TRUE] My Tier Value ==  Destination Tier Value && Tier Value !=1 \n");
				char*  parentTierAddress;
				//	memset(parentTierAddress,'\0',20);
				
				char tempMyTierAddress[20];
				memcpy(tempMyTierAddress,myTierAdd,strlen(myTierAdd)+1);
				printf("Case:3 Trying to get the parent address from myTierAdd=%s \n",tempMyTierAddress);
				//trying to get the parent 
				parentTierAddress = getParent(tempMyTierAddress,'.');
				printf("Case:3 parentTierAddress=%s myTierAdd=%s \n",parentTierAddress,myTierAdd);
				
				returnValue = setNextTierToSendPacket(parentTierAddress);
			}
			else
			{
			//Case4 and 5: 
				printf("Case:3 [FALSE] My Tier Value =  Destination Tier Value && Tier Value !=1 \n");

				char destUID[20];
				char myUID[20];

				getUID(myUID,myTierAdd);
				getUID(destUID,desTierAdd);
				
				char parentTierAdd[20];
				memset(parentTierAdd,'\0',20);
				printNeighbourTable();
				findParntLongst(myTierAdd,parentTierAdd);


				if(myTierValue != destTierValue)
				{
					//case 4
					if(myTierValue > destTierValue)
					{
						printf("\n Entered case 4");
						boolean check = checkIfDestUIDSubStringUID(destUID,myUID);

						if(check == true)
						{	
							printf("\n checkIfDestUIDSubStringUID = TRUE");
							printf("\n Sending packet to parent %s\n", parentTierAdd);
							returnValue = setNextTierToSendPacket(parentTierAdd);
						}
						else
						{
							printf("\n checkIfDestUIDSubStringUID = FALSE");
							char longstMatchingNgbr[20];
							memset(longstMatchingNgbr,'\0',20);

							printNeighbourTable();
							int isDestUIDSubNeigbUID = examineNeighbourTable(desTierAdd,longstMatchingNgbr);
							
							//if not success , set the next node to my parent
							if(isDestUIDSubNeigbUID != SUCCESS){
								printf("\n Destination UID not a substring of any neighbour UID, setting the next address to parent address\n");
								strcpy(longstMatchingNgbr,parentTierAdd);
							}
							else{
								printf("\n Sending the packet to the longest matching neighbour %s",longstMatchingNgbr);
							}
							returnValue = setNextTierToSendPacket(longstMatchingNgbr);
						}
					}
					//case 5
					else 
					{
						printf("\n Entered case 5");	
						boolean check = checkIfDestUIDSubStringUID(destUID,myUID);

						if(check == true)
						{	
							printf("Case 5 : Destination UID substring of my UID ");
							//Forward packet to my child with the longest substring match
							char childTierAdd[20];
							memset(childTierAdd,'\0',20);

							printNeighbourTable();
							findChildLongst(desTierAdd,childTierAdd);
							
							printf("\n checkIfDestUIDSubStringUID = TRUE \n");
							printf("\n Sending the packet to longest child -%s\n",childTierAdd);
							returnValue = setNextTierToSendPacket(childTierAdd);
						}
						else
						{
   							printf("Case 5 : Destination UID not a substring of my UID , sending to longest Matching Neighbour\n");
							char longstMatchingNgbr[20];
							memset(longstMatchingNgbr,'\0',20);
							printNeighbourTable();
							int isDestUIDSubNeigbUID = examineNeighbourTable(desTierAdd,longstMatchingNgbr);
							
							//if not success , set the next node to my parent
							if(isDestUIDSubNeigbUID != SUCCESS){
								strcpy(longstMatchingNgbr,parentTierAdd);
							}
						
							printf("\n checkIfDestUIDSubStringUID = FALSE \n");
	                                                printf("\n Sending the packet to longest neighbour %s \n",longstMatchingNgbr);
	                        returnValue = setNextTierToSendPacket(longstMatchingNgbr);
							
						}

					}	
				}
			}
		}		
	}
	printf("\n\n%s:Exit , returnValue = %d \n",__FUNCTION__,returnValue);
	return returnValue;
}


/**
 * setNextTierToSendPacket(char[])
 *
 * method to set the address of next node to send the packet
 *
 * @param nodeAddress (char[]) - destination  node address
 * @return returnValue   (int) - algorithm return SUCCESS if it is able to
 									else ERROR
 */

int setNextTierToSendPacket(char* nodeAddress)
{
	int returnValue = ERROR;
	//sending the packet from the current node to the next node
	boolean checkFWDSet = setByTierOnly(nodeAddress, true);
	
	if (checkFWDSet == true)
	{
		printf("checkFWDSet == true , setting the fwdSet \n");
		fwdSet = SUCCESS; //to-do need of this variable ?
		returnValue = SUCCESS;
	} 
	else 
	{
		printf("ERROR: Failed to set to the parent Tier Address\n");
		returnValue = ERROR;
		fwdSet = ERROR; //to-do need of this variable ?
	}
	return returnValue;

}


/**
 * checkIfDestUIDSubStringUID(char[],char[])
 *
 * method to check if the destination UID is a substring of my UID
 *
 * @param destUID (char[]) - destination  UID
 * @param myUID   (char[]) - current UID
 *
 * @return returnValue   (boolean) - algorithm return true if it is
 									else false
 */

boolean checkIfDestUIDSubStringUID(char* destUID,char* myUID)
{
	printf("\n myUID = %s destUID=%s",myUID,destUID);

	//3.1 //1

	int pos1 = 0;
	int pos2 = 0;

	while(destUID[pos1] != '\0' && myUID[pos2] != '\0'){

		int destVal = 0;

		while(destUID[pos1] != '\0' && destUID[pos1] != '.'){

		 	destVal = destVal * 10 + destUID[pos1] - '0';
		 	pos1++;
		}
		pos1++;

		int myVal = 0;

		while(myUID[pos2] != '\0' && myUID[pos2] != '.'){

		 	myVal = myVal * 10 + myUID[pos2] - '0';
		 	pos2++;
		}
		pos2++;
		
		printf("\n destVal =%d myVal=%d",destVal,myVal);
		if(destVal != myVal){
			printf("\n False");
			return false;
		}

	}
	printf("\n True");
	return true;

}


/**
 * formNextUIDtoTransferInCase3B
 *
 * Method that forms the next UID to transfer in case of 3B
 *
 * @return boolean
 */

void formNextUIDtoTransferInCase3B(char* nextTierAddress ,char* currentTierAddress,boolean cond  ){

	int i = strlen(currentTierAddress)-1; 
	int k = 0;
	int savePos = 0;
	
	printf("\n formNextUIDtoTransferInCase3B : currentTierAddress = %s condition = %s \n",currentTierAddress,
								(cond == true)?"true":"false");

	//currentTierAddress = 1.1

	strcpy(nextTierAddress,currentTierAddress);

	//nextTierAddress = 1.1

	i = strlen(nextTierAddress)-1;

	//i = 2
	
	while(nextTierAddress[i-1] != '.'){
		i--;
	}

	//i = 2

	savePos = i;

	//SavePos = 2

	k = 0;
	char temp[20];
	memset(temp,'\0',20);

	//temp = "" 
	//i = 2 

	while(i < strlen(nextTierAddress))
	{
		temp[k] = nextTierAddress[i];
		k++;
		i++;
	}

	//temp = "1"

	int tempPart = atoi(temp);

	//tempPart = 1

	//case 3B: +1 case , cond = true
	if(cond) 
	{
		//tempPart = 2
		tempPart++;
	}
	else
	{
	//case 3B: -1 case , cond = true
		tempPart--;
	}

	//temp = itoa(tempPart);

	sprintf(temp, "%d", tempPart);

	//temp = "2"

	k = 0;

	//k= 0 strlen(temp) = 2
	while(k < strlen(temp))
	{
		nextTierAddress[savePos] = temp[k];
		k++;
		savePos++;
	}
	//nextTierAddress = 1.2
	printf("\n%s : nextTierAddress = %s Length=%d\n",__FUNCTION__,nextTierAddress,(int)strlen(nextTierAddress));

}


/**
 * compareUIDs(A,B)
 *
 * Method that compares two given UIDs (A and B) and returns true if A < B and False if A > B
 *
 * @return boolean
 */

boolean compareUIDs(char* curUID,char* destUID) {

	//compare the UID's of both current NOde and the destination node
	int ic =0;
	int id = 0;
	char curPart[20];
	char destPart[20];
	int k;
	printf("\n%s : curUID =%s , destUID =%s",__FUNCTION__,curUID,destUID);
	while( curUID[ic] != '\0' && destUID[id] != '\0' ){

		k  =0;
		while(curUID[ic]  != '\0' && curUID[ic] != '.'){
			curPart[k++] = curUID[ic];
			ic++;
		}
		curPart[k++] = '\0';

		k  =0;
		while(destUID[id] != '\0' && destUID[id] != '.'){
			destPart[k++] = destUID[id];
			id++;
		}
		destPart[k++] = '\0';

		int curPartVal = atoi(curPart);
		int destPartVal = atoi(destPart);
		printf("\n %s: comparing UIDS curPartVal=%d destPartVal=%d ",__FUNCTION__,curPartVal,destPartVal);

		if(curPartVal < destPartVal){
			printf("\n%s : curPartVal < destPartVal",__FUNCTION__);
			return true;
		}
		else if(curPartVal > destPartVal){
			printf("\n%s : curPartVal < destPartVal",__FUNCTION__);
			return false;
		}
		else{
			//equal case
			//continue
			ic++;
			id++;
			memset(curPart,'\0',20);

			memset(destPart,'\0',20);
		}
	}

	if(destUID[id] != '\0' ){
		printf("%s : destUID is still left",__FUNCTION__);
		return false;	
	}
	
	printf("%s: Some error!! ",__FUNCTION__);
	return true; //Should never come to this case as destID is always > curID length

}

/**
 * getUID()
 *
 * method to get the UID from the current Tier address and store it in curUID.
 *
 * @return none
 */

void getUID(char* curUID,char* currentTier){

	int i = 0;
	////Truncate and store the truncated part as the Tier value the UID's of both the current and the destination

	while(currentTier[i] != '.'){
		i++;

	}
	i = i+1;

	int k = 0;

	while(currentTier[i] != '\0'){
			curUID[k] = currentTier[i];
			i++;
			k++;

	}
	curUID[k] = '\0';
}


/**
 * getTierVal()
 *
 * method to get the Tier value from the passed Tier address and return the same.
 *
 * @return int [ the tier it belongs to]
 */

int getTierVal(char* tierAdd)
{
	int i = 0;
	char tierValInString[20];
	int tier = -1;
	memset(tierValInString,'\0',20);
	
	while(tierAdd[i] != '.')
	{
		tierValInString[i] = tierAdd[i];
		i++;
	}

	tier = atoi(tierValInString);
	return tier;
}

/**
 * isFWDFieldsSet()
 *
 * method to check whether forwarding fields are set or not
 *
 * @return fwdSet  (int)- forwarding field status
 */
int isFWDFieldsSet() {

	return fwdSet;
}


