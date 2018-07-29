#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_LINE_LENGTH 255
#define MAX_OPCODE_LENGTH 5
#define OPCODE_COUNT 26
#define MAX_PSEUDOOP_LENGTH 5
#define PSEUDOOP_COUNT 3
#define RESGISTER_COUNT 8
#define HEX_WORD_SIZE 3 /* ex) 0x0*/

/* /////////////////////////////////////////GLOBAL VARs///////////////////////////////////////////////////*/
/* LC3-b instruction set mapping*/
char opcodeHex[OPCODE_COUNT][HEX_WORD_SIZE +1] = {"0x1", "0x5", "0x0", "0x0", "0x0", "0x0",
	"0x0", "0x0", "0x0", "0x0", "0xC", "0x4", "0x4", "0x2", "0x6", "0xE",
	"0x9", "0xC", "0xD", "0xD", "0xD", "0x8", "0x3", "0x7", "0x9"};
char opcodes[OPCODE_COUNT][MAX_OPCODE_LENGTH +1] = {"ADD", "AND", "BRn", "BRz", "BRp", "BR", 
	"BRzp", "BRnp", "BRnz", "BRnzp", "JMP", "JSR", "JSRR", "LDB", "LDW", "LEA",
	"NOT", "RET", "LSHF", "RSHFL", "RSHFA", "RTI", "STB", "STW", "XOR"};

/* Pseudo-Ops */
char pseudoOps[PSEUDOOP_COUNT][MAX_PSEUDOOP_LENGTH] = {".ORIG", ".FILL", ".END"};
static int ORIG_pseudoOp;
int FILL_pseudoOp;
int END_pseudoOp;

/* Label mapping */
int labelCount;
char **label; 	/* contains the labels - label index is used to index into boundedAddress array */
int *boundedAddress; /* contains address corresponding to label at that index */

int current_line_num;
int uncounted_lines;
int countSpecial;
/* /////////////////////////////////////////GLOBAL VARs///////////////////////////////////////////////////*/

/* lower cases string str */
char* lowerCase(char *str){
	char *lowerCaseStr = (char*) malloc(sizeof(char) * (strlen(str)+1)); /* todo: FREE */

	int i;
	for(i = 0; i < strlen(str); i++){
		lowerCaseStr[i] = tolower(str[i]);
	}
	lowerCaseStr[i] = '\0';

	return lowerCaseStr;
}

/* returns the next line of code */
char* nextLine(char *asmFile, int pass){ /* todo: second pass */
	char *buff = (char*) malloc(sizeof(char) * MAX_LINE_LENGTH); /* todo: FREE */
	FILE *fp = fopen(asmFile, "r");

	int i;
	for(i = 0; i <= current_line_num; i++){
		fgets(buff, MAX_LINE_LENGTH, (FILE*)fp);

		char *temp = (char*) malloc(sizeof(char) * MAX_LINE_LENGTH);
		strcpy(temp, buff);	
		temp = strtok(temp, "\t\n ,");
		if(strncmp(lowerCase(temp), ".end", 5) == 0){
			if(countSpecial == 0 && pass == 2){ exit(4); }
			current_line_num = 0;
			uncounted_lines = 0;
			return NULL; 
		}
	} 
	if(buff == NULL && current_line_num != 0){ exit(4); }
	current_line_num++;


	/* when bounding labels to a line of code, ignore .orig and comment as lines */
	if(pass == 2){
		char *temp = (char*) malloc(sizeof(char) * MAX_LINE_LENGTH);
		char *token = strtok(strcpy(temp, buff), "\t\n ,"); 
		if(strncmp(token, ";", 1) == 0 || strncmp(lowerCase(token), ".orig", 5) == 0){ uncounted_lines++; } 
		free(temp);
	}

	fclose(fp);
	return buff;
}

/* checks if token is a pseudo op - returns pseudo op index in pseudoOps[][] */
int isPseudoOp(char *token){
	int i;
	for(i = 0; i < PSEUDOOP_COUNT; i++){
		if(strncmp(lowerCase(pseudoOps[i]), lowerCase(token), 4) == 0){ return i; } /* todo: hardcoded */
	}
	return -1;
}

