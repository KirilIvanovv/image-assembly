#ifndef IMGASM_BYTECODE_H
#define IMGASM_BYTECODE_H

#define IMGASM_OP_NOOP     0
#define IMGASM_OP_MOVE     2
#define IMGASM_OP_RETURN   3
#define IMGASM_OP_CONSTANT 4
#define IMGASM_OP_SAMPLE   5
#define IMGASM_OP_ADD      8
#define IMGASM_OP_MULTIPLY 9

int imgasm_op_length(char operation);
const char* imgasm_op_name(char operation);

#endif // IMGASM_BYTECODE_H