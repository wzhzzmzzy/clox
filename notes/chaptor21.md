## Chaptor 21: Global Variables

这一章主要实现的内容：
- 支持了 `var` 指令，可以通过这个指令声明全局变量并初始化；
- 实现了变量存取能力；

### 编译优先级

1. 需要将 `declaration` 语句的优先级置于 `statment` 之上，再在 `statment` 中解析 `expression`，保证解析不会冲突。

2. 为了保证赋值 `=` 的优先级，我们需要判断当前上下文能否解析 `variable`。这里，我们添加了一个 `canAssign = precedence <= PREC_ASSIGNMENT`。

3. 这里我们补完了 `synchronize`，用于在发生编译错误的时候，依然完成整个语句的解析，保证一次编译完整个 Chunk。

### 声明全局变量

1. 声明全局变量时，我们将他的变量名创建为一个 `ObjString` 常量，这样可以顺便利用之前的 string intern 功能。

2. 需要兼容全局变量后续的赋值功能，这里我们在 `varDeclaration` 中特殊处理，解析到 `TOKEN_EQUAL` 时调用 `expression()` 即可。

3. 创建了变量名后，在运行时将变量名存储在 `vm.globals` 中。

### 读取全局变量

读取变量名的逻辑很简单，我们只需要从 `vm.globals` 读取出变量名对应的值即可。