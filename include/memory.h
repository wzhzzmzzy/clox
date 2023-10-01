#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// type* + void* = 泛型
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * (oldCount), 0)

/**
 * @brief 
 * 封装的 free() 和 realloc() ；
 * newSize=0 时使用 free 清理内存，
 * 否则按照 newSize 调用 realloc，分配内存失败时 exit(1)
 * 
 * @param pointer 需要分配内存的指针，新建时传 NULL
 * @param oldSize TBD
 * @param newSize 需要的内存大小
 * @return realloc 返回的 void* 指针
 */
void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void markObject(Obj* object);
/**
 * @brief 将当前 Value 标记为正在被引用
 * 
 * @param value 
 */
void markValue(Value value);
void collectGarbage();
/**
 * @brief
 * 释放 VM 中所有的 Obj 对象内存
 */
void freeObjects();

#endif