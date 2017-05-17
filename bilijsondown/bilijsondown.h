#pragma once

#include <stdint.h>

typedef struct part {
	uint32_t bytes;
	char *downurl;
	char **backupurls;
} part_t;