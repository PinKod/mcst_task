#include <errno.h>
#include <stdlib.h>

#include "include/error.h"
#include "include/parse_arg.h"


uint32_t get_thread_num(int argc, char* argv[]) {
	if(argc != 2) {
		err_log("wrong usage, %s <num of threads - uint32_t>\n", argv[0]);
		return 1;
	}

	const char* str = argv[1];
	if(str == NULL || *str == '\0' || *str == '-') {
		err_log("invalid arg. note: tread num cant be negative\n");
		return 0;
	}

	char *endptr = NULL;
    errno = 0;
    const unsigned long val = strtoul(str, &endptr, 10);
	if(endptr == str || errno == ERANGE || *endptr != '\0' || val > UINT32_MAX) {
		err_log("invalid arg. error during num parsing\n");
		return 0;
	}
	else {
		return (uint32_t)val;
	}
}