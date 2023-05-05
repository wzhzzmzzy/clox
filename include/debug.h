#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

/**
 * @brief 将字节码反编译出来，输出可阅读的指令
 * 
 * @param chunk 字节码内容
 * @param name 这一段字节码的名称（例如函数名）
 */
void disassembleChunk(Chunk* chunk, const char* name);
/**
 * @brief 输出某一位置的字节码的名称，并递增偏移位置到下一字节码
 * 
 * @param chunk 指令参数
 * @param offset 指令位置
 * @return int 移动 offset 到下一个指令位置
 */
int disassembleInstruction(Chunk* chunk, int offset);

#endif
