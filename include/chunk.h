#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
} OpCode;

/**
 * @brief 动态数组，用于存储字节码
 */
typedef struct {
  int count; // 数组已用空间
  int capacity; // 数组总占用空间
  uint8_t* code; // 数组指针
  int* lines; // 行号数组指针，与 code 等长
  ValueArray constants; // 常量数组
} Chunk;

/**
 * @brief 初始化动态数组，同时初始化常量数组
 * 
 * @param chunk 数组指针
 */
void initChunk(Chunk* chunk);
/**
 * @brief 释放动态数组空间
 * 
 * @param chunk 数组指针
 */
void freeChunk(Chunk* chunk);
/**
 * @brief 添加字节码到字节码数组尾部
 * 
 * @param chunk 数组指针
 * @param byte 字节码
 * @param line 行号
 */
void writeChunk(Chunk* chunk, uint8_t byte, int line);
/**
 * @brief 添加常量值到常量动态数组尾部
 * 
 * @param chunk Chunk指针
 * @param value 常量值
 * @return int 常量值位于数组的 index
 */
int addConstant(Chunk* chunk, Value value);

#endif