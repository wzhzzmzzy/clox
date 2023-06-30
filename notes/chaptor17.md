## Chaptor 17: Compiling Expressions

这一章主要实现的内容是：
- 将源码处理为字节码的编译器（`compiler.h`）

### 编译器模块

编译器模块的入口是 `compile()`，输出字符串源码，输出字节码数组 Chunk。它也只导出这一个文件。目前编译只处理一个表达式，然后就会 EOF。

