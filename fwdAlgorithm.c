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

int packetForwardAlgorithm(char currentTier[], char desTier[]);
boolean optimusForwardAlgorithm(char currentTier[], char desTier[]);
boolean algorithmOptimusG(char currentTier[], char desTier[],
		int tierValueCurrent);

//Functions created for util later to be moved
void getUID(char* curUID,char* currentTier);
void formNextUIDtoTransferInCase3B(char* nextUID,char* curUID,boolean cond);
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
int packetForwardAlgorithm(char myTierAdd[], char desTierAdd[]) {

	//printf("Entering packetForwardAlgorithm \n");

	int returnValue = ERROR;
	boolean checkOFA = false;


	if ((strlen(myTierAdd) == strlen(desTierAdd))
			&& ((strncmp(myTierAdd, desTierAdd, strlen(desTierAdd)) == 0)))
    {

		// Case:1 Current Tier  = Destination Tier
		printf("Case:1 My Tier [%s] = Destination Tier [%s] \n",myTierAdd,desTierAdd);
		boolean checkIfFWDSet =setByTierManually(desTierAdd,true);

		if (checkIfFWDSet == true)
        {
			checkOFA = true;  //to-do need of this variable ?
			fwdSet = SUCCESS; //to-do need of this variable ?
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
            printf("Case:2 Destinaton label is in my neighbour table \n");

			//Forward Packet to the port curresponding  to the Destination label

			//to-do	//Doubt here how the port is found out here  , the code is same as in case 1
			boolean checkFWDSet = setByTierOnly(desTierAdd, true);

            //Checking if the packet was able to successfully sent to the forward port.
			if (checkFWDSet == true)
            {
                checkOFA = true;
				fwdSet = SUCCESS;
				returnValue = SUCCESS;
			}
            else
            {
                printf("Case2:ERROR: Failed to set FWD Tier Address\n");
				fwdSet = ERROR;
				returnValue = ERROR;
			}

		}
		else
        {


			//Goto case3
			/*
			Check 3:
			Is  (My_TV ==  Dest_TV)  && TV == 1
			Process 1: Full mesh topology
              	Check my neighbor table
              	Dest_Label is in my neighbor table
				Forward the MPLR encapsulated packet to the port of access corresponding to the Dest_Label
				Not in my neighbor table – goto Process 2
				Process 2: Linear topology
				              	Check My_UID, Dest_UID
				If (My_UID < Dest_UID )
				Send MNLR packet to TV.(My_UID + 1 )
				Else  - Send MNLR packet to TV.( My_UID -1) }
			*/

			int myTierValue =  getTierVal(myTierAdd);
			int destTierValue = getTierVal(desTierAdd);

			printf("Case:2 [NOT TRUE]  My Tier [%s] = Destination Tier [%s] \n",myTierAdd,desTierAdd);
			if(myTierValue == destTierValue)
            {
            	//checking for case 3
            	if(myTierValue == 1)
            	{
	            	//Case:3 TRUE , Checking cases 3A and 3B
	            	printf("Case:3 [TRUE] myTierValue == destTierValue && myTierValue == 1 \n");
					if (containsTierAddress(desTierAdd) == true)
	                {
	                    //Case:3A True , same as case2
	                    printf("Case:3A [TRUE] Destinaton label is in my neighbor table\n");
	                    
	                    //Forwarding the packet to the destination tier address port
	                    boolean checkFWDSet = setByTierOnly(desTierAdd, true);

	                    //Checking if the packet was successfully sent
						if (checkFWDSet == true)
	                    {
	                       	checkOFA = true;
							fwdSet = 0;
							returnValue = 0;
						}
						else
						{
	                       printf("ERROR: Failed to set FWD Tier Address (Case: 2)\n");
							fwdSet = 1;
							returnValue = 1;
						}

					}
					else
	                {
	                	printf("Case:3A [FALSE] Destinaton label is not in my neighbor table\n");
	                	printf("Case:3B [TRUE] Destinaton label is not in my neighbor table\n");
	                    	                    
	                    //Goto case3B
						char myUID[20];
						char destUID[20];
						char nextUID[20];
						memset(nextUID,'\0',20);

						//Foriming the UIDs from my tier address and destination tier address
						getUID(myUID,myTierAdd);
						getUID(destUID,desTierAdd);
						boolean checkUIDComp = false;

						//Comparing the UIDs of my  tier and destination tier , returns true my tier < destination tier
						checkUIDComp = compareUIDs(myUID,destUID);

						if(checkUIDComp)
						{
							//formNextUID(nextUID,curUID,true); //+1 case
							formNextUIDtoTransferInCase3B(nextUID,myUID,true);
						}
						else
						{
							//formNextUID(nextUID,curUID,true); //-1 case
							formNextUIDtoTransferInCase3B(nextUID,myUID,false);

						}

						//sendPacketTo Next UID
						//to-do modify address with the UID .
						boolean checkFWDSet = setByTierOnly(nextUID, true);
						
						if (checkFWDSet == true)
						{
							returnValue = true;
						} 
						else 
						{
							printf("ERROR: Failed to set FWD Tier Address\n");
							returnValue = false;
						}
	                }
	            }
	            else
	            {
	            	//checking for case 4

	            	printf("Case:4 [TRUE] \n");
	            	char* parentTierAddress;
	            	memset(parentTierAddress,'\0',20);

	            	//trying to get the parent 
	            	parentTierAddress = getParent(myTierAdd,'.');

	            	//sending the packet from the current node to the parent node
	            	boolean checkFWDSet = setByTierOnly(parentTierAddress, true);
						
					if (checkFWDSet == true)
					{
						returnValue = true;
					} 
					else 
					{
						printf("ERROR: Failed to set to the parent Tier Address\n");
						returnValue = false;
					}

	            }
            }
            else
            {
            	//Case:5  My Tier Value !=  Destination Tier Value 
            	printf("Case:5  [TRUE] My Tier Value !=  Destination Tier Value \n");

            	int myTierValue = getTierVal(myTierAdd);
            	int destTierValue = getTierVal(desTierAdd);

            	/*      If Destination UID is a substring of My UID
           				  	// Destination node is my parent/grandparent
            				Forward packet to my parent with the longest substring match 
        				Else
        				{ 
        				    
        				}
           		*/

        		char destUID[20];
				char myUID[20];

				getUID(myUID,myTierAdd);
				getUID(destUID,desTierAdd);

            	//Case:5A
            	if(myTierValue > destTierValue)
            	{
            		boolean check = checkIfDestUIDSubStringUID(destUID,myUID);

            		if(check == true)
            		{	
            			//Forward packet to my parent with the longest substring match
            		}
            		else
            		{
            			//Print table

						//Examine every neighbor table entry
    				
        				//If any Destination UID is a substring of a neighbor node UID
        				    //Forward packet to the neighbor with the longest substring match
        				//Else
        				    //Forward packet to my parent
            		}
            	}
            	else //Case:5B If( My Tier Value < Destination Tier Value)
            	{
            			//If My UID is a substring of Destination UID
       					// 		Destination node is a child /grandchild
            			//		Forward to my child with the longest substring match	
       					//Else
        				//	Examine every neighbor table entry
            			//  If a neighbor node UID is a substring of the Destination UID
                 		//		Forward packet to the matching neighbor
             			//	Else
                 		//		Forward packet to my parent
				}	
            }
        }
    }
    return returnValue;
}

