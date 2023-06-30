#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

/**
 * @brief VM 结构体
 */
typedef struct {
  Chunk* chunk; // 字节码内容
  uint8_t* ip; // Instruction Pointer，指向当前执行的字节码
  Value stack[STACK_MAX]; // 表达式求值时临时存储在栈内，初始化长度 256
  Value* stackTop; // 当前的栈顶位置
} VM;

/**
 * @brief 解释执行的结果枚举
 */
typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();
void freeVM();
/**
 * @brief 解释器执行入口，输入源代码，输出执行完成时状态
 * 
 * @param source 代码文本
 * @return InterpretResult 
 */
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif