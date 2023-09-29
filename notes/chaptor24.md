# Chaptor 24: Calls and Functions

这一章主要实现的内容：
- 支持使用`fun`关键字声明函数，每次创建函数时，都会创建一个 Compiler 实例；
- 支持使用`call()`语法调用函数，每次调用函数时，都会创建一个 CallFrame 实例；
- 支持内置使用 C 语言实现的 Native 函数；

## 声明函数

首先被修改的是整个编译器的基础。在 24 章之前，我们会调用 Compiler 将整个脚本编译为一个 Chunk，其存储于 VM 实例当中，这样在解释器运行时，VM 可以直接使用 ip 在整个 Chunk 中不断行走跳跃，执行字节码。

24 章中，我们将 Chunk 存储在 ObjFunction 当中。每当声明一个函数，我们都会创建一个新的 Compiler 实例，将其与新声明的 ObjFunction 相关联，并在 `compile()` 方法执行完成后返回这个 ObjFunction。这样，每一个函数都拥有独立的字节码 Chunk。

对于全局脚本本身，我们将其作为特例的主脚本函数编译，存储为一个匿名的 ObjFunction。

## 调用/执行函数

过去执行字节码时，VM 会递增 ip，顺序执行 chunk 数组中的字节码，在求值栈（`vm.stack`）中增增减减。现在字节码并不存储与 VM 中了，为了处理调用，我们创建了一个新的数据结构 CallFrame，用于描述调用时的状态，相当于从 VM 中将 chunk 和ip 字段抽象出来。

每次调用函数时会创建一个 CallFrame，将其与被调用的函数关联，ip 指向函数的 chunk，slots 指向调用栈中的参数列表开头位置。后续流程基本不用修改，只需要将原本使用的 `vm.ip`、`vm.chunk`、`vm.stack` 修改为 `frame->ip`、`frame->function->chunk`、`frame->slots` 即可。

语法部分，我们新增加了一个`call()`函数，用于处理调用逻辑。