## Chaptor 15: A Virtual Machine

这一章主要实现的内容是：
- 执行字节码指令的虚拟机模块（`vm.h`）
- 增添四则运算相关指令（`chunk.h`）

### 虚拟机模块

前一章实现的，主要是字节码指令、数据的存储数据结构。这一章的虚拟机模块则是真正用于执行字节码的逻辑，也就是编译器的 Backend。

当前的语法较为简单，所以逻辑也非常清晰：VM 实例按照 IP 指针递增读取字节码，依次执行。表达式求值时，将值缓存于 VM 的临时堆栈中，之后的指令可以不断地将栈中的数据取出用于计算。这里简单起见，使用了 256 长度的数组作为求值堆栈，因此，表达式可缓存值的最大数量是 256。这一点目前还不太重要。

为了使逻辑更清晰，将 debug 功能放在了 vm 当中调用（`#ifdef DEBUG_TRACE_EXECUTION`）。

### 课后作业

1. 下面这段表达式的字节码指令串是什么样子？
```
1 * 2 + 3
1 + 2 * 3
3 - 2 - 1
1 + 2 * 3 - 4 / -5
```

2. `OP_NEGATE` or `OP_SUBTRACT`，可以只保留一个，可以尝试一下如果没有其中之一，字节码指令是什么样的