#include <stdio.h>
#include <stdint.h>


#include "include/error.h"
#include "include/parse_arg.h"

int main(int argc, char* argv[]) {
	const uint32_t tread_num = get_thread_num(argc, argv);
	if(tread_num == 0) {
		return 1;
	}
	
	return 0;
}


