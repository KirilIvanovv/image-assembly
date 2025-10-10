#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "template.h"
#include "error.h"
#include "bytecode.h"
#include "assembly.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define MSF_GIF_IMPL
#include "msf_gif.h"

typedef struct {
    char* template_file;
    char* input_files[16];
    int input_count;
    char* save_template;
    char* output_file;
    int frame_index;
    int load_full;
    int print_info;
    int print_pixel_x;
    int print_pixel_y;
    int print_pixel_frame;
} cli_options_t;

static void print_usage() {
    printf("Image Assembly CLI Tool\n\n");
    printf("Usage: imgasm [options]\n\n");
    printf("Template Operations:\n");
    printf("  -t <file>       Load template file (.image format)\n");
    printf("  -s <file>       Save template to file (converts/optimizes)\n");
    printf("  --full          Load all frame bytecode immediately\n");
    printf("  --info          Print template information\n");
    printf("  --pixel <frame> <x> <y>  Print bytecode for specific pixel\n\n");
    printf("Image Assembly Operations:\n");
    printf("  -t <file>       Load template file (.image format)\n");
    printf("  -i1 <file>      Load input image for slot 1\n");
    printf("  -i2 <file>      Load input image for slot 2\n");
    printf("  -iN <file>      Load input image for slot N (up to 16)\n");
    printf("  -f <n>          Process only frame N (default: all frames)\n");
    printf("  -o <file>       Output file (.png, .jpg, .bmp, .tga, .gif)\n\n");
    printf("Examples:\n");
    printf("  Show info:            imgasm -t template.image --info\n");
    printf("  Print pixel bytecode: imgasm -t temp.image --pixel 0 16 4\n");
    printf("  Convert template:     imgasm -t input.image -s output.image\n");
    printf("  Assemble single frame: imgasm -t temp.image -i1 img.png -f 0 -o out.png\n");
    printf("  Assemble animation:    imgasm -t temp.image -i1 img.png -o out.gif\n");
}

static imgasm_image_t* load_image(const char* filename, int expected_width, int expected_height) {
    int width, height, channels;
    uint8_t* data = stbi_load(filename, &width, &height, &channels, 4);
    
    if (!data) {
        printf("Failed to load image: %s\n", filename);
        return NULL;
    }
    
    if (expected_width > 0 && expected_height > 0) {
        if (width != expected_width || height != expected_height) {
            printf("Resizing image from %dx%d to %dx%d: %s\n",
                   width, height, expected_width, expected_height, filename);
            
            uint8_t* resized = malloc(expected_width * expected_height * 4);
            if (!resized) {
                printf("Failed to allocate memory for resizing\n");
                return NULL;
            }
            
            float x_ratio = (float)width / expected_width;
            float y_ratio = (float)height / expected_height;
            
            for (int y = 0; y < expected_height; y++) {
                for (int x = 0; x < expected_width; x++) {
                    int src_x = (int)(x * x_ratio);
                    int src_y = (int)(y * y_ratio);
                    int src_idx = (src_y * width + src_x) * 4;
                    int dst_idx = (y * expected_width + x) * 4;
                    
                    resized[dst_idx + 0] = data[src_idx + 0];
                    resized[dst_idx + 1] = data[src_idx + 1];
                    resized[dst_idx + 2] = data[src_idx + 2];
                    resized[dst_idx + 3] = data[src_idx + 3];
                }
            }
            
            stbi_image_free(data);
            data = resized;
            width = expected_width;
            height = expected_height;
        }
    }
    
    imgasm_image_t* image = malloc(sizeof(imgasm_image_t));
    image->width = width;
    image->height = height;
    image->pixels = data;
    
    return image;
}

static int save_image(const char* filename, imgasm_image_t* image) {
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        printf("No file extension specified\n");
        return 0;
    }
    
    if (strcmp(ext, ".png") == 0) {
        return stbi_write_png(filename, image->width, image->height, 4, 
                             image->pixels, image->width * 4);
    } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        return stbi_write_jpg(filename, image->width, image->height, 4, 
                             image->pixels, 90);
    } else if (strcmp(ext, ".bmp") == 0) {
        return stbi_write_bmp(filename, image->width, image->height, 4, 
                             image->pixels);
    } else if (strcmp(ext, ".tga") == 0) {
        return stbi_write_tga(filename, image->width, image->height, 4, 
                             image->pixels);
    } else {
        printf("Unsupported output format: %s\n", ext);
        return 0;
    }
}

static int save_animated_gif(FILE* fd, const char* filename,
                     imgasm_template_t* template, 
                     imgasm_assembly_list_t* inputs, int delay_cs) {
    if (template->frame_count == 0) {
        printf("No frames to save\n");
        return 0;
    }
    
    int width = template->frames[0].width;
    int height = template->frames[0].height;
    
    MsfGifState gifState = {};
    msf_gif_begin(&gifState, width, height);
    
    for (int i = 0; i < template->frame_count; i++) {
        printf("Processing frame %d/%d...\n", i + 1, template->frame_count);
        
		imgasm_load_frame(fd, template, i);
		
        int result = imgasm_assemble_image(template, i, inputs);
        if (result != IMGASM_ERROR_NONE) {
            imgasm_print_error();
            return 0;
        }
        
        if (!msf_gif_frame(&gifState, inputs->images[0].pixels, delay_cs, 16, width * 4)) {
            printf("Failed to add frame %d\n", i);
            return 0;
        }
    }
    
    MsfGifResult result = msf_gif_end(&gifState);
    if (!result.data) {
        printf("Failed to encode GIF\n");
        return 0;
    }
    
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        printf("Failed to open file for writing: %s\n", filename);
        msf_gif_free(result);
        return 0;
    }
    
    fwrite(result.data, result.dataSize, 1, fp);
    fclose(fp);
    
    printf("Saved animated GIF: %s (%d frames)\n", filename, template->frame_count);
    return 1;
}

