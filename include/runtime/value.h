#ifndef RUNTIME_VALUE_H
#define RUNTIME_VALUE_H

#include <stddef.h>

typedef union {
    int int_value;
    int bool_value;
} Value;
struct ValueArray {
    Value *data;
    size_t size;
    size_t capacity;
};

void valuearray_init(struct ValueArray *arr);
void valuearray_write(struct ValueArray *arr, Value value);
void valuearray_free(struct ValueArray *arr);

#endif // RUNTIME_VALUE_H
