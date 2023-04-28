#include <stdlib.h>

#include "memory.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  // newSize == 0 时，清理内存
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  /**
   * realloc 会
   * 当连续内存可用时自动扩展
   * 无连续内存可用时重新申请并复制
   * 无法重新申请时返回 NULL，此时立即退出程序
   */
  void* result = realloc(pointer, newSize);
  if (result == NULL) exit(1);
  return result;
}