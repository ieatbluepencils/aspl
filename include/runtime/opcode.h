#ifndef RUNTIME_OPCODE_H
#define RUNTIME_OPCODE_H

enum OpCode {
    // integer addition
    OP_IADD = 0, 
    // integer subtraction
    OP_ISUB = 1,
    // integer multplication
    OP_IMUL = 2,
    // integer divison
    OP_IDIV = 3,
    // load constant from constant pool 
    // takes operand of size uint8_t
    OP_LOADCONST = 4,
    // halt execution
    OP_HALT = 5,
    // print integer
    OP_IPRINT = 6,
    // store integer variable
    OP_ISTORELOCAL = 7,
    // load integer variable
    OP_ILOADLOCAL = 8,
    // compare if 2 integers are equal
    OP_ICMP_EQ = 9,
    // compare if 2 integers are not equal
    OP_ICMP_NE = 10,
    // comapre if int 1 is less than int 2
    OP_ICMP_LT = 11,
    // comapre if int 1 is less than or equal to int 2
    OP_ICMP_LE = 12,
    // compare if int 1 is greter than int 2
    OP_ICMP_GT = 13,
    // compare if int 1 is greater than or equal to int 2
    OP_ICMP_GE = 14,
    // negate bool 
    OP_BNOT = 15,
    // logical and for bool
    OP_BAND = 16,
    // logical or for bool
    OP_BOR = 17,
    // unconditional jump
    // takes uint8_t operand of offset to jump to relative to current position
    OP_JMP = 18,
    // jump if false
    // condition jump
    // jump if not equal i.e. if the value at the top of the stack is false
    // takes uint8_t operand of offset to jump to relative to current position
    OP_JIF = 19,
    // integer negation
    OP_INEGATE = 20,
    // pop 
    // pops top value off the stack and discards the results
    // used in stmtexpr
    OP_POP = 21,
};

#endif // RUNTIME_OPCODE_H
