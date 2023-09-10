## Chaptor 19: String

这一章主要实现的内容：
- 支持了在堆上分配内存的通用数据类型 `Obj`
- 在 `Obj` 的基础上，派生了 `ObjString`，作为 `string` 数据类型
- 实现了基础的内存回收，在 `vm` 执行完成时，调用 `freeVM()`，并回收所有的 `Obj`

### 结构体和继承

在 14 章，我们实现了基础的内存管理工具（`memory.h`）。我们的 `Obj` 同样使用这个方式来分配内存。

在 C 语言中，结构体的内存是顺序分配的。如果将 B 结构体的第一个元素声明为 A 结构体，这样，可以直接将 B 结构体的值直接 cast 为 A。在 Obj 和 ObjString 之间，可以看下面的例子：

```c
struct Obj {
  ObjType type;
  struct Obj* next;
};

struct ObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
};

Obj* obj;
ObjString* str = (ObjString*) obj;
```

通过这种方式，给 Obj 类型设计的方法也可以给 ObjString 类型使用，可以看做 C 语言中的继承能力。

### 字符串类型的内存管理

ObjString 的内存分配方式基本沿用了 14 章的内存管理工具（`memory.h`），在 `object.h` 中进一步封装。这里我们需要特殊处理一下两个字符串相加时的情况，因为他不能简单地使用 C 语言的 `+` 符号了。我们将两个字符串通过 `memcpy` 合并在一块内存当中，然后新建一个 `ObjString` 实例并返回。

### 垃圾回收

这一章只是提供了非常简单的垃圾回收机制：在程序结束时 `freeVM()`，清理 vm 执行程序时，记录的所有 `Obj` 实例，释放所有堆上的内存，避免内存泄漏。

在 `Obj` 上添加一个 `obj->next` 指针，并同时记录在 `vm.objects`上，这个链表就可以记录整个程序执行阶段的所有 `Obj` 实例。需要注意，我们释放 `Obj` 实例时，需要同步释放 `ObjString` 下的所有 `char*`。
