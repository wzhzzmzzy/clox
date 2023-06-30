## Chaptor 16: Scanning on Demand

这一章主要实现的内容是：
- 真实的解释器入口逻辑，包含 REPL 和代码文本输入（`main.h`）
- 扫描 Lox 代码，解析为 Token 串的扫描器（`scanner.h`）
- 暂时还没有任何编译逻辑的编译器（`compiler.h`）

### 扫描器模块

本章在原本的 interpret 函数直接调用 vm 逻辑中，添加了完整流程中的关键部分：Scanner 和 Compiler。

目前 Compiler 还没有实现，只是写了一些占位逻辑。本章重点在实现 Scanner 上。整体的 Scanner 实现和 jlox 类似，通过字符串比对，Switch Case 逻辑，确定当前字符对应的 Token 类型和字符位置。

调用顺序是 initVM - interpret - compile - scanner，scanner 将 Token 返回给 compiler 处理成字节码，再由 vm 执行。

### 扫描逻辑

`scanner.c` 的入口是 `scanToken()`，其逻辑是双指针扫描，start 指向 Token 的起始位置，current 是当前扫描到的位置，最终以 `return makeToken(...)` 结束。当调用 `makeToken()` 时，current 就时当前 Token 的结束位置 +1。

`scanToken()` 会按照不同的起始字母对当前源代码做判断处理，例如数字开头就会调用 `number()` 解析，字母开头调用 `identifier()` 解析，其他符号情况直接使用 switch 语句特判处理。

`identifier()` 中主要是需要特殊处理一些关键字，例如 if else for while 等。使用 switch 语句检查即可。