/* checks if token is an opcode - returns opcode index in opcodes[][] */
int isOpcode(char *token){
	int i;
	for(i = 0; i < OPCODE_COUNT; i++){
		if(strcmp(lowerCase(opcodes[i]), lowerCase(token)) == 0){ return i; }
	}
	return -1;
}

/* checks if token is an label - returns label index in label[][] which corresponds to boundedAddress[] */
int isLabel(char *token){ /* todo: EVERYWEHRE null might affect token and crap */
	int i;
	for(i = 0; i < labelCount; i++){
		if(strcmp(lowerCase(label[i]), lowerCase(token)) == 0){ return i; }
	}
	return -1;
}

/* returns 1 (true) if valid label
   Valid label: start with letter (except x), can contain 1 - 20 letters/numbers, different than opcodes and pseudo-op, not comment, not "in", "out", "getc", "puts" */
int isLabelValid(char *token){
	int i = 0;
	if((strlen(token) >= 1 && strlen(token) <= 20) && isPseudoOp(token) == -1 && isOpcode(token) == -1 && isalpha(token[i]) && token[i] != 'x' 
		&& strcmp(lowerCase(token), "in") != 0 && strcmp(lowerCase(token), "out") != 0 && strcmp(lowerCase(token), "getc") != 0 && strcmp(lowerCase(token), "puts") 
		!= 0 && strcmp(lowerCase(token), "trap") != 0 && strcmp(lowerCase(token), "halt") != 0 && strcmp(lowerCase(token), "nop") != 0 && strcmp(lowerCase(token), "r0") != 0 
		&& strcmp(lowerCase(token), "r1") != 0 && strcmp(lowerCase(token), "r2") != 0 && strcmp(lowerCase(token), "r3") != 0 && strcmp(lowerCase(token), "r4") != 0
		&& strcmp(lowerCase(token), "r5") != 0 && strcmp(lowerCase(token), "r6") != 0 && strcmp(lowerCase(token), "r7") != 0){
		for(i = 0; i < strlen(token); i++){
			if(!isalpha(token[i]) && !isdigit(token[i])){
				return 0;
			}
		}
		return 1;
	}
	return 0;	
}

void isLabelValid_ErrorCodeCheck(char *token){
	int i = 0;
	if(token[i] != 'x' && token[i] != '#'){
		if(!isalpha(token[i]) || (strlen(token) < 1) || (strlen(token) > 20) || isPseudoOp(token) != -1 || isOpcode(token) != -1 || strcmp(lowerCase(token), "in") != 0 ||
		 strcmp(lowerCase(token), "out") != 0 || strcmp(lowerCase(token), "getc") != 0 || strcmp(lowerCase(token), "puts") != 0 || strcmp(lowerCase(token), "trap") != 0 || 
		 strcmp(lowerCase(token), "halt") != 0 || strcmp(lowerCase(token), "nop") != 0 || strcmp(lowerCase(token), "r0") != 0 
		|| strcmp(lowerCase(token), "r1") != 0 || strcmp(lowerCase(token), "r2") != 0 || strcmp(lowerCase(token), "r3") != 0 || strcmp(lowerCase(token), "r4") != 0
		|| strcmp(lowerCase(token), "r5") != 0 || strcmp(lowerCase(token), "r6") != 0 || strcmp(lowerCase(token), "r7") != 0){ exit(1); }

		for(i = 0; i < strlen(token); i++){
			if(!isalpha(token[i]) && !isdigit(token[i])){
				exit(1);
			}
		}
	}
}

/*tokenize input string */
char** tokenize(char* line){ /* todo: tab vs space */
	int tokenCount = 0;
	char **tokenizedLine = (char**) calloc(MAX_LINE_LENGTH, sizeof(char*)); /* todo: free */

	char *temp = strtok(line, "\t\n ,");
	while(temp != NULL){
		tokenizedLine[tokenCount] = (char*) calloc(16, sizeof(char)); /* todo: hardcoded */
		strcpy(tokenizedLine[tokenCount], temp);
		tokenCount++;
		temp = strtok(NULL, "\t\n ,");
	}
	return tokenizedLine;
}

