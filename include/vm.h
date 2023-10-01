#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

/**
 * @brief 用于跟踪当前调用状态的结构体
 * 
 */
typedef struct {
  ObjClosure* closure; // 调用的闭包
  uint8_t* ip; // 当前正在执行语句的 IP
  Value* slots; // 指向闭包内第一个局部变量槽位
} CallFrame;

/**
 * @brief VM 结构体
 */
typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX]; // 表达式求值时临时存储在栈内，初始化长度 256
  Value* stackTop; // 当前的栈顶位置
  Table globals; // 常量集合
  Table strings; // string intern
  ObjUpvalue* openUpvalues; // 所有 upvalue 集合，保证复用
  
  size_t bytesAllocated;
  size_t nextGC;
  Obj* objects;
  // 用于存储 GC 对象的灰色栈
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
} VM;

/**
 * @brief 解释执行的结果枚举
 */
typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

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