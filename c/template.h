#ifndef IMGASM_TEMPLATE_H
#define IMGASM_TEMPLATE_H

#include <stdio.h>

typedef struct imgasm_frame {
    int width;
    int height;
    long offset;
    char* bytecode_begin;
    int bytecode_length;
} imgasm_frame_t;

typedef struct imgasm_input {
    int width;
    int height;
} imgasm_input_t;

typedef struct imgasm_template {
    int flags;
	
    imgasm_input_t* inputs;
    imgasm_frame_t* frames;
	
	int input_count;
    int frame_count;
} imgasm_template_t;

#define IMGASM_LOAD_DEFAULT 0x00
#define IMGASM_LOAD_FULL    0x01

int imgasm_load_template(const char* filepath, int flags, imgasm_template_t* template);
int imgasm_load_template_fd(FILE* fd, int flags, imgasm_template_t* template);

int imgasm_load_frame(FILE* fd, imgasm_template_t* template, int frame_index);

void imgasm_clear_template(imgasm_template_t* template);

#endif // IMGASM_TEMPLATE_H