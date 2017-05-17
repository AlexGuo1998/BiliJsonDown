#define _CRT_SECURE_NO_WARNINGS

#include "bilijsondown.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "jsondecode.h"

#if defined(_WIN32)
#define DELETE_CMD "del"
#elif defined(__linux__)
#define DELETE_CMD "rm"
#else
#error Unsupported platform!
#endif


#define MALLOC_OR_DIE(var, type, size) \
var = (type *)malloc(size);\
if (var == NULL) {\
	fprintf(stderr, "Run out of memory!\n");\
	return 1;\
}

void printHelp(void) {
	fprintf(stderr,
		"Bilibili Json Downloader - Download videos from bilibili and concat automatically\n"
		"Usage: bilijsondown [args] FILE\n"
		"FILE is a \"index.json\" file from Bilibili Android Application.\n\n"
		"args:\n"
		"-n, --name [String]    The name of the download file (without ext)\n"
		"-t, --retry [Integer]  Retry count per file\n"
		"-k, --keep             Keep temp files\n"
	);
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		fprintf(stderr, "\nPlease open a \"index.json\" file!\ntype: \"beepplayer -h\" for help\n");
		return 1;
	}
	char *outfilename = "bilidownload";
	int retrytimes = 5;
	int warned = 0;
	int deletefile = 1;
	for (int i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
			printHelp();
			return 0;
		} else {
			if (i < argc - 1) {//0 var args
				if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keep") == 0) {
					deletefile = 0;
				}
			}
			if (i < argc - 2) {//1 var args
				if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--name") == 0) {
					i++;
					outfilename = argv[i];
				} else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--retry") == 0) {
					i++;
					sscanf(argv[i], "%d", &retrytimes);
				}
			}
		}
	}

	FILE *pf = fopen(argv[argc - 1], "rb");
	if (pf == NULL) {
		fprintf(stderr, "Can't open file \"%s\"!\n", argv[argc - 1]);
		return 1;
	}

	fseek(pf, 0, SEEK_END);
	char* filestr; int filelen;
	filelen = ftell(pf);
	
	MALLOC_OR_DIE(filestr, char, filelen + 1);
	rewind(pf);
	fread(filestr, 1, filelen, pf);
	filestr[filelen] = '\0';
	fclose(pf);

	char *liststart;
	if (!((liststart = findStrSameLevel(filestr, "segment_list")) && (liststart = findStrSameLevel(liststart + 12, "[")))) {
		fprintf(stderr, "Can't find segment_list\n");
		return 1;
	}

	char *n = liststart;
	int partcount = 1;
	while (n = findNextComma(n)) {
		partcount++;
	}

	part_t *partlist;
	MALLOC_OR_DIE(partlist, part_t, sizeof(part_t) * partcount);

	n = liststart;
	for (int i = 0; i < partcount; i++) {
		char *start0, *start, *end;
		if (!((start0 = findStrSameLevel(n, "{")) &&
		(start = findStrSameLevel(start0, "url")) &&
		(start = findStrSameLevel(start + 3, "\"")) &&
		(end = findPairedQuote(start)))) {
			fprintf(stderr, "Can't get main URL for part #%d!\n", i + 1);
			return 1;
		}
		//addr
		MALLOC_OR_DIE(partlist[i].downurl, char, end - start);

		int copiedcount = 0;
		for (int j = 1; j < end - start; j++) {
			if (start[j] == '\\' && start[j + 1] == '/') j++;
			partlist[i].downurl[copiedcount++] = start[j];
		}
		partlist[i].downurl[copiedcount] = '\0';
		
		//size
		char sizestr[32];
		if ((start = findStrSameLevel(start0, "bytes")) &&
		(start = findStrSameLevel(start + 5, ":")) &&
		(end = findNextComma(start)) &&
		(end - start <= 32)) {
			strncpy(sizestr, start + 1, end - start - 1);
			sizestr[end - start - 1] = '\0';
			int ret = sscanf(sizestr, "%d", &partlist[i].bytes);
			if (ret == 0) partlist[i].bytes = 0;
		} else {
			partlist[i].bytes = 0;
		}
		//TODO read backup
		/*
		if ((start = findStrSameLevel(start0, "bytes")) &&
			(start = findStrSameLevel(start + 5, ":")) &&
			(end = findNextComma(start)) &&
			(end - start <= 32)) {
			strncpy(sizestr, start + 1, end - start - 1);
			sizestr[end - start - 1] = '\0';
			int ret = sscanf(sizestr, "%d", &partlist[i].bytes);
			if (ret == 0) partlist[i].bytes = 0;
		} else {
			partlist[i].bytes = 0;
		}
		*/
		n = findNextComma(n);
	}
	
	free(filestr);
	
	char *concatfile;
	MALLOC_OR_DIE(concatfile, char, sizeof("-concat.txt") + strlen(outfilename));

	sprintf(concatfile, "%s-concat.txt", outfilename);
	FILE *pfconcat = fopen(concatfile, "w");
	
	for (int i = 0; i < partcount; i++) {
		fprintf(stderr, "URL: %s\nSize:%d\n", partlist[i].downurl, partlist[i].bytes);
		
		char *downfile;
		MALLOC_OR_DIE(downfile, char, sizeof("-.flv") + 31 + strlen(outfilename));
		sprintf(downfile, "%s-%d.flv", outfilename, i);
		
		//TODO down backup
		char *wgetcmd;
		MALLOC_OR_DIE(wgetcmd, char, sizeof("wget -o \".flv\" --progress=dot:giga \"\"") + strlen(partlist[i].downurl) + strlen(downfile));
		sprintf(wgetcmd, "wget -O \"%s\" --progress=dot:giga \"%s\"", downfile, partlist[i].downurl);
		
		int downtimes = retrytimes + 1;
		do {
			system(wgetcmd);
		
			//check
			if (partlist[i].bytes > 0) {
				FILE *pf = fopen(downfile, "rb");
				if (pf) {
					fseek(pf, 0, SEEK_END);
					if (ftell(pf) == partlist[i].bytes) downtimes = 0;
					fclose(pf);
				}
			} else {
				fprintf(stderr, "WARNING: File size check ignored\n");
				warned = 1;
			}
			downtimes--;
		} while (downtimes > 0);

		if (downtimes == 0) {
			fprintf(stderr, "WARNING: Download failed\n");
			warned = 1;
		}
		fprintf(pfconcat, "file '%s'\n", downfile);
		free(wgetcmd);
		free(downfile);
	}
	fclose(pfconcat);
	//concat
	char *concatcmd;
	MALLOC_OR_DIE(concatcmd, char, sizeof("ffmpeg -f concat -i \"\" -c copy \".flv\"") + strlen(concatfile) + strlen(outfilename));
	sprintf(concatcmd, "ffmpeg -f concat -i \"%s\" -c copy \"%s.flv\"", concatfile, outfilename);
	system(concatcmd);
	free(concatcmd);

	if (!warned && deletefile) {
		char *rmcmd;
		MALLOC_OR_DIE(rmcmd, char, sizeof(DELETE_CMD " \"\"") + strlen(concatfile));
		sprintf(rmcmd, DELETE_CMD " \"%s\"", concatfile);
		system(rmcmd);
		free(rmcmd);
		MALLOC_OR_DIE(rmcmd, char, sizeof(DELETE_CMD " \"-.flv\"") + strlen(outfilename) + 31);
		for (int i = 0; i < partcount; i++) {
			sprintf(rmcmd, DELETE_CMD " \"%s-%d.flv\"", outfilename, i);
			system(rmcmd);
		}
		free(rmcmd);
	}
	free(concatfile);

	for (int i = 0; i < partcount; i++) {
		free(partlist[i].downurl);
		//TODO free backup
	}
	free(partlist);

	return 0;
}