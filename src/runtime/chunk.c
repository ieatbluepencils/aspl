#include "runtime/chunk.h"

#include <stdlib.h>
#include <stdio.h>
#include "io/ioutils.h"

#define BYTECODEARRAY_INITIAL_CAPACITY 16

void bytecodearray_init(struct BytecodeArray *bytecodearray) {
    bytecodearray->capacity = BYTECODEARRAY_INITIAL_CAPACITY;
    bytecodearray->size = 0;
    bytecodearray->data = malloc(sizeof(Byte) * bytecodearray->capacity);
    bytecodearray->lines = malloc(sizeof(size_t) * bytecodearray->capacity);
    if (!bytecodearray->data || !bytecodearray->lines) {
        eprintf("memory allocation failure\n");
        exit(1);
    }
}

void bytecodearray_write(struct BytecodeArray *bytecodearray, Byte value, size_t line) {
    if (bytecodearray->size >= bytecodearray->capacity) {
        size_t new_capacity = bytecodearray->capacity * 2;
        Byte *tmp = realloc(bytecodearray->data, new_capacity * sizeof(Byte));
        if (!tmp) {
            eprintf("memory allocation failure\n");
            exit(1);
        }
        bytecodearray->data = tmp;
        size_t *tmp1 = realloc(bytecodearray->lines, new_capacity * sizeof(size_t));
        if (!tmp1) {
            eprintf("memory allocation failure\n");
            exit(1);
        }
        bytecodearray->lines = tmp1;
        bytecodearray->capacity = new_capacity;
    }
    bytecodearray->data[bytecodearray->size] = value;
    bytecodearray->lines[bytecodearray->size] = line;
    bytecodearray->size++;
}

void bytecodearray_free(struct BytecodeArray *bytecodearray) {
    free(bytecodearray->data);
    free(bytecodearray->lines);
    bytecodearray->data = NULL;
    bytecodearray->lines = NULL;
    bytecodearray->size = 0;
    bytecodearray->capacity = 0;
}

void chunk_init(struct Chunk *chunk) {
    struct BytecodeArray *bytecode = malloc(sizeof(struct BytecodeArray));
    struct ValueArray *constants = malloc(sizeof(struct ValueArray));
    if (!bytecode || !constants) {
        eprintf("memory allocation failure\n");
        exit(1);
    }
    bytecodearray_init(bytecode);
    valuearray_init(constants);
    chunk->bytecode = bytecode;
    chunk->constants = constants;
}

void chunk_free(struct Chunk *chunk) {
    bytecodearray_free(chunk->bytecode);
    valuearray_free(chunk->constants);
    free(chunk->bytecode);
    free(chunk->constants);
    chunk->bytecode = NULL;
    chunk->constants = NULL;
}

void chunk_write(struct Chunk *chunk, Byte value, size_t line) {
    bytecodearray_write(chunk->bytecode, value, line);
}

size_t chunk_addconstant(struct Chunk *chunk, Value value) {
    size_t index = chunk->constants->size;
    valuearray_write(chunk->constants, value);
    return index;
}

void dump_chunk(struct Chunk *chunk) {
    printf("DEBUG: PRINT CHUNK:\n");
    printf("BYTECODE: ");
    for (size_t i = 0; i < chunk->bytecode->size; ++i) {
        printf("%d, ", (int)chunk->bytecode->data[i]);
    }
    printf("\nCONSTANTS: ");
    for (size_t i = 0; i < chunk->constants->size; ++i) {
        printf("as int: %d, ", chunk->constants->data[i].int_value);
        printf("as bool: %d, ", chunk->constants->data[i].bool_value);
    }
    printf("\n");
}