/* first pass - bounds labels to specific memory addresses */
void boundLables(char* asmFile){
	char* line;
	int size = 5; /* initial size of label and boundedAddress array */
	labelCount = 0;

	label = (char**) malloc(sizeof(char**) * size);
	boundedAddress = (int*) malloc(sizeof(int) * size);

	/* labels are always the first word and not an opcode or comment */
	while((line = nextLine(asmFile, 1)) != NULL){
		char *temp = (char*) malloc(sizeof(char) * strlen(line));
		strcpy(temp, line);
		char **token = tokenize(line);

		/* when bounding labels to a line of code, ignore .orig and comment as lines */
		if(strncmp(token[0], ";", 1) == 0 || strncmp(lowerCase(token[0]), ".orig", 5) == 0){ uncounted_lines++; } 

		if(isLabelValid(token[0]) == 1 && token[1] != NULL){
			if(isLabel(token[0]) != -1){ exit(4); }
			label[labelCount] = (char*) malloc(sizeof(char) * strlen(token[0]));
			strcpy(label[labelCount], token[0]);
			boundedAddress[labelCount] = current_line_num - uncounted_lines; 

			/* increase size for both label and boundedAddress string arrays */
			if(++labelCount >= size){ 
				size *= size;
				label = realloc(label, size);
				boundedAddress = realloc(boundedAddress, size);
			} 
		}
		free(temp); free(token);
	}
}

/* converts decimal dec to binary */
long long intToBin(long long dec){ 
    if (dec == 0){
    	return 0;
    }
    else{
        return (dec % 2 + 10 * intToBin(dec / 2));
    }
}

 /* Takes 4 characters at a time from the long binary string and translate it to hex (and outputs to file) --> hexAmount is number of resutlting hex chacters from binaryStr */
void finalTranslation(char *binaryStr, FILE *fp, int hexAmount){
	char bin[16][5] = {"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};
	char hex[16][2] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};

	char fourBinaryChars[5];
	int i, j;
	for(i = 0; i < hexAmount; i++, binaryStr+=4){ 
		memcpy(fourBinaryChars, binaryStr, 4); 

		for(j = 0; j < 16; j++){  /* find hex translation and print it to file */
			if(strncmp(fourBinaryChars, bin[j], 4) == 0){ 
				fprintf(fp, "%s", hex[j]);
				break; 
			}
		}
	}
	fprintf(fp, "\n"); 
}