int main(int argc, char** argv) {
    cli_options_t opts = {0};
    opts.frame_index = -1;
    opts.print_pixel_x = -1;
    opts.print_pixel_y = -1;
    opts.print_pixel_frame = -1;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            opts.template_file = argv[++i];
        } else if (strncmp(argv[i], "-i", 2) == 0 && i + 1 < argc) {
            int slot = atoi(argv[i] + 2);
            if (slot >= 1 && slot <= 16) {
                opts.input_files[slot - 1] = argv[++i];
                if (slot > opts.input_count) opts.input_count = slot;
            }
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            opts.frame_index = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            opts.save_template = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            opts.output_file = argv[++i];
        } else if (strcmp(argv[i], "--full") == 0) {
            opts.load_full = 1;
        } else if (strcmp(argv[i], "--info") == 0) {
            opts.print_info = 1;
        } else if (strcmp(argv[i], "--pixel") == 0 && i + 3 < argc) {
            opts.print_pixel_frame = atoi(argv[++i]);
            opts.print_pixel_x = atoi(argv[++i]);
            opts.print_pixel_y = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage();
            return 1;
        }
    }
    
    if (!opts.template_file) {
        printf("Error: Template file required (-t)\n");
        print_usage();
        return 1;
    }
    
	// SETUP FLAGS
    printf("Loading template: %s\n", opts.template_file);
    imgasm_template_t template;
	
	int flags = IMGASM_LOAD_DEFAULT;
	if (opts.load_full || opts.save_template) flags |= IMGASM_LOAD_FULL;

	// LOAD TEMPLATE
    FILE* fd = fopen(opts.template_file, "rb");
    if (!fd) {
        printf("Failed to open file '%s'", opts.template_file);
		return 1;
    }
	
    if (imgasm_load_template_fd(fd, flags, &template)) {
        imgasm_print_error();
        return 1;
    }
    
    printf("Template loaded: %d inputs, %d frames\n", 
           template.input_count, template.frame_count);
    
	// PRINT INFO
    if (opts.print_info) {
        printf("\n=== Template Information ===\n");
        printf("Inputs: %d\n", template.input_count);
        for (int i = 0; i < template.input_count; i++) {
            printf("  Input %d: %dx%d\n", i, 
                   template.inputs[i].width, template.inputs[i].height);
        }
        printf("\nFrames: %d\n", template.frame_count);
        for (int i = 0; i < template.frame_count; i++) {
            printf("  Frame %d: %dx%d (bytecode: %d bytes)\n", i,
                   template.frames[i].width, template.frames[i].height,
                   template.frames[i].bytecode_length);
        }
        printf("\n");
    }
    
    if (opts.print_pixel_x >= 0) {
		imgasm_load_frame(fd, &template, opts.print_pixel_frame);
		
        printf("\n=== Pixel Bytecode ===\n");
        imgasm_print_bytecode(&template, opts.print_pixel_frame, 
                             opts.print_pixel_x, opts.print_pixel_y);
        printf("\n");
    }
    
    if ((opts.print_info || opts.print_pixel_x >= 0) && 
        !opts.save_template && !opts.output_file) {
        return 0;
    }
    
	// SAVE TEMPLATE
    if (opts.save_template && !opts.output_file) {
        printf("Saving template: %s\n", opts.save_template);
        if (imgasm_save_template(opts.save_template, &template)) {
            imgasm_print_error();
            return 1;
        }
        printf("Template saved successfully\n");
        return 0;
    }
    
	// ASSEMBLE IMAGE
    if (opts.output_file) {
        if (opts.input_count == 0) {
            printf("Error: No input images specified\n");
            return 1;
        }
        
        imgasm_assembly_list_t inputs;
        inputs.image_count = template.input_count;
        inputs.images = calloc(inputs.image_count, sizeof(imgasm_image_t));
        
        inputs.images[0].width = template.frames[0].width;
        inputs.images[0].height = template.frames[0].height;
        inputs.images[0].pixels = calloc(inputs.images[0].width * 
                                        inputs.images[0].height * 4, 1);
        
        for (int i = 1; i < inputs.image_count; i++) {
            if (!opts.input_files[i - 1]) {
                printf("Error: Missing input image for slot %d\n", i);
                return 1;
            }
            
            printf("Loading input %d: %s\n", i, opts.input_files[i - 1]);
            imgasm_image_t* img = load_image(opts.input_files[i - 1],
                                            template.inputs[i].width,
                                            template.inputs[i].height);
            if (!img) {
                return 1;
            }
			
            inputs.images[i] = *img;
        }
        
        const char* ext = strrchr(opts.output_file, '.');
        if (ext && strcmp(ext, ".gif") == 0 && opts.frame_index == -1) {
            save_animated_gif(fd, opts.output_file, &template, &inputs, 5);
        } else {
            int frame = (opts.frame_index > 0) ? opts.frame_index : 0;
			
            imgasm_load_frame(fd, &template, frame);
			
            printf("Assembling frame %d...\n", frame);
            if (imgasm_assemble_image(&template, frame, &inputs)) {
                imgasm_print_error();
            } else {
                printf("Saving output: %s\n", opts.output_file);
				
                if (save_image(opts.output_file, &inputs.images[0])) {
                    printf("Output saved successfully\n");
                } else {
                    printf("Failed to save output image\n");
                }
            }
        }
    }
	
    return 0;
}