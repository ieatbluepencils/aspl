#include "runtime/vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "io/ioutils.h"
#include "runtime/opcode.h"

void vm_init(struct VM* vm) {
    for (int i = 0; i < 256; i++) {
        vm->locals[i].int_value = 0;
        vm->locals[i].bool_value = 0;
    } 
}

static Byte read_byte(struct VM *vm, struct Chunk *chunk) {
    return chunk->bytecode->data[vm->ip++];
}

static Value pop(struct VM *vm) {
    if (vm->stack.sp == 0) {
        eprintf("stack underflow\n");
        exit(1);        
    }
    return vm->stack.stack[--vm->stack.sp];
}

static void push(struct VM *vm, Value value) {
    if (vm->stack.sp >= STACK_SIZE) {
        eprintf("stack overflow\n");
        exit(1);        
    }
    vm->stack.stack[vm->stack.sp++] = value;
}

static enum OpCode decode_byte(Byte byte) {
    return (enum OpCode)byte;
}

static Value read_constant(struct VM *vm, struct Chunk *chunk) {
    Byte index = read_byte(vm, chunk);
    return chunk->constants->data[index];
}

static Value make_bool(int value) {
    return (Value) { .bool_value = value != 0};
}

static Value make_int(int value) {
    return (Value) { .int_value = value};
}

void vm_run(struct VM *vm, struct Chunk *chunk) {
    vm->stack.sp = 0;
    vm->ip = 0;
    while (true) {
        Byte byte = read_byte(vm, chunk);
        enum OpCode opcode = decode_byte(byte);
        if (opcode == OP_HALT) {
            break;
        }
        switch (opcode) {
            case OP_IADD: {
                // assume all opcodes beginning with i such as iadd and isub to operate on integers
                // guarenteed by compiler
                Value b = pop(vm);
                Value a = pop(vm);
                int i = a.int_value;
                int j = b.int_value;
                Value v = make_int(i + j);
                push(vm, v);
                break;
            }
            case OP_ISUB: {
                Value b = pop(vm);
                Value a = pop(vm);
                int i = a.int_value;
                int j = b.int_value;
                Value v = make_int(i - j);
                push(vm, v);
                break;
            }
            case OP_IMUL: {
                Value b = pop(vm);
                Value a = pop(vm);
                int i = a.int_value;
                int j = b.int_value;
                Value v = make_int(i * j);
                push(vm, v);
                break;
            }
            case OP_IDIV: {
                Value b = pop(vm);
                if (b.int_value == 0) {
                    eprintf("division by 0\n");
                    exit(1);
                }
                Value a = pop(vm);
                int i = a.int_value;
                int j = b.int_value;
                Value v = make_int(i / j);
                push(vm, v);
                break;
            }
            case OP_LOADCONST: {
                Value value = read_constant(vm, chunk);
                push(vm, value);
                break;
            }
            case OP_IPRINT: {
                Value value = pop(vm);
                int ivalue = value.int_value;
                printf("%d\n", ivalue);
                break;
            }
            case OP_ILOADLOCAL: {
                Byte slot = read_byte(vm, chunk);
                push(vm, vm->locals[slot]);
                break;
            }
            case OP_ISTORELOCAL: {
                Byte slot = read_byte(vm, chunk);
                vm->locals[slot] = pop(vm);
                break;
            }
            case OP_ICMP_EQ: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, make_bool(a.int_value == b.int_value));
                break;
            }
            case OP_ICMP_NE: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, make_bool(a.int_value != b.int_value));
                break;
            }
            case OP_ICMP_LT: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, make_bool(a.int_value < b.int_value));
                break;
            }
            case OP_ICMP_LE: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, make_bool(a.int_value <= b.int_value));
                break;
            }
            case OP_ICMP_GT: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, make_bool(a.int_value > b.int_value));
                break;
            }
            case OP_ICMP_GE: {
                Value b = pop(vm);
                Value a = pop(vm);
                push(vm, make_bool(a.int_value >= b.int_value));
                break;
            }
            case OP_BNOT: {
                Value a = pop(vm);
                push(vm, make_bool(a.bool_value == 0));
                break;
            }
            case OP_BAND: {
                Value b = pop(vm);
                Value a = pop(vm);
                if (a.bool_value > 0 && b.bool_value > 0) {
                    push(vm, make_bool(1));
                } else {
                    push(vm, make_bool(0));
                }
                break;
            }
            case OP_BOR: {
                Value b = pop(vm);
                Value a = pop(vm);
                if (a.bool_value > 0 || b.bool_value > 0) {
                    push(vm, make_bool(1));
                } else {
                    push(vm, make_bool(0));
                }
                break;
            }
            case OP_JMP: {
                // vm reinterpret unsigned operand as signed to allow for backwards jumps
                int8_t offset = (int8_t)read_byte(vm, chunk);
                vm->ip += offset;
                break;
            }
            case OP_JIF: {
                Value v = pop(vm);
                // vm reinterpret unsigned operand as signed to allow for backwards jumps
                int8_t offset = (int8_t)read_byte(vm, chunk);
                if (v.bool_value == 0) {
                    vm->ip += offset;
                }
                break;
            }
            case OP_INEGATE: {
                Value v = pop(vm);
                push(vm, make_int(-v.int_value));
                break;
            }
            case OP_POP: {
                pop(vm);
                break;
            }
            default: {
                eprintf("unknown opcode: %d", opcode);
                exit(1);
            } 
        }
    }
}

void vm_free(struct VM *vm) {
    vm->ip = 0; 
}
 