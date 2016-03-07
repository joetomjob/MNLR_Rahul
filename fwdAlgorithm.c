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
boolean algorithmG2(char currentTier[], char desTier[]);
boolean algorithmG3(char currentTier[], char desTier[]);
boolean algorithmG4(char currentTier[], char desTier[]);

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

int packetForwardAlgorithm(char currentTier[], char desTier[]) {

	//printf("Entering packetForwardAlgorithm \n");

	int returnValue = -1;
	boolean checkOFA = false;
    
	if ((strlen(currentTier) == strlen(desTier))
			&& ((strncmp(currentTier, desTier, strlen(currentTier)) == 0))) {

		// Case: A

		//printf("Case: A \n");
		boolean checkFWDSet =setByTierManually(desTier,true);

		if (checkFWDSet == true) {

			checkOFA = true;
			fwdSet = 0;
			returnValue = 0;

		} else {

			//printf("ERROR: Failed to set FWD Tier Address (Case: A)\n");
			fwdSet = 1;
			returnValue = 1;
		}

	} else {

		if (containsTierAddress(desTier) == true) {

			// Case: B

			//printf("Case: B \n");

			//printf("TEST: containsTierAddress - return value - true \n");
			boolean checkFWDSet = setByTierOnly(desTier, true);

			if (checkFWDSet == true) {

				checkOFA = true;
				fwdSet = 0;
				returnValue = 0;

			} else {

				//printf("ERROR: Failed to set FWD Tier Address (Case: B)\n");
				fwdSet = 1;
				returnValue = 1;
			}

		} else {

			// Case: No A, No B

			checkOFA = optimusForwardAlgorithm(currentTier, desTier);
		}

	}

	if (checkOFA == false) {
		returnValue = 1;
	} else {

		returnValue = 0;
		fwdSet = 0;

	}

	return returnValue;
}

/**
 * optimusForwardAlgorithm(char[],char[])
 *
 * method to perform optimal forwarding of a packet
 *
 * @param currentTier (char[]) - current tier address
 * @param desTier     (char[]) - destination tier address
 *
 * @return returnVal (boolean) - return status
 */

boolean optimusForwardAlgorithm(char currentTier[], char desTier[]) {

	//printf("Entering optimusForwardAlgorithm \n");

	boolean returnVal = true;
	boolean checkSubAlgoG = true;

	int tierValueC = getTierValue(currentTier);
	int tierValueD = getTierValue(desTier);

	if (tierValueC == tierValueD) {

		if (tierValueC == 1) {

			checkSubAlgoG = algorithmOptimusG(currentTier, desTier, tierValueC);

		} else {

			// Case: C

			//printf("Case: C \n");
			char parentC[20];
			memset(parentC, '\0', 20);
			strcpy(parentC, currentTier);
            
            strcpy(parentC, getParent(parentC, '.'));

			boolean checkFWDSet = setByTierOnly(parentC, true);

			if (checkFWDSet == true) {

				fwdSet = 0;
				returnVal = true;

			} else {

				printf("ERROR: Failed to set FWD Tier Address (Case: C)\n");
				fwdSet = 1;
				returnVal = false;
			}

		}
	} else {

		if (tierValueC == 1) {

			// Case: F, G, H
			checkSubAlgoG = algorithmG2(currentTier, desTier);

		} else {

			if (tierValueC < tierValueD) {

				// Case: I, J
				checkSubAlgoG = algorithmG3(currentTier, desTier);

			} else {

				// Case: K, L
				checkSubAlgoG = algorithmG4(currentTier, desTier);

			}

		}

	}

	if (checkSubAlgoG == false) {

		fwdSet = 1;
		returnVal = false;

	} else {

		fwdSet = 0;
		returnVal = true;

	}

	return returnVal;
}


boolean algorithmOptimusG(char currentTier[], char desTier[],
		int tierValueCurrent) {

	//printf("Entering Algorithm OptimusG \n");

	boolean returnVal = false;

	int uniqueChildC = getUniqueChildIndex(currentTier);
	int uniqueChildD = getUniqueChildIndex(desTier);

	if (uniqueChildC < uniqueChildD) {

		// Case: D

		//printf("Case: D \n");
		uniqueChildC = uniqueChildC + 1;

	} else {

		// Case: E

		//printf("Case: E \n");
		uniqueChildC = uniqueChildC - 1;

	}

	int tierValueCSize = floor(log10(abs(tierValueCurrent))) + 1;
	char subStrTierC[tierValueCSize];
	toString(subStrTierC, tierValueCurrent);

	int uniqueChildCSize = floor(log10(abs(uniqueChildC))) + 1;
	char subStrC[uniqueChildCSize];
	toString(subStrC, uniqueChildC);

	char trunkTier[20];
	strcpy(trunkTier, subStrTierC);
	strcat(trunkTier, ".");
	strcat(trunkTier, subStrC);

	boolean checkFWDSet = setByTierOnly(trunkTier, true);

	if (checkFWDSet == true) {
		returnVal = true;

	} else {

		printf("ERROR: Failed to set FWD Tier Address (Case: D-E)\n");
		returnVal = false;
	}

	return returnVal;
}


