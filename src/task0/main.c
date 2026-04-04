#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>

#include "include/error.h"


static uint32_t get_thread_num(const char* str);


int main(int argc, char* argv[]) {
	if(argc != 2) {
		err_log("wrong usage, %s <num of threads - uint32_t>\n", argv[0]);
		return 1;
	}

	const uint32_t tread_num =  get_thread_num(argv[1]);
	if(tread_num == 0) {
		return 1;
	}

	return 0;
}



static uint32_t get_thread_num(const char* str) {
	if(str == NULL || *str == '\0' || *str == '-') {
		err_log("invalid arg\n");
		return 0;
	}

	char *endptr = NULL;
    errno = 0;
    const unsigned long val = strtoul(str, &endptr, 10);
	if(endptr == str || errno == ERANGE || *endptr != '\0' || val > UINT32_MAX) {
		err_log("invalid arg\n");
		return 0;
	}
	else {
		return (uint32_t)val;
	}
}