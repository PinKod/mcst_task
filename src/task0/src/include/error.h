#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdarg.h>




static inline void internal_err_log(const char* file, const char* func, int line, const char* format, ...) {
	fprintf(stderr, "[err log[file:%s, func:%s, line:%d]]	<", file, func, line);
	if(format == NULL) perror("error");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, ">\n");
}

#define err_log(format, ...) internal_err_log(__FILE__, __func__, __LINE__, format, ##__VA_ARGS__);

#endif