boolean algorithmG2(char currentTier[], char desTier[]) {

	//printf("Entering Algorithm G2 \n");

	boolean returnVal = false;

	//int tierCDot2 = -1;







	char tierCByDot2[10];
	memset(tierCByDot2, '\0', 10);
	getStringByDot(currentTier, 2, tierCByDot2);
	toInt(tierCByDot2, strlen(tierCByDot2));

	char tierDByDot2[10];
	memset(tierDByDot2, '\0', 10);
	getStringByDot(desTier, 2, tierDByDot2);
	toInt(tierDByDot2, strlen(tierDByDot2));

	char masterTierAddr[20];
	memset(masterTierAddr, '\0', 20);

	if (tierCByDot2 == tierDByDot2) {

		// Case: F

		char tempMasterVal[10];
		memset(tempMasterVal, '\0', 10);

		toString(tempMasterVal, 2);
		strcpy(masterTierAddr, tempMasterVal);

		strcat(masterTierAddr, ".");

		memset(tempMasterVal, '\0', 10);
		getStringByDot(desTier, 1, tempMasterVal);

		strcat(masterTierAddr, tempMasterVal);
		strcat(masterTierAddr, ".");

		memset(tempMasterVal, '\0', 10);
		getStringByDot(desTier, 2, tempMasterVal);
		strcat(masterTierAddr, tempMasterVal);
		memset(tempMasterVal, '\0', 10);

	} else {

		char tempMasterVal[10];
		memset(tempMasterVal, '\0', 10);

		toString(tempMasterVal, 1);
		strcpy(masterTierAddr, tempMasterVal);

		strcat(masterTierAddr, ".");

		memset(tempMasterVal, '\0', 10);
		getStringByDot(desTier, 1, tempMasterVal);

		int tempInt = toInt(tempMasterVal, strlen(tempMasterVal));

		if (tierCByDot2 < tierDByDot2) {

			// Case: G

			tempInt = tempInt + 1;
		} else {

			// Case: H

			tempInt = tempInt - 1;

		}

		memset(tempMasterVal, '\0', 10);
		toString(tempMasterVal, tempInt);

		strcat(masterTierAddr, tempMasterVal);
		memset(tempMasterVal, '\0', 10);
	}

	boolean checkFWDSet = setByTierOnly(masterTierAddr, true);

	if (checkFWDSet == true) {
		returnVal = true;

	} else {

		printf("ERROR: Failed to set FWD Tier Address (Case: F-G-H)\n");
		returnVal = false;
	}

	memset(masterTierAddr, '\0', 20);
	return returnVal;

}


boolean algorithmG3(char currentTier[], char desTier[]) {

	//printf("Entering Algorithm G3 \n");

	boolean returnVal = false;
	char masterTierAddr[20];
	memset(masterTierAddr, '\0', 20);

	// insert code below

	char parentCExT[20];
	memset(parentCExT, '\0', 20);
	strcpy(parentCExT, currentTier);
	int firstDotPosC = getFirstDotPosition(currentTier);
	getParentExcludeT(parentCExT, '.', firstDotPosC + 1);

	char parentDExT[20];
	memset(parentDExT, '\0', 20);
	strcpy(parentDExT, desTier);
	int firstDotPosD = getFirstDotPosition(desTier);
	getParentExcludeT(parentDExT, '.', firstDotPosD + 1);

	int checkMatch = findOccurenceAt(parentDExT, parentCExT);

	if (checkMatch == 0) {

		// Case: I

		int tierVal = getTierValue(currentTier);

		char tempMasterVal[10];
		memset(tempMasterVal, '\0', 10);

		toString(tempMasterVal, tierVal + 1);
		strcpy(masterTierAddr, tempMasterVal);
		memset(tempMasterVal, '\0', 10);

		int i = 0;
		for (; i < tierVal + 1; i++) {

			memset(tempMasterVal, '\0', 10);
			strcat(masterTierAddr, ".");
			getStringByDot(desTier, i + 1, tempMasterVal);
			strcat(masterTierAddr, tempMasterVal);

		}

	} else {

		// Case: J
		// D1 is for tier C and D2 is for tier (C-1)

		int tierVal = getTierValue(currentTier);

		char tempMasterVal[10];
		memset(tempMasterVal, '\0', 10);

		char superParentOfD1[20];
		memset(superParentOfD1, '\0', 20);
		char superParentOfD2[20];
		memset(superParentOfD2, '\0', 20);

		toString(tempMasterVal, tierVal);
		strcpy(superParentOfD1, tempMasterVal);
		memset(tempMasterVal, '\0', 10);

		toString(tempMasterVal, tierVal - 1);
		strcpy(superParentOfD2, tempMasterVal);
		memset(tempMasterVal, '\0', 10);

		int i = 0;
		for (; i < tierVal; i++) {

			memset(tempMasterVal, '\0', 10);
			strcat(superParentOfD1, ".");
			getStringByDot(desTier, i + 1, tempMasterVal);
			strcat(superParentOfD1, tempMasterVal);

			if (i < tierVal - 1) {

				memset(tempMasterVal, '\0', 10);
				strcat(superParentOfD2, ".");
				getStringByDot(desTier, i + 1, tempMasterVal);
				strcat(superParentOfD2, tempMasterVal);
			}

		}

		if (containsTierAddress(superParentOfD1) == true) {

			// Case: J-1A

			strcpy(masterTierAddr, superParentOfD1);

		} else {

			if (containsTierAddress(superParentOfD2) == true) {

				// Case: J-1B

				strcpy(masterTierAddr, superParentOfD2);

			} else {

				// Case: J-2

				char parentOfC[20];
				memset(parentOfC, '\0', 20);
				strcpy(parentOfC, currentTier);

				strcpy(parentOfC, getParent(parentOfC, '.'));
				strcpy(masterTierAddr, parentOfC);

				memset(parentOfC, '\0', 20);
			}

		}

		memset(superParentOfD1, '\0', 20);
		memset(superParentOfD2, '\0', 20);

	}

	// insert code above

	boolean checkFWDSet = setByTierOnly(masterTierAddr, true);

	if (checkFWDSet == true) {
		returnVal = true;

	} else {

		printf("ERROR: Failed to set FWD Tier Address (Case: I-J)\n");
		returnVal = false;
	}

	memset(masterTierAddr, '\0', 20);
	return returnVal;

}


