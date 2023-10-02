#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

/**
 * @brief 
 * 根据 Key 找到哈希表的指定行数，方法是按哈希表的容量取模，
 * 如果位置上的 key 与当前 key 对应，则返回，否则 行数++；
 * 为了避免死循环，这里假设最后会遇到一个空 Entry，并返回它；
 * 此处特殊处理了删除条目的情况，被删除的条目会被记录为墓碑，在查找时跳过，并作为空条目返回
 * 
 * @param entries HashTable 内存储的 KV 对数组
 * @param capacity HashTable 容量，用于取模
 * @param key 需要查找的 key
 */
static Entry* findEntry(Entry* entries, int capacity, ObjString* key) {
  uint32_t index = key->hash & (capacity - 1);
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = &entries[index];
    // key 为空，需要检查是否是空条目
    if (entry->key == NULL) {
      // 进入了空条目，优先返回墓碑位置
      if (IS_NIL(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      }
      // 非空，此处是墓碑，需要记录下来
      else {
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      return entry;
    }

    index = (index + 1) & (capacity - 1);
  }
}

/**
 * @brief 
 * 按照新的容量重新分配哈希表的 Entry 数组大小，
 * 并将老数据迁移到新的数组内
 * 
 * @param table 哈希表指针
 * @param capacity 新的容量
 */
static void adjustCapacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // 重建索引时，为了不统计墓碑，这里重新对条目数量计数
  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key == NULL) continue;

    Entry* dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableGet(Table* table, ObjString* key, Value* value) {
  if (table->count == 0) return false;

  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

bool tableSet(Table* table, ObjString* key, Value value) {
  // 达到容量限制时，对 Entry 数组容量进行扩展，并将老数据迁移到新数组中
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  // 覆盖墓碑条目时，不增加条目计数
  if (isNewKey && IS_NIL(entry->value)) table->count++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}

bool tableDelete(Table* table, ObjString* key) {
  if (table->count == 0) return false;

  // 尝试找到这一条
  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  // 放置一个特殊值在这里
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash) {
  if (table->count == 0) return NULL;

  uint32_t index = hash & (table->capacity - 1);
  for (;;) {
    Entry* entry = &table->entries[index];
    if (entry->key == NULL) {
      // Stop if we find an empty non-tombstone entry.
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0) {
      // We found it.
      return entry->key;
    }

    index = (index + 1) & (table->capacity - 1);
  }
}

void tableRemoveWhite(Table* table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (entry->key != NULL && !entry->key->obj.isMarked) {
      tableDelete(table, entry->key);
    }
  }
}

void markTable(Table* table) {
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    markObject((Obj*)entry->key);
    markValue(entry->value);
  }
}