/* concatenates binary in the different opcodes into a binary string (based on lc3-b ISA) */
char* concatenateBinary(char *opcode, char *operand1, char *operand2, char *operand3, int immediate){
	char *binaryStr;

	if(operand1 != NULL  && operand2 == NULL){
		binaryStr = (char*) malloc(sizeof(char) * (strlen(operand1)));
	}else if(operand2 != NULL && operand3 == NULL){
		binaryStr = (char*) malloc(sizeof(char) * (strlen(operand1) + strlen(operand2)));
	}else{
		binaryStr = (char*) malloc(sizeof(char) * (strlen(operand1) + strlen(operand2) + strlen(operand3)));
	}

	if(strcmp(lowerCase(opcode), lowerCase("LDB")) == 0 || strcmp(lowerCase(opcode), lowerCase("LDW")) == 0 || strcmp(lowerCase(opcode), lowerCase("STB")) == 0 || strcmp(lowerCase(opcode), lowerCase("STW")) == 0){
		sprintf(binaryStr, "%s%s%s", operand1, operand2, operand3);
	}else if(strcmp(lowerCase(opcode), lowerCase("LSHF")) == 0){
		sprintf(binaryStr, "%s%s00%s", operand1, operand2, operand3);
	}else if(strcmp(lowerCase(opcode), lowerCase("RSHFL")) == 0){
		sprintf(binaryStr, "%s%s01%s", operand1, operand2, operand3);
	}else if(strcmp(lowerCase(opcode), lowerCase("RSHFA")) == 0){
		sprintf(binaryStr, "%s%s11%s", operand1, operand2, operand3);
	}else if(strcmp(lowerCase(opcode), lowerCase("LEA")) == 0){
		sprintf(binaryStr, "%s%s", operand1, operand2);
	}else if(strcmp(lowerCase(opcode), lowerCase("NOT")) == 0){
		sprintf(binaryStr, "%s%s111111", operand1, operand2);
	}else if(strcmp(lowerCase(opcode), lowerCase("ADD")) == 0 || strcmp(lowerCase(opcode), lowerCase("AND")) == 0 || strcmp(lowerCase(opcode), lowerCase("XOR")) == 0){
		if(immediate == 0){
			sprintf(binaryStr, "%s%s000%s", operand1, operand2, operand3);
		}else {
			sprintf(binaryStr, "%s%s1%s", operand1, operand2, operand3);
		}
	}else if(strcmp(lowerCase(opcode), lowerCase("RET")) == 0){
		sprintf(binaryStr, "000111000000");
	}else if(strcmp(lowerCase(opcode), lowerCase("JMP")) == 0){
		sprintf(binaryStr, "000%s000000", operand1);
	}else if(strcmp(lowerCase(opcode), lowerCase("JSR")) == 0){
		sprintf(binaryStr, "1%s", operand1);
	}else if(strcmp(lowerCase(opcode), lowerCase("JSRR")) == 0){
		sprintf(binaryStr, "000%s000000", operand1);
	}else if(strcmp(lowerCase(opcode), lowerCase("BRn")) == 0){
		sprintf(binaryStr, "100%s", operand1);
	}
	else if(strcmp(lowerCase(opcode), lowerCase("BRz")) == 0){
		sprintf(binaryStr, "010%s", operand1);
	}
	else if(strcmp(lowerCase(opcode), lowerCase("BRp")) == 0){
		sprintf(binaryStr, "010%s", operand1);
	}
	else if(strcmp(lowerCase(opcode), lowerCase("BR")) == 0){
		sprintf(binaryStr, "111%s", operand1);
	}
	else if(strcmp(lowerCase(opcode), lowerCase("BRzp")) == 0){
		sprintf(binaryStr, "011%s", operand1);
	}
	else if(strcmp(lowerCase(opcode), lowerCase("BRnp")) == 0){
		sprintf(binaryStr, "101%s", operand1);
	}
	else if(strcmp(lowerCase(opcode), lowerCase("BRnz")) == 0){
		sprintf(binaryStr, "110%s", operand1);
	}
	else if(strcmp(lowerCase(opcode), lowerCase("BRnzp")) == 0){
		sprintf(binaryStr, "111%s", operand1);
	}

	return binaryStr;
}


