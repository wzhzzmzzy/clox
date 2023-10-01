#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"

/**
 * @brief 编译代码文本，生成 Token
 * 
 * @param source 
 */
ObjFunction* compile(const char* source);

/**
 * @brief 递归标记所有 Compiler 关联的 function
 * 
 */
void markCompilerRoots();

#endif