boolean checkIfDestUIDSubStringUID(char* destUID,char* myUID)
{
	return true;
}


/**
 * formNextUIDtoTransferInCase3B
 *
 * Method that forms the next UID to transfer in case of 3B
 *
 * @return boolean
 */

void formNextUIDtoTransferInCase3B(char* nextUID,char* curUID,boolean cond){

	int i = strlen(curUID)-1; 
	int k = 0;
	int savePos = 0;

	while( (i >=0)  && (curUID[i-1] != '.') )
	{
			i--;
	}

	for(;i<strlen(curUID);i++)
	{
		nextUID[k] = curUID[i];
		k++;
	}

	i = strlen(nextUID)-1;
	
	while(nextUID[i-1] != '.'){
		i--;
	}

	savePos = i;

	k = 0;
	char temp[20];
	memset(temp,'\0',20);

	while(i < strlen(nextUID))
	{
		temp[k] = nextUID[i];
		k++;
		i++;
	}

	int tempPart = atoi(temp);

	//case 3B: +1 case , cond = true
	if(cond) 
	{
		tempPart++;
	}
	else
	{
	//case 3B: -1 case , cond = true
		tempPart--;
	}

	//temp = itoa(tempPart);

	sprintf(temp, "%d", tempPart);

	k = 0;
	while(savePos < strlen(temp))
	{
		nextUID[savePos] = temp[k];
		k++;
		savePos++;
	}
	printf("%s : nextUID = %s",__FUNCTION__,nextUID);

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

	while( curUID[ic] != '\0' && destUID[id] != '\0' ){

		k  =0;
		while(curUID[ic]  != '\0' && curUID[ic] != '.'){
			curPart[k++] = curUID[ic];
			ic++;
		}
		curPart[k++] = '\0';

		k  =0;
		while(destUID[id] != '\0' && destUID[id] != '.'){
			destPart[k++] = curUID[ic];
			id++;
		}
		destPart[k++] = '\0';

		int curPartVal = atoi(curPart);
		int destPartVal = atoi(destPart);

		if(curPartVal < destPartVal)
			return true;
		else if(curPartVal > destPartVal)
			return false;
		else{
			//equal case
			//continue
			ic++;
			id++;
			memset(curPart,'\0',20);

			memset(destPart,'\0',20);
		}
    }

	if(destUID[id] != '\0' )
		return false;

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




