#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

typedef struct {
  ObjString* key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry* entries;
} Table;

void initTable(Table* table);
/**
 * @brief 
 * 清理 HashTable 内存储的 Entry 数组，
 * 注意不会清除 Entry 数组内的 ObjString
 */
void freeTable(Table* table);

/**
 * @brief
 * 尝试查找 key 对应的 value，并将其放在 Value* 中
 * 
 * @param table HashTable
 * @param key
 * @param value 返回的 value 会被放置在这里
 * 
 * @return 是否找到了 value
 */
bool tableGet(Table* table, ObjString* key, Value* value);

/**
 * @brief 用于修改 Table 的函数
 * 
 * @param  table HashTable指针
 * @param  key   需要添加/修改的 Key 的指针
 * @param  value 任意值
 */
bool tableSet(Table* table, ObjString* key, Value value);

/**
 * @brief 用于删除 Table 中的指定条目
 * 
 * @param  table HashTable指针
 * @param  key   需要删除的 Key 的指针
 */
bool tableDelete(Table* table, ObjString* key);

/**
 * @brief 从 from 中浅拷贝所有数据到 to 中
 * 
 * @param from 源数据 HashTable
 * @param to   需要拷贝的目标 HashTable
 */
void tableAddAll(Table* from, Table* to);

/**
 * @brief 使用原始的 cstring 在哈希表中查找是否有相同的字符串
 * 
 * @param table 存储字符串的哈希表
 * @param chars 待匹配的 cstring
 * @param length cstring 长度
 * @param hash cstring hash
 * 
 * @return 返回相同的字符串或 NULL
 */
ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash);

/**
 * @brief 清理 string set 中的弱引用条目
 * 
 * @param table 
 */
void tableRemoveWhite(Table* table);
/**
 * @brief 标记 Table 中所有引用到的 K-V 对
 * 
 * @param table 
 */
void markTable(Table* table);
#endif