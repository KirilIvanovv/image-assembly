#include "assembly.h"
#include "bytecode.h"
#include "error.h"

typedef struct {
    int r;
    int g;
    int b;
    int a;
} register_t;

int imgasm_assemble_image(imgasm_template_t* template, int frame, imgasm_assembly_list_t* images) {
    if (!template || !images) {
        return imgasm_set_error(IMGASM_ERROR_INVALID_ASSEMBLY_LIST, "Invalid arguments");
    }
    
    if (frame < 0 || frame >= template->frame_count) {
        return imgasm_set_error(IMGASM_ERROR_INVALID_ASSEMBLY_LIST,
                                "Invalid frame index %d, index allowed between 0 and %d",
                                frame, template->frame_count);
    }
    
    if (images->image_count != template->input_count) {
        return imgasm_set_error(IMGASM_ERROR_INVALID_ASSEMBLY_LIST,
                                "Invalid image list length %d, needs to be %d",
                                images->image_count, template->input_count);
    }
    
    for (int i = 0; i < images->image_count; i++) {
        int pixels_in_frame = template->inputs[i].width * template->inputs[i].height;
        int pixels_in_input = (images->images[i].width * images->images[i].height);
        
        if (pixels_in_frame != pixels_in_input) {
            return imgasm_set_error(IMGASM_ERROR_INVALID_ASSEMBLY_IMAGE,
                                    "Invalid input index %d size %d, needs to be %d",
                                    i, pixels_in_input, pixels_in_frame);
        }
    }
    
    imgasm_frame_t* frm = &template->frames[frame];
    
    if (!frm->bytecode) {
        return imgasm_set_error(IMGASM_ERROR_INVALID_ASSEMBLY_IMAGE,
                                "Frame %d bytecode not loaded", frame);
    }
    
    unsigned char* output = images->images[0].pixels;
    int pixel_count = frm->width * frm->height;
    unsigned char* bytecode = (unsigned char*)frm->bytecode;
    int byte_index = 0;
    
    register_t registers[4];
    
    for (int pixel = 0; pixel < pixel_count; pixel++) {
        for (int i = 0; i < 4; i++) {
            registers[i].r = 0x00;
            registers[i].g = 0x00;
            registers[i].b = 0x00;
            registers[i].a = 0x00;
        }
        
        for (;;) {
            unsigned char operation = bytecode[byte_index];
            byte_index++;
            
            switch (operation) {
                case IMGASM_OP_NOOP:
                    break;
                    
                case IMGASM_OP_MOVE: {
                    unsigned char param = bytecode[byte_index];
                    byte_index++;
                    
                    unsigned char destination_register = param >> 4;
                    unsigned char source_register = param & 0x0F;
                    
                    registers[destination_register].r = registers[source_register].r;
                    registers[destination_register].g = registers[source_register].g;
                    registers[destination_register].b = registers[source_register].b;
                    registers[destination_register].a = registers[source_register].a;
                    break;
                }
                
                case IMGASM_OP_RETURN:
                    break;
                    
                case IMGASM_OP_CONSTANT: {
                    unsigned char param = bytecode[byte_index];
                    byte_index++;
                    
                    unsigned char destination_register = param >> 4;
                    
                    registers[destination_register].r = bytecode[byte_index + 0];
                    registers[destination_register].g = bytecode[byte_index + 1];
                    registers[destination_register].b = bytecode[byte_index + 2];
                    registers[destination_register].a = bytecode[byte_index + 3];
                    
                    byte_index += 4;
                    break;
                }
                
                case IMGASM_OP_SAMPLE: {
                    unsigned char param = bytecode[byte_index];
                    byte_index++;
                    
                    unsigned char destination_register = param >> 4;
                    unsigned char source_input = param & 0x0F;
                    
                    unsigned short x = bytecode[byte_index + 0] | (bytecode[byte_index + 1] << 8);
                    unsigned short y = bytecode[byte_index + 2] | (bytecode[byte_index + 3] << 8);
                    
                    byte_index += 4;
                    
                    imgasm_image_t* source_image = &images->images[source_input];
                    int source_pixel = source_image->width * y + x;
                    int source_index = source_pixel * 4;
                    
                    registers[destination_register].r = source_image->pixels[source_index + 0];
                    registers[destination_register].g = source_image->pixels[source_index + 1];
                    registers[destination_register].b = source_image->pixels[source_index + 2];
                    registers[destination_register].a = 0xFF;
                    break;
                }
                
                case IMGASM_OP_ADD: {
                    unsigned char param = bytecode[byte_index];
                    byte_index++;
                    
                    unsigned char destination_register = param >> 4;
                    unsigned char source_register = param & 0x0F;
                    
                    registers[destination_register].r += registers[source_register].r;
                    registers[destination_register].g += registers[source_register].g;
                    registers[destination_register].b += registers[source_register].b;
                    break;
                }
                
                case IMGASM_OP_MULTIPLY: {
                    unsigned char param = bytecode[byte_index];
                    byte_index++;
                    
                    unsigned char destination_register = param >> 4;
                    unsigned char source_register = param & 0x0F;
                    
                    registers[destination_register].r *= registers[source_register].r;
                    registers[destination_register].g *= registers[source_register].g;
                    registers[destination_register].b *= registers[source_register].b;
                    
                    registers[destination_register].r >>= 8;
                    registers[destination_register].g >>= 8;
                    registers[destination_register].b >>= 8;
                    break;
                }
                
                default:
                    break;
            }
            
            if (operation == IMGASM_OP_RETURN) {
                break;
            }
        }
        
        if (registers[0].r > 255) registers[0].r = 255;
        if (registers[0].g > 255) registers[0].g = 255;
        if (registers[0].b > 255) registers[0].b = 255;
        if (registers[0].a > 255) registers[0].a = 255;
        
        output[pixel * 4 + 0] = (unsigned char)registers[0].r;
        output[pixel * 4 + 1] = (unsigned char)registers[0].g;
        output[pixel * 4 + 2] = (unsigned char)registers[0].b;
        output[pixel * 4 + 3] = 0xFF;
    }
    
    imgasm_clear_error();
    return IMGASM_ERROR_NONE;
}