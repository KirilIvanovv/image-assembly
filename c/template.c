#include "template.h"
#include "error.h"
#include "bytecode.h"

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
    
    // ============================= LOAD HEADER =============================
    char file_header[8];
    if (fread(file_header, 1, 8, fd) != 8) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read file header");
    }
    
    if (memcmp(file_header, "IMGBCODE", 8) != 0) {
        return imgasm_set_error(IMGASM_ERROR_FILE_INCORRECT_HEADER, 
                                "Invalid file header '%.8s'", file_header);
    }
    
    unsigned short file_type_version, file_flags, input_count, frame_count;
    
    if (fread(&file_type_version, 2, 1, fd) != 1 ||
        fread(&file_flags, 2, 1, fd) != 1 ||
        fread(&input_count, 2, 1, fd) != 1 ||
        fread(&frame_count, 2, 1, fd) != 1) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read file header");
    }
    
    if (file_type_version < 0 || file_type_version > 1) {
        return imgasm_set_error(IMGASM_ERROR_FILE_INCORRECT_VERSION,
                                "This library version can't process files with version '%d'",
                                file_type_version);
    }
    
    if (input_count < 1) {
        return imgasm_set_error(IMGASM_ERROR_FILE_INCORRECT_INPUT_INFO,
                                "File contains %d inputs, but needs at least 1", input_count);
    }
    
    template->flags = file_flags;
    template->input_count = input_count;
    template->frame_count = frame_count;
    
    // ========================== LOAD INPUT INFOS ===========================
    template->inputs = malloc(sizeof(imgasm_input_t) * input_count);
    if (!template->inputs) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Failed to allocate inputs");
    }
    
    for (int i = 0; i < input_count; i++) {
        unsigned char input_index;
        unsigned short input_width, input_height;
        
        if (fread(&input_index, 1, 1, fd) != 1) {
            imgasm_clear_template(template);
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read input info");
        }
        
        if (input_index >= input_count) {
            imgasm_clear_template(template);
            return imgasm_set_error(IMGASM_ERROR_FILE_INCORRECT_INPUT_INFO,
                                    "Incorrect input index %d", input_index);
        }
        
        if (fseek(fd, 3, SEEK_CUR) != 0) {
            imgasm_clear_template(template);
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read input info");
        }
        
        if (fread(&input_width, 2, 1, fd) != 1 ||
            fread(&input_height, 2, 1, fd) != 1) {
            imgasm_clear_template(template);
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read input info");
        }
        
        template->inputs[input_index].width = input_width;
        template->inputs[input_index].height = input_height;
    }
    
    // ========================== LOAD FRAME INFOS ===========================
    template->frames = malloc(sizeof(imgasm_frame_t) * frame_count);
    if (!template->frames) {
        imgasm_clear_template(template);
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Failed to allocate frames");
    }
    memset(template->frames, 0, sizeof(imgasm_frame_t) * frame_count);
    
    for (int i = 0; i < frame_count; i++) {
        char frame_header[4];
        if (fread(frame_header, 1, 4, fd) != 4) {
            imgasm_clear_template(template);
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read frame header");
        }
        
        if (memcmp(frame_header, "FRME", 4) != 0) {
            imgasm_clear_template(template);
            return imgasm_set_error(IMGASM_ERROR_FILE_INCORRECT_FRAME,
                                    "Invalid frame header '%.4s'", frame_header);
        }
        
        unsigned short frame_width, frame_height;
        if (fread(&frame_width, 2, 1, fd) != 1 ||
            fread(&frame_height, 2, 1, fd) != 1) {
            imgasm_clear_template(template);
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read frame");
        }
        
        template->frames[i].width = frame_width;
        template->frames[i].height = frame_height;
        template->frames[i].offset = ftell(fd);
        template->frames[i].bytecode = NULL;
        template->frames[i].bytecode_length = 0;
        
        if (file_type_version >= 1) {
            unsigned long bytecode_size;
            if (fread(&bytecode_size, 4, 1, fd) != 1) {
                imgasm_clear_template(template);
                return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read bytecode size");
            }
            
            template->frames[i].bytecode_length = bytecode_size;
            
            if (flags & IMGASM_LOAD_FULL) {
                if (imgasm_load_frame(fd, template, i) != IMGASM_ERROR_NONE) {
                    imgasm_clear_template(template);
                    return imgasm_get_error_code();
                }
            } else {
                if (fseek(fd, bytecode_size, SEEK_CUR) != 0) {
                    imgasm_clear_template(template);
                    return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, 
                                            "Can't skip bytecode data");
                }
            }
        } else {
            int pixel_count = frame_width * frame_height;
            int bytecode_length = 0;
            
            for (int j = 0; j < pixel_count; j++) {
                for (;;) {
                    unsigned char operation;
                    if (fread(&operation, 1, 1, fd) != 1) {
                        imgasm_clear_template(template);
                        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't read operation");
                    }
                    bytecode_length++;
                    
                    int param_length = imgasm_op_length(operation);
                    if (param_length < 0) {
                        imgasm_clear_template(template);
                        return imgasm_set_error(IMGASM_ERROR_INVALID_OPERATION,
                                                "Invalid operation %x", operation);
                    }
                    
                    if (param_length > 0) {
                        if (fseek(fd, param_length, SEEK_CUR) != 0) {
                            imgasm_clear_template(template);
                            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR,
                                                    "Can't read operation parameters");
                        }
                        bytecode_length += param_length;
                    }
                    
                    if (operation == IMGASM_OP_RETURN) {
                        break;
                    }
                }
            }
            
            template->frames[i].bytecode_length = bytecode_length;
            
            if (flags & IMGASM_LOAD_FULL) {
                if (imgasm_load_frame(fd, template, i) != IMGASM_ERROR_NONE) {
                    imgasm_clear_template(template);
                    return imgasm_get_error_code();
                }
            }
        }
    }
    
    imgasm_clear_error();
    return IMGASM_ERROR_NONE;
}