boolean algorithmG4(char currentTier[], char desTier[]) {

	//printf("Entering Algorithm G4 \n");

	boolean returnVal = false;

	char masterTierAddr[20];
	memset(masterTierAddr, '\0', 20);

	// insert code below

	boolean tryToSet = true;

	char parentCExT[20];
	memset(parentCExT, '\0', 20);
	strcpy(parentCExT, currentTier);
	int firstDotPosC = getFirstDotPosition(currentTier);
	getParentExcludeT(parentCExT, '.', firstDotPosC + 1);

	char parentDExT[20];
	memset(parentDExT, '\0', 20);
	strcpy(parentDExT, desTier);
	int firstDotPosD = getFirstDotPosition(desTier);
	getParentExcludeT(parentDExT, '.', firstDotPosD + 1);

	int checkMatch = findOccurenceAt(parentCExT, parentDExT);

	if (checkMatch == 0) {

		// Case: K

		char parentforC[20];
		memset(parentforC, '\0', 20);
		strcpy(parentforC, currentTier);

		strcpy(parentforC, getParent(parentforC, '.'));
		strcpy(masterTierAddr, parentforC);

		memset(parentforC, '\0', 20);
		tryToSet = true;

	} else {

		// Case: L
		// D1 is for tier (D+1)

		int tierVal = getTierValue(desTier);

		char tempMasterVal[10];
		memset(tempMasterVal, '\0', 10);

		char superParentOfD1[20];
		memset(superParentOfD1, '\0', 20);

		toString(tempMasterVal, tierVal + 1);
		strcpy(superParentOfD1, tempMasterVal);
		memset(tempMasterVal, '\0', 10);

		strcat(superParentOfD1, ".");

		char parentDExcTU[20];
		memset(parentDExcTU, '\0', 20);
		strcpy(parentDExcTU, desTier);

		int dFDot = getFirstDotPosition(desTier);
		int dLDot = getLastDotPosition(desTier);

		getParentExcludeTU(parentDExcTU, '.', dFDot + 1, dLDot - 1);
		strcat(superParentOfD1, parentDExcTU);
		strcat(superParentOfD1, ".");

		boolean checkMatchOpt = setByTierPartial(superParentOfD1, true);

		if (checkMatchOpt == true) {

			// Case: L-1

			strcpy(masterTierAddr, superParentOfD1);

			memset(parentDExcTU, '\0', 20);
			memset(superParentOfD1, '\0', 20);

			returnVal = true;
			tryToSet = false;

		} else {

			// Case: L-2 (Similar to Case: K)

			char parentforC[20];
			memset(parentforC, '\0', 20);
			strcpy(parentforC, currentTier);

			strcpy(parentforC, getParent(parentforC, '.'));
			strcpy(masterTierAddr, parentforC);

			memset(parentforC, '\0', 20);
			tryToSet = true;
		}

	}

	// insert code above

	if (tryToSet == true) {
		boolean checkFWDSet = setByTierOnly(masterTierAddr, true);

		if (checkFWDSet == true) {
			returnVal = true;

		} else {

			printf("ERROR: Failed to set FWD Tier Address (Case: K-L)\n");
			returnVal = false;
		}

	}
	memset(masterTierAddr, '\0', 20);
	return returnVal;

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
