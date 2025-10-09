#ifndef IMGASM_ASSEMBLY_H
#define IMGASM_ASSEMBLY_H

#include "template.h"

typedef struct imgasm_image {
    int width;
    int height;
    unsigned char* pixels;
} imgasm_image_t;

typedef struct imgasm_assembly_list {
    imgasm_image_t* images;
    int image_count;
} imgasm_assembly_list_t;

int imgasm_assemble_image(imgasm_template_t* template, int frame, imgasm_assembly_list_t* images);

#endif // IMGASM_ASSEMBLY_H