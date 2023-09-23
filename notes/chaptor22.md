## Chaptor 22: Local Variables

这一章主要实现的内容：
- 扩展了 `var` 指令，支持声明局部变量了；
- 添加了作用域和编译器实例的逻辑；

### 作用域和编译器实例

`Compiler` 类型的声明如下：

```c
/**
 * 局部变量类型
 */ 
typedef struct {
  Token name;
  int depth;
} Local;

/**
 * 编译器实例，其中缓存了编译器状态数据
 */
typedef struct {
  Local locals[UINT8_COUNT]; // 局部变量缓存
  int localCount; // 当前局部变量的数量，也就是当前变量的索引
  int scopeDepth; // 当前作用域深度
} Compiler;
```

这里增加了作用域的概念。`Compiler.locals` 是一个局部变量栈，在一个作用域中，会 Push 局部变量，在离开作用域时，会将其中所有的局部变量都 Pop（Pop 的时候只需要修改 localCount 即可）。这里没有使用指针，所有局部变量都放在内存栈区，保证了语义一致性。

这里需要特殊处理的情况是内层引用外层同名变量的情况，例如：

```
{
    var a = 1;
    {
        var a = a;
    }
}
```

这里我们禁止这种情况，方法是区分声明变量和初始化变量这两个步骤。在声明变量之后 `local.depth = -1`，在初始化之后，我们将其调整为正常值：`local.depth =  current->scopeDepth`。

### 存储和读取局部变量

遍历局部变量缓存，按名称一一匹配即可。

