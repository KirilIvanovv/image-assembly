#include "bytecode.h"

int imgasm_op_length(char operation) {
    switch (operation) {
        case IMGASM_OP_NOOP:     return 0;
        case IMGASM_OP_MOVE:     return 1;
        case IMGASM_OP_RETURN:   return 0;
        case IMGASM_OP_CONSTANT: return 5;
        case IMGASM_OP_SAMPLE:   return 5;
        case IMGASM_OP_ADD:      return 1;
        case IMGASM_OP_MULTIPLY: return 1;
        default:                 return -1;
    }
}

const char* imgasm_op_name(char operation) {
    switch (operation) {
        case IMGASM_OP_NOOP:     return "NOOP";
        case IMGASM_OP_MOVE:     return "MOVE";
        case IMGASM_OP_RETURN:   return "RET ";
        case IMGASM_OP_CONSTANT: return "LOAD";
        case IMGASM_OP_SAMPLE:   return "SMPL";
        case IMGASM_OP_ADD:      return "ADD ";
        case IMGASM_OP_MULTIPLY: return "MULT";
        default:                 return "INVD";
    }
}