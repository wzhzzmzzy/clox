#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

/**
 * @brief 编译代码文本，生成 Token
 * 
 * @param source 
 */
bool compile(const char* source, Chunk* chunk);

#endif