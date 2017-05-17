#pragma once

//""
char* findPairedQuote(char* str);

//[]
char* findPairedBracket(char* str);

//{}
char* findPairedBigBracket(char* str);

//,
char* findNextComma(char* str);

char* findStrSameLevel(char* str, char* findStr);