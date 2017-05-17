#include "jsondecode.h"

#include <string.h>
#if defined(__linux__)
#include <stddef.h>
#endif

char *findPairedQuote(char *str) {
	do {
		str++;//strchr?
	} while ((str[0] != '\"' || str[-1] == '\\') && str[0] != '\0');
	return str;
}

char* findPairedBracket(char* str) {
	do {
		str++;
		switch (str[0]) {
			case '\"':
				str = findPairedQuote(str);
				break;
			case '[':
				str = findPairedBracket(str);
				break;
			case ']':
				return str;
			default:
				break;
		}
	} while (str[0]!='\0');
	return str;
}

char* findPairedBigBracket(char* str) {
	do {
		str++;
		switch (str[0]) {
			case '\"':
				str = findPairedQuote(str);
				break;
			case '{':
				str = findPairedBigBracket(str);
				break;
			case '}':
				return str;
			default:
				break;
		}
	} while (str[0] != '\0');
	return str;
}

char* findNextComma(char* str) {
	do {
		str++;
		switch (str[0]) {
			case '\"':
				str = findPairedQuote(str);
				break;
			case '[':
				str = findPairedBracket(str);
				break; 
			case '{':
				str = findPairedBigBracket(str);
				break;
			case ',':
				return str;
			case '}': case ']':
				return NULL;
			default:
				break;
		}
	} while (str[0] != '\0');
	return str;
}

char* findStrSameLevel(char* str, char* findStr) {
	size_t lenCmp = strlen(findStr);
	do {
		str++;
		if (strncmp(str, findStr, lenCmp) == 0) {
			return str;
		}
		switch (str[0]) {
			case '[':
				str = findPairedBracket(str);
				break;
			case '{':
				str = findPairedBigBracket(str);
				break;
			case '}': case ']':
				return NULL;
			default:
				break;
		}
	} while (str[0] != '\0');
	return NULL;
}