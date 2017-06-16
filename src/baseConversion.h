
#ifndef BASECONVERSION_H_
#define BASECONVERSION_H_

#include <stdio.h>
#include<math.h>
#include<stdint.h>
#include <string.h>

int getHexToDecInt(uint8_t a[]);
void toString(char str[], int num);
unsigned int hexToInt(char hex[]);
void hexStringToDecString(char decStr[], char hexStr[]);

int getHexToDecInt(uint8_t intHex[]) {

	int returnVal = 0;

	char temp;

	int IHSize = strlen(intHex);
	int x = 0;
	uint8_t num[IHSize];
	memset(num, '\0', IHSize);
	IHSize--;

	int count = 0;
	int sum = 0;

	while (IHSize >= 0) {

		int temp2 = 0;

		temp = intHex[IHSize];

		if ((temp >= '0') && (temp <= '9')) {

			temp2 = temp - 48;
		} else {

			if ((temp >= 'A') && (temp <= 'F'))
				temp2 = temp - 55;

		}

		sum = sum + (int) pow(16, count) * temp2;
		IHSize--;
		count++;

	}

	return sum;
}

unsigned int hexToInt(char hex[]) {
	int intValue = (int) strtol(&(hex[0]), NULL, 16);
	return intValue;
}

void hexStringToDecString(char decStr[], char hexStr[]) {

	int i = 0;
	char temp[3];
	memset(temp, '\0', 3);
	int destIndex = 0;

	for (; i < strlen(hexStr); i = i + 2) {

		temp[0] = hexStr[i];
		temp[1] = hexStr[i + 1];

		int asciiVal = getHexToDecInt(temp);
		char a = asciiVal;

		decStr[destIndex] = a;

		destIndex++;

	}
	memset(temp, '\0', 3);

}

#endif