int imgasm_save_template(const char* filepath, imgasm_template_t* template) {
    if (!filepath || !template) {
        return imgasm_set_error(IMGASM_ERROR_FILE_OPEN_ERROR, "Invalid arguments");
    }
    
    FILE* fd = fopen(filepath, "wb");
    if (!fd) {
        return imgasm_set_error(IMGASM_ERROR_FILE_OPEN_ERROR, "Failed to open file '%s'", filepath);
    }
    
    int result = imgasm_save_template_fd(fd, template);
    fclose(fd);
    
    return result;
}

int imgasm_save_template_fd(FILE* fd, imgasm_template_t* template) {
    if (!fd || !template) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Invalid arguments");
    }
    
    if (template->input_count < 1) {
        return imgasm_set_error(IMGASM_ERROR_INVALID_ASSEMBLY_LIST,
                                "Template must have at least 1 input");
    }
    
    // ============================= WRITE HEADER =============================
    const char file_header[8] = "IMGBCODE";
    if (fwrite(file_header, 1, 8, fd) != 8) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't write file header");
    }
    
    unsigned short file_type_version = 1;
    unsigned short file_flags = template->flags;
    unsigned short input_count = (unsigned short)template->input_count;
    unsigned short frame_count = (unsigned short)template->frame_count;
    
    if (fwrite(&file_type_version, 2, 1, fd) != 1 ||
        fwrite(&file_flags, 2, 1, fd) != 1 ||
        fwrite(&input_count, 2, 1, fd) != 1 ||
        fwrite(&frame_count, 2, 1, fd) != 1) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't write file header");
    }
    
    // ========================== WRITE INPUT INFOS ===========================
    for (int i = 0; i < template->input_count; i++) {
        unsigned char input_index = (unsigned char)i;
        char padding[3] = {0, 0, 0};
        unsigned short input_width = (unsigned short)template->inputs[i].width;
        unsigned short input_height = (unsigned short)template->inputs[i].height;
        
        if (fwrite(&input_index, 1, 1, fd) != 1 ||
            fwrite(padding, 1, 3, fd) != 3 ||
            fwrite(&input_width, 2, 1, fd) != 1 ||
            fwrite(&input_height, 2, 1, fd) != 1) {
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't write input info");
        }
    }
    
    // ========================== WRITE FRAME DATA ============================
    for (int i = 0; i < template->frame_count; i++) {
        imgasm_frame_t* frame = &template->frames[i];
        
        if (!frame->bytecode) {
            return imgasm_set_error(IMGASM_ERROR_INVALID_ASSEMBLY_IMAGE,
                                    "Frame %d bytecode not loaded", i);
        }
        
        const char frame_header[4] = "FRME";
        if (fwrite(frame_header, 1, 4, fd) != 4) {
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't write frame header");
        }
        
        unsigned short frame_width = (unsigned short)frame->width;
        unsigned short frame_height = (unsigned short)frame->height;
        
        if (fwrite(&frame_width, 2, 1, fd) != 1 ||
            fwrite(&frame_height, 2, 1, fd) != 1) {
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't write frame dimensions");
        }
        
        unsigned long bytecode_size = (unsigned long)frame->bytecode_length;
        if (fwrite(&bytecode_size, 4, 1, fd) != 1) {
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't write bytecode size");
        }
        
        if (fwrite(frame->bytecode, 1, frame->bytecode_length, fd) != (size_t)frame->bytecode_length) {
            return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Can't write bytecode data");
        }
    }
    
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
    
    if (frame->bytecode) {
        return IMGASM_ERROR_NONE;
    }
    
    if (fseek(fd, frame->offset, SEEK_SET) != 0) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Failed to seek to frame");
    }
    
    frame->bytecode = malloc(frame->bytecode_length);
    if (!frame->bytecode) {
        return imgasm_set_error(IMGASM_ERROR_FILE_IO_ERROR, "Failed to allocate bytecode buffer");
    }
    
    if (fread(frame->bytecode, 1, frame->bytecode_length, fd) != frame->bytecode_length) {
        free(frame->bytecode);
        frame->bytecode = NULL;
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
            if (template->frames[i].bytecode != NULL) {
                free(template->frames[i].bytecode);
            }
        }
        free(template->frames);
    }
    
    if (template->inputs != NULL) {
        free(template->inputs);
    }
    
    memset(template, 0, sizeof(imgasm_template_t));
}

