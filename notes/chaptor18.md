## Chaptor 18: Types of Values

这一章主要实现的内容是：
- 支持了通过 Union 和 Type tag 实现的 Value 类型（`value.h`）
- 添加了新的类型和相关的计算逻辑（`vm.h`、`compiler.c`、`debug.c`）

### 数值类型实现

这一章的开头，扩展了 Value 类型。之前 Value 类型就是 double 的别名，所以我们的代码只能输入整数和浮点数，以及常规的四则运算。扩充后的 Value 类型，由一个 Type Tag 和一个 Union 组成，它能够表示 Number、Boolean 和 Nil 三种类型。然后通过一系列的宏来包装和解包。

```c
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as; 
} Value;
```

Union 类型的特征是，多个类型共用一块内存，这样可以节省内存，但代价就是不太安全。因此我们需要在运行时对类型做检查，这块检查位于 `run() - vm.c`。

### 扩充布尔类型和 Nil 类型

布尔类型求值可以完全复用 C 语言的操作符，所以也就可以复用之前四则运算的求值逻辑。Nil 和他的名字一样，就是空空的类型，所以不需要存储任何数值。

这里值得一提的是对于比较运算符的处理，我们只处理了 ==、<、>这三个运算符，对于组合起来的 !=、 <=、 >= 这三个运算符，可以通过语法糖的方式处理。也就是 a != b 实现为 !(a==b) 即可。
