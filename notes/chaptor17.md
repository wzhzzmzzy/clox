## Chaptor 17: Compiling Expressions

这一章主要实现的内容是：
- 将源码处理为字节码的编译器（`compiler.h`）

### 编译器模块

编译器模块的入口是 `compile()`，输出字符串源码，输出字节码数组 Chunk。它也只导出这一个文件。目前编译只处理一个表达式，然后就会 EOF。

编译器的核心函数是 `parsePrecedence()`，其功能是按照指定的优先级解析当前 Token。目前来说，他无法处理比较复杂的语句，只能处理前缀、中缀运算符，也就是一元、二元运算符。解析 Token 依赖 ParserRule 表格，他有四个元素组成，是 TokenType、Prefix、Infix、Precedence。TokenType 用于检索当前 Token 对应的 ParserRule，Prefix 是一元运算符/表达式的编译逻辑，Infix 是二元运算符的编译逻辑，Precedence 是其优先级。

为了通过一个 `parsePrecedence` 编译一整个 `expression`，这里使用了递归来处理。`prefix`和`infix`函数都会在逻辑内再次调用低优先级的`parsePrecedence`，递归树会随着优先级的降低而收敛，直到解析完成，并且避免处理后续更高优先级的表达式。

编译完成后，所有字节码会通过 emitByte 函数输出到 Chunk 当中。