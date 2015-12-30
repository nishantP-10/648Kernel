/*
 * calculator app
 * author: Mengye Gong
 */


#include <stdio.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <stdlib.h>
#include <string.h>
#include <linux/errno.h>
#define floatBits 12
#define intBits 20

double convertToFloat(char *result);

int main(int argc, char *argv[]) {
	char *operand1;
	char *operand2;
	char operation;
	// check arguments number 	
	if (argc != 4) {
		printf("Input Error! Enter: ./calc [operand1] [operation] [operand2]\n");
		return -EINVAL;
	}
	
	operand1 = argv[1];
	operand2 = argv[3];
	operation = *argv[2];	
	
	//check whether operands are valid
	if (numbersOnly(operand1) == 0 || numbersOnly(operand2) == 0) {
		printf("Operand Error!\n");
		return -EINVAL;
	}
	//check whether operation is valid
	if (operation != '+' && operation != '-' && operation != '*' && operation != '/') {
		printf("Operation Error!\n"); 
		return -EINVAL;
	}
	//convert operands to fix point
	int operandFix1 = convertToFix(operand1);
	int operandFix2 = convertToFix(operand2);
	
        char str1[32];
	char str2[32];

	//convert int to string
	sprintf(str1, "%d", operandFix1);
	sprintf(str2, "%d", operandFix2);
	char *result = malloc(sizeof(char) * 32);
		
	//call syscall: calc
	int status = syscall(__NR_calc, str1, str2, operation, result);
 	//calculation is not valid if status equals to 0
	if (status == 0) {
		printf("%s\n", "nan");
		return -EINVAL;
	}	
	printf("%f\n", convertToFloat(result));
	return 1;
}

/* check whether the input is valid */
int numbersOnly(char *s) {
	int numOfPoint = 0;
	if (isdigit(*s) == 0) {
		//the first charactor of the operand could be '-'
		if (*s != '-') {
			return 0;
		}
		else {
			s++;
		}
	}
	while (*s) {
		if (isdigit(*s++) == 0) {
			return 0;
		}
		if (*s == '.') {	
			numOfPoint++;
			//there only could be at most one '.' in the operand
			if (numOfPoint > 1) {
				return 0;
			}
			s++;
		}
	}
	return 1;
}
/* convert input string to fix point format */
int convertToFix(char *operand) {
	int sign = 1;
	if (*operand == '-') {
		sign = -1;
		operand++;
	}
	char *decimalPosition = strchr(operand, '.');
	if (decimalPosition == NULL) {
		return ((atoi(operand) * sign) << floatBits);
	}
	double value = atof(operand);
	*decimalPosition = '\0';
	//get the integer part of the operand
	int intPart = atoi(operand);
	//get the float part of the operand
	double floatPart = value - (float)intPart;
	int floatParttoFix = floatPart * (1 << floatBits);
	return ((intPart << floatBits) | floatParttoFix) * sign;
}
/* convert fix point string to float */
double convertToFloat(char *result) {
	double sign = 1;
	if (*result == '-') {
		sign = -1;
		result++;
	}
	double resultFloat = atof(result) / (1 << floatBits);
	return resultFloat * sign;
}
