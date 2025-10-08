#ifndef IMGASM_ERROR_H
#define IMGASM_ERROR_H

#define IMGASM_ERROR_NONE                      0
#define IMGASM_ERROR_FILE_IO_ERROR             1
#define IMGASM_ERROR_FILE_OPEN_ERROR           2
#define IMGASM_ERROR_FILE_INCORRECT_HEADER     3
#define IMGASM_ERROR_FILE_INCORRECT_VERSION    4
#define IMGASM_ERROR_FILE_INCORRECT_INPUT_INFO 5
#define IMGASM_ERROR_FILE_INCORRECT_FRAME      6
#define IMGASM_ERROR_INVALID_OPERATION         7
#define IMGASM_ERROR_INVALID_ASSEMBLY_LIST     8
#define IMGASM_ERROR_INVALID_ASSEMBLY_IMAGE    9

int imgasm_set_error(int code, const char* format, ...);

const char* imgasm_get_error();

int imgasm_get_error_code();

void imgasm_clear_error();
void imgasm_print_error();

#endif // IMGASM_ERROR_H 