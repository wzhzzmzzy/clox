## Chaptor 16: Scanning on Demand

这一章主要实现的内容是：
- 真实的解释器入口逻辑，包含 REPL 和代码文本输入（`main.h`）
- 扫描 Lox 代码，解析为 Token 串的扫描器（`scanner.h`）
- 暂时还没有任何编译逻辑的编译器（`compiler.h`）

### 扫描器模块

本章在原本的 interpret 函数直接调用 vm 逻辑中，添加了完整流程中的关键部分：Scanner 和 Compiler。

目前 Compiler 还没有实现，只是写了一些占位逻辑。本章重点在实现 Scanner 上。整体的 Scanner 实现和 jlox 类似，通过字符串比对，Switch Case 逻辑，确定当前字符对应的 Token 类型和字符位置。