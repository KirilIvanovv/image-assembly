#include "error.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h> 

#define ERROR_BUFFER_SIZE 512

static char error_buffer[ERROR_BUFFER_SIZE] = {'\0'};
static int last_error_code = IMGASM_ERROR_NONE;

int imgasm_set_error(int code, const char* format, ...) {
    last_error_code = code;
    
    if (format != NULL) {
        va_list args;
        va_start(args, format);
        vsnprintf(error_buffer, ERROR_BUFFER_SIZE, format, args);
        va_end(args);
    } else {
        error_buffer[0] = '\0';
    }
	
	return code;
}

const char* imgasm_get_error() {
    return error_buffer;
}

int imgasm_get_error_code() {
    return last_error_code;
}

void imgasm_clear_error() {
    last_error_code = IMGASM_ERROR_NONE;
    error_buffer[0] = '\0';
}

void imgasm_print_error() {
	printf("%s\n", error_buffer);
}