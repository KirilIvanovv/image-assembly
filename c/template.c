#include "template.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

int imgasm_load_template(const char* filepath, int flags, imgasm_template_t* template) {
    if (!filepath || !template) {
        return imgasm_set_error(IMGASM_ERROR_FILE_OPEN_ERROR, "Invalid arguments");
    }
    
    FILE* fd = fopen(filepath, "rb");
    if (!fd) {
        return imgasm_set_error(IMGASM_ERROR_FILE_OPEN_ERROR, "Failed to open file '%s'", filepath);
    }
    
    int result = imgasm_load_template_fd(fd, flags, template);
    fclose(fd);
    
    return result;
}

int imgasm_load_template_fd(FILE* fd, int flags, imgasm_template_t* template) {
    if (!fd || !template) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Invalid arguments");
    }

    memset(template, 0, sizeof(imgasm_template_t));
    
	// TODO: load header
	// TODO: load input infos
	// TODO: load frame infos 
	
	// TODO: if IMGASM_LOAD_FULL flag set, load all frames too!
    
    imgasm_clear_error();
    return IMGASM_ERROR_NONE;
}

int imgasm_load_frame(FILE* fd, imgasm_template_t* template, int frame_index) {
    if (fd == NULL || template == NULL) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Invalid arguments");
    }
    
    if (frame_index < 0 || frame_index >= template->frame_count) {
        return imgasm_set_error(IMGASM_ERROR_FILE_INCORRECT_FRAME,
		                        "Frame index %i out of bounds [0; %i)",
								frame_index, template->frame_count);
    }
    
    imgasm_frame_t* frame = &template->frames[frame_index];
    
	// check if already loaded
    if (frame->bytecode_begin) {
        return IMGASM_ERROR_NONE;
    }
    
    if (fseek(fd, frame->offset, SEEK_SET) != 0) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Failed to seek to frame");
    }
    
    frame->bytecode_begin = malloc(frame->bytecode_length);
    if (!frame->bytecode_begin) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Failed to allocate bytecode buffer");
    }
    
    if (fread(frame->bytecode_begin, 1, frame->bytecode_length, fd) != frame->bytecode_length) {
        free(frame->bytecode_begin);
        frame->bytecode_begin = NULL;
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Failed to read frame bytecode");
    }
    
    return IMGASM_ERROR_NONE;
}

void imgasm_clear_template(imgasm_template_t* template) {
    if (!template) {
        return;
    }
    
    if (template->frames != NULL) {
        for (int i = 0; i < template->frame_count; i++) {
            if (template->frames[i].bytecode_begin != NULL) {
                free(template->frames[i].bytecode_begin);
            }
        }
        free(template->frames);
    }
    
    if (template->inputs != NULL) {
        free(template->inputs);
    }
    
    memset(template, 0, sizeof(imgasm_template_t));
}