#define HEX_INT_SIZE 4
char* hexToBin(char *hexStr, int bits, int hexStrLength){
	char bin[16][5] = {"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};
	char hex[16][2] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F"};

	char *tempArr = (char*) calloc(4*hexStrLength, sizeof(char));

	hexStr = lowerCase(hexStr);
	int i, j;
	for(i = 0; i < hexStrLength; i++){
		for(j = 0; j < 16; j++){
			if(strncmp(hexStr, lowerCase(hex[j]), 1) == 0){
				strcat(tempArr, bin[j]);
				hexStr++;
				break;
			}
		}
	}

	int time = (hexStrLength*4)-bits;

	if(time < 0){
		time = (16 - bits)-1;
		char temp[17] = "0000000000000000";
		strcpy(&temp[time], tempArr);
		tempArr = (char*) realloc(tempArr, 16);
		strcpy(tempArr, temp);
	}

	/*printf("%i, %s\n", time, &(tempArr[time]));*/
	return (tempArr+=time);
}

/* returns hex of the string input - input should be either # or #- or x or x- */
char* hex(char *operand, int bits){
	int lower = (int) -(pow((double)2, (double)(bits-1)));
	int upper = ((int) pow((double)2, (double)(bits-1))) -1;

	int temp;
	if(strncmp(operand, "#", 1) == 0){
		temp = atoi(++operand);
		if(temp > upper || temp < lower){ exit(3); }
		sprintf(operand, "%x", temp); /* convert to hex */
	}else if(strncmp(lowerCase(operand), "x", 1) == 0){
		if(strncmp(++operand, "-", 1) == 0){
			temp = strtol(++operand, NULL, 16); 
			temp = -(temp);
		}else{
			temp = strtol(operand, NULL, 16);
		}
		if(temp > upper || temp < lower){ exit(3); }
		sprintf(operand, "%x", temp);
	}
	return operand;
}

/* returns hex of the string input - input should be either # or #- or x or x- */
char* hex2(char *operand, int bits){
	int lower = 0;
	int upper = ((int) pow((double)2, (double)bits)) -1;

	int temp;
	if(strncmp(operand, "#", 1) == 0){
		temp = atoi(++operand);
		if(temp > upper || temp < lower){ exit(3); }
		sprintf(operand, "%x", temp); /* convert to hex */
	}else if(strncmp(lowerCase(operand), "x", 1) == 0){
		if(strncmp(++operand, "-", 1) == 0){
			temp = strtol(++operand, NULL, 16); 
			temp = -(temp);
		}else{
			temp = strtol(operand, NULL, 16);
		}
		if(temp > upper || temp < lower){ exit(3); }
		sprintf(operand, "%x", temp);
	}
	return operand;
}

void operandsToBinary(char* opcode, char *operand1, char *operand2, char *operand3, FILE *fp){
	char registers[RESGISTER_COUNT][3] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7"};
	char registersBin[RESGISTER_COUNT][4] = {"000", "001", "010", "011", "100", "101", "110", "111"};

	int immediate = 0, i;
	char *binaryStr;

	int operand1_register = 0; int operand2_register = 0; int operand3_register = 0;

	/* if operands are not null and represent registers, convert to the corresponding binary (opcode binary will be concatinated and converted to hex later) */
	if(operand1 != NULL && strcmp(lowerCase(&operand1[0]), "r") == 0 && (atoi(&operand1[1]) > 7 || atoi(&operand1[1]) < 0)){ exit(4); }
	if(operand2 != NULL && strcmp(lowerCase(&operand2[0]), "r") == 0 && (atoi(&operand2[1]) > 7 || atoi(&operand2[1]) < 0)){ exit(4); }
	if(operand3 != NULL && strcmp(lowerCase(&operand3[0]), "r") == 0 && (atoi(&operand3[1]) > 7 || atoi(&operand3[1]) < 0)){ 
		exit(4); 
	}
	for(i = 0; i < RESGISTER_COUNT; i++){ /* covers jsrr, not, jmp */
		if(operand1 != NULL && strcmp(lowerCase(operand1), lowerCase(registers[i])) == 0){ operand1 = registersBin[i]; operand1_register = 1;}
		if(operand2 != NULL && strcmp(lowerCase(operand2), lowerCase(registers[i])) == 0){ operand2 = registersBin[i]; operand2_register = 1; }
		if(operand3 != NULL && strncmp(lowerCase(operand3), lowerCase(registers[i]), 2) == 0){ operand3 = registersBin[i]; operand3_register = 1;} /* todo: hardcoded because null was affecting */
	}

	/* if instruction has immediate, do the same as above --> instructions have certain of their opcodes as possibly immediates (checking only those) */ /* todo: check range if it fits in x bits */
	if(strcmp(lowerCase(opcode), lowerCase("ADD")) == 0 || strcmp(lowerCase(opcode), lowerCase("AND")) == 0 || strcmp(lowerCase(opcode), lowerCase("XOR")) == 0){
		if(operand1_register == 1 && operand2_register == 1 && operand3_register == 0){
			if(operand3 == NULL){ exit(4); }
			if(strncmp(operand3, "#", 1) != 0 && strncmp(lowerCase(operand3), "x", 1) != 0){ exit(4); }
			operand3 = hex(operand3, 5);
			sprintf(operand3, "%.5i", atoi(hexToBin(operand3, 5, strlen(operand3)))); /* convert to binary */
			immediate++;
		}else if(operand1_register == 0 || operand2_register == 0 || operand3_register == 0){ exit(4);}
	}else if(strcmp(lowerCase(opcode), lowerCase("LDB")) == 0 || strcmp(lowerCase(opcode), lowerCase("LDW")) == 0 || strcmp(lowerCase(opcode), lowerCase("STB")) == 0 || strcmp(lowerCase(opcode), lowerCase("STW")) == 0){
		if(operand1_register == 1 && operand2_register == 1 && operand3_register == 0){
			if(operand3 == NULL){ exit(4); }
			if(strncmp(operand3, "#", 1) != 0 && strncmp(lowerCase(operand3), "x", 1) != 0){ exit(4); }
			operand3 = hex(operand3, 6);
			sprintf(operand3, "%.6i", atoi(hexToBin(operand3, 6, strlen(operand3)))); /* convert to binary */
			immediate++;
		}else{ exit(4); }
	}else if(strncmp(lowerCase(opcode), lowerCase("BR"), 2) == 0){
		if(operand1_register == 0 && operand2_register == 0 && operand3_register == 0){
		 /* convert the label to # of instructions to change PC by */
			if(operand1 == NULL){ exit(4); }
			int index;
			if((index = isLabel(operand1)) != -1){
				sprintf(operand1, "%i", boundedAddress[index] - ((current_line_num - uncounted_lines)+1)); /*add one bc pc is at next instructions already */
				if(strncmp(operand1, "-", 1) == 0){ /* - num */
					int temp = atoi(operand1);
					sprintf(operand1, "%x", temp); /* convert to hex */
					sprintf(operand1, "%.9i", atoi(hexToBin(operand1, 9, strlen(operand1))));  /* convert to binary array */
				}else{
					sprintf(operand1, "%.9lld", intToBin((long long)atoi(operand1))); 
				}
				immediate++;
			}else{
				isLabelValid_ErrorCodeCheck(operand1);
				if(strncmp(operand1, "#", 1) != 0 && strncmp(lowerCase(operand1), "x", 1) != 0){ exit(4); }
				operand1 = hex(operand1, 9);
				sprintf(operand1, "%.9i", atoi(hexToBin(operand1, 9, strlen(operand1)))); /* convert to binary */
			}
		}else { exit(4); }
	}else if(strcmp(lowerCase(opcode), lowerCase("LEA")) == 0){
		if(operand1_register == 1 && operand2_register == 0 && operand3_register == 0){
			if(operand2 == NULL){ exit(4); }
			 /* convert the label to # of instructions to change PC by */
			int index;
			if((index = isLabel(operand2)) != -1){
				sprintf(operand2, "%i", boundedAddress[index] - ((current_line_num - uncounted_lines)+1)); /*add one bc pc is at next instructions already */
				if(strncmp(operand2, "-", 1) == 0){ /* - num */
					int temp = atoi(operand2);
					sprintf(operand2, "%x", temp); /* convert to hex */
					sprintf(operand2, "%.9i", atoi(hexToBin(operand2, 9, strlen(operand2))));  /* convert to binary array */
				}else{
					sprintf(operand2, "%.9lld", intToBin((long long)atoi(operand2))); 
				}
				immediate++;
			}else{
				isLabelValid_ErrorCodeCheck(operand2);
				if(strncmp(operand2, "#", 1) != 0 && strncmp(lowerCase(operand2), "x", 1) != 0){ exit(4); }
				operand2 = hex(operand2, 9);
				sprintf(operand2, "%.9i", atoi(hexToBin(operand2, 9, strlen(operand2)))); /* convert to binary */
			}
		}else{ exit(4); }
	}else if(strcmp(lowerCase(opcode), lowerCase("LSHF")) == 0 || strcmp(lowerCase(opcode), lowerCase("RSHFL")) == 0 || strcmp(lowerCase(opcode), lowerCase("RSHFA")) == 0){
		if(operand1_register == 1 && operand2_register == 1 && operand3_register == 0){
			if(operand3 == NULL){ exit(4); }
			if(strncmp(operand3, "#", 1) != 0 && strncmp(lowerCase(operand3), "x", 1) != 0){ exit(4); }
			if(strncmp(++operand3, "-", 1) == 0 && strncmp(++operand3, "0", 1) != 0){ exit(3); }
			operand3-=2;
			operand3 = hex(operand3, 4);
			sprintf(operand3, "%.4i", atoi(hexToBin(operand3, 4, strlen(operand3)))); /* convert to binary */
		}else{ exit(4); }
	}else if(strcmp(lowerCase(opcode), lowerCase("JSR")) == 0){
		if(operand1_register == 0 && operand2_register == 0 && operand3_register == 0){
			if(operand1 == NULL){ exit(4); }
			int index;
			if((index = isLabel(operand1)) != -1){
				sprintf(operand1, "%i", boundedAddress[index] - ((current_line_num - uncounted_lines)+1)); /*add one bc pc is at next instructions already */
				if(strncmp(operand1, "-", 1) == 0){ /* - num */
					int temp = atoi(operand1);
					sprintf(operand1, "%x", temp); /* convert to hex */
					sprintf(operand1, "1%.10i", atoi(hexToBin(operand1, 10, strlen(operand1))));  /* convert to binary array */
				}else{
					sprintf(operand1, "%.11lld", intToBin((long long)atoi(operand1))); 
				}
				immediate++;
			}else{
				isLabelValid_ErrorCodeCheck(operand1);
				if(strncmp(operand1, "#", 1) != 0 && strncmp(lowerCase(operand1), "x", 1) != 0){ exit(4); }
				operand1 = hex(operand1, 11);
				sprintf(operand1, "%11i", atoi(hexToBin(operand1, 11, strlen(operand1)))); /* convert to binary */
			}
		}else{ exit(4); }
	}

	/* use immediate later when adding intermediate bits based on ISA (ex. ADD with 3 registers vs immediate) */
	if(operand1 == NULL && operand2 == NULL && operand3 == NULL){ exit(4); }
	if(immediate == 0){ binaryStr = concatenateBinary(opcode, operand1, operand2, operand3, 0); }
	else{ binaryStr = concatenateBinary(opcode, operand1, operand2, operand3, 1); }

	/* final step: convert binaryStr into hex and output to .obj file */
	finalTranslation(binaryStr, fp, 3);
}

int isORIGOdd(char hex){
	if(strncmp(&hex, "1", 1) == 0 || strncmp(&hex, "3", 1) == 0 || strncmp(&hex, "5", 1) == 0 || strncmp(&hex, "7", 1) == 0 || strncmp(&hex, "9", 1) == 0 || 
		strncmp(&hex, "b", 1) == 0 || strncmp(&hex, "d", 1) == 0 || strncmp(&hex, "f", 1) == 0){ exit(3); }
}


/* translates and writes to .obj file */
void beginTranslation(char **tokenizedLine, FILE *fp){
	int i = 0;
	int index;

	/* remove comments */
	while(tokenizedLine[i] != NULL && strncmp(&(tokenizedLine[i][0]), ";", 1) != 0){ /* todo: 2 label error, instruction after instruction, what if only 2 operands and comment even though u need 3 */
		countSpecial++;
		if(countSpecial == 1 && strncmp(tokenizedLine[i], pseudoOps[0], 4) != 0){ exit(4); } /* must start with .ORIG */

		if((index = isLabel(tokenizedLine[i])) == -1){
			if((index = isOpcode(tokenizedLine[i])) != -1){
				char* opcode = tokenizedLine[i];
				char *operand1 = NULL; /* why does this work without malloc? */
				char *operand2 = NULL;
				char *operand3 = NULL;

				/* parse out operands for opcodes */
				if(tokenizedLine[++i] != NULL && strncmp(&(tokenizedLine[i][0]), ";", 1) != 0){ operand1 = strtok(tokenizedLine[i], ","); }
				else{ i--; }
				if(tokenizedLine[++i] != NULL && strncmp(&(tokenizedLine[i][0]), ";", 1) != 0){ operand2 = strtok(tokenizedLine[i], ","); }
				else{ i--; }
				if(tokenizedLine[++i] != NULL && strncmp(&(tokenizedLine[i][0]), ";", 1) != 0){ operand3 = strtok(tokenizedLine[i], ","); }
				else{ i--; }

				fprintf(fp, "%s", opcodeHex[index]);  /* output opcode to .obj file (not operands yet) */
				operandsToBinary(opcode, operand1, operand2, operand3, fp);
			}else if((index = isPseudoOp(tokenizedLine[i])) != -1){
				if(strncmp(tokenizedLine[i], pseudoOps[0], 4) == 0 || strncmp(tokenizedLine[i], pseudoOps[1], 4) == 0){ /* .ORIG or .FILL */
					char *number = (char*) malloc(sizeof(char) * 80);
					i++; 
					if(tokenizedLine[i] == NULL){ exit(4); } 

					char temp4[30];
					strcpy(temp4, tokenizedLine[i]);
					if(index == 0){ 
						hex2(temp4, 16);
					}else{
						hex(temp4, 16);
					}

					if(strncmp(lowerCase(tokenizedLine[i]), "x", 1) == 0){ /* hex */ 
						if(strncmp(++tokenizedLine[i], "-", 1) == 0){
							int temp = strtol(++tokenizedLine[i], NULL, 16); 
							temp = -(temp);
							sprintf(number, "%x", temp);

							if(index == 0){ isORIGOdd(number[7]); }

							fprintf(fp, "0x%s\n", number+=4);
						}else{
							if(index == 0){ isORIGOdd(tokenizedLine[1][strlen(tokenizedLine[1])-1]); }

							fprintf(fp, "0x");
							int j;
							for(j = strlen(tokenizedLine[i]); j < 4; j++){
								fprintf(fp, "0");
							}
							fprintf(fp, "%s\n", tokenizedLine[i]);
						}
					}else if(strncmp(lowerCase(tokenizedLine[i]), "#", 1) == 0){ /* number */
						fprintf(fp, "0x");
						if(strncmp(++tokenizedLine[i], "-", 1) == 0){
							sprintf(number, "%x", atoi(tokenizedLine[i])); /* convert to hex */
							if(index == 0){ isORIGOdd(number[7]); }
							strcpy(number, hexToBin(number, 16, strlen(number)));
							finalTranslation(number, fp, 4);
						}else{
							sprintf(number, "%.16lld", intToBin((long long)atoi(tokenizedLine[i])));
							if(index == 0 && strncmp(&number[15], "1", 1) == 0){ exit(3); }
							finalTranslation(number, fp, 4);
						}
					}else{ exit(4); }
				}else if(strncmp(tokenizedLine[i], pseudoOps[2], 4) == 0){ /* .End */ }
				break; /*todo: check if invalid instruction*/
			}else if(strcmp(lowerCase(tokenizedLine[i]), "trap") == 0 || strcmp(lowerCase(tokenizedLine[i]), "halt") == 0){ /* trap and halt */
				fprintf(fp, "0xF025\n");
				break;
			}else if(strcmp(lowerCase(tokenizedLine[i]), "nop") == 0){
				fprintf(fp, "0x0000\n");
				break;
			}else if(strcmp(lowerCase(tokenizedLine[i]), "rti") == 0){
				fprintf(fp, "0x8000\n");
				break;
			}else if(strcmp(lowerCase(tokenizedLine[i]), "ret") == 0){
				fprintf(fp, "0xC1C0\n");
				break;
			}else{
				exit(2); /* means not an opcode - assume pseduoOps are well formatted */
			}
		}
		i++;
	}
}

/* second pass - translates assembly to machine language (hex) */
void translation(char* asmFile, char* outputFile){
	char *line;
	FILE *fp = fopen(outputFile, "w");

	if(current_line_num != 0){ perror("Second pass error"); } /* first pass didnt make it through entire file */
	
	while((line = nextLine(asmFile,2)) != NULL){
		char **tokenizedLine = tokenize(line); /* contains single line of asembly that is tokenized */
		beginTranslation(tokenizedLine, fp);
	}

	fclose(fp);
}

void assemble(char* asmFile, char* outputFile){
	boundLables(asmFile);
	translation(asmFile, outputFile);
}


int main(int argc, char* argv[]){
	char *asmFile = (char*) malloc(sizeof(char) * strlen(argv[1]));
	char *outputFile = (char*) malloc(sizeof(char) * strlen(argv[2]));

	strcpy(asmFile, argv[1]); 
	strcpy(outputFile, argv[2]);

	assemble(asmFile, outputFile);

	free(asmFile); free(outputFile);
	exit(0);
}