void imgasm_print_bytecode(imgasm_template_t* template, int frame, int x, int y) {
    if (!template) {
        printf("Invalid template\n");
        return;
    }
    
    if (frame < 0 || frame >= template->frame_count) {
        printf("Invalid frame index %d, index allowed between 0 and %d\n", 
               frame, template->frame_count);
        return;
    }
    
    imgasm_frame_t* frm = &template->frames[frame];
    
    if (x < 0 || x >= frm->width) {
        printf("Invalid frame %d x coordinate %d, coordinate allowed between 0 and %d\n",
               frame, x, frm->width);
        return;
    }
    
    if (y < 0 || y >= frm->height) {
        printf("Invalid frame %d y coordinate %d, coordinate allowed between 0 and %d\n",
               frame, y, frm->height);
        return;
    }
    
    if (!frm->bytecode) {
        printf("Frame %d bytecode not loaded\n", frame);
        return;
    }
    
    int pixel_index = frm->width * y + x;
    int byte_index = 0;
    
    for (int i = 0; i < pixel_index; i++) {
        for (;;) {
            unsigned char operation = frm->bytecode[byte_index];
            int parameter_length = imgasm_op_length(operation);
            
            if (parameter_length < 0) {
                printf("Invalid operation encountered\n");
                return;
            }
            
            byte_index += parameter_length + 1;
            
            if (operation == IMGASM_OP_RETURN) {
                break;
            }
        }
    }
    
    printf("Found at index: %d\n", byte_index);
    
    for (;;) {
        printf("%06d ", byte_index);
        
        unsigned char operation = frm->bytecode[byte_index];
        byte_index++;
        
        switch (operation) {
            case IMGASM_OP_NOOP:
                printf("NOOP");
                break;
                
            case IMGASM_OP_MOVE: {
                unsigned char param = frm->bytecode[byte_index];
                byte_index++;
                
                unsigned char destination_register = param >> 4;
                unsigned char source_register = param & 0x0F;
                
                printf("MOVE %02d, %02d", destination_register, source_register);
                break;
            }
            
            case IMGASM_OP_RETURN:
                printf("RET ");
                break;
                
            case IMGASM_OP_CONSTANT: {
                unsigned char param = frm->bytecode[byte_index];
                byte_index++;
                
                unsigned char destination_register = param >> 4;
                
                unsigned char r = frm->bytecode[byte_index + 0];
                unsigned char g = frm->bytecode[byte_index + 1];
                unsigned char b = frm->bytecode[byte_index + 2];
                unsigned char a = frm->bytecode[byte_index + 3];
                
                byte_index += 4;
                
                printf("LOAD %02d, %02x %02x %02x %02x", destination_register, r, g, b, a);
                break;
            }
            
            case IMGASM_OP_SAMPLE: {
                unsigned char param = frm->bytecode[byte_index];
                byte_index++;
                
                unsigned char destination_register = param >> 4;
                unsigned char source_input = param & 0x0F;
                
                unsigned short sample_x = frm->bytecode[byte_index + 0] | 
                                   (frm->bytecode[byte_index + 1] << 8);
                unsigned short sample_y = frm->bytecode[byte_index + 2] | 
                                   (frm->bytecode[byte_index + 3] << 8);
                
                byte_index += 4;
                
                printf("SMPL %02d, %02d %04d %04d", destination_register, source_input, 
                       sample_x, sample_y);
                break;
            }
            
            case IMGASM_OP_ADD: {
                unsigned char param = frm->bytecode[byte_index];
                byte_index++;
                
                unsigned char destination_register = param >> 4;
                unsigned char source_register = param & 0x0F;
                
                printf("ADD  %02d, %02d", destination_register, source_register);
                break;
            }
            
            case IMGASM_OP_MULTIPLY: {
                unsigned char param = frm->bytecode[byte_index];
                byte_index++;
                
                unsigned char destination_register = param >> 4;
                unsigned char source_register = param & 0x0F;
                
                printf("MULT %02d, %02d", destination_register, source_register);
                break;
            }
            
            default:
                printf("INVD");
                break;
        }
        
        printf("\n");
        
        if (operation == IMGASM_OP_RETURN) {
            break;
        }
    }
}

