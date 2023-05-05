#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
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
 * @brief 初始化动态数组
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
 * @brief 添加元素到动态数组尾部
 * 
 * @param chunk 数组指针
 * @param byte 新元素
 * @param line 行号
 */
void writeChunk(Chunk* chunk, uint8_t byte, int line);
/**
 * @brief 添加元素到常量动态数组尾部
 * 
 * @param chunk Chunk指针
 * @param value 常量值
 * @return int 刚刚添加的常量所在位置
 */
int addConstant(Chunk* chunk, Value value);

#endif