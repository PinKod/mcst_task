#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdarg.h>


static inline void err_log(const char* format, ...) {
	if(format == NULL) perror("error");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

#endif