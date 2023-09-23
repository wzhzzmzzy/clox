## Chaptor 23: Jumping Back and Forth

这一章主要实现的内容：
- 语法部分：`if-else`、`while`、`for`、`and`、`or`
- 指令部分：`OP_JUMP`、`OP_JUMP_IF_FALSE`、`OP_LOOP`

### Jump 指令

作为控制流基础的 `OP_JUMP`、`OP_JUMP_IF_FALSE`。这两个指令的特殊点在于，它们可以修改当前执行指令的位置，也就是我们的`vm.ip`。Jump 指令无法一次性构造完成，需要在开始 Jump 的点调用 `emitJump`，在 Jump 目标点调用 `patchJump`。

#### `emitJump` 和 `patchJump`

```c
/**
 * 输出一个 Jump 指令到 Chunk，
 * 后续的两个 0xff 字节是 Jump 指向的位置，
 * 因此最大的偏移量是 UINT16_MAX
 * 
 * @param instruction Jump 指令
 */
static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}
```

可以看到 Jump 指令后面跟着两个 Byte 长度的操作数，初始化时，我们将其设置为 `0xffff`，然后在 `patchJump` 时修改这个操作数。

```c
/**
 * 根据当前的字节码位置和 offset 计算需要跳转的距离，
 * 并补全 offset 位置 emitJump 的字节码
 * 
 * @param offset OP_JUMP_IF_FALSE 所在位置
 */
static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}
```

我们根据 `patchJump` 时的指令位置，和 `emitJump` 时的指令位置（`offset`），计算出需要跳转的指令数量，并修改前面的操作数。这里可以看到，由于操作数是两个字节的长度，我们最多能一次跳转 `UINT16_MAX` 个指令。

通过 Jump 指令，我们就可以实现 `if-else` 和短路操作符 `and`、`or`。

### Loop 指令

其实 Loop 和 Jump 指令很像，只是 Loop 指令是在 Loop 迭代执行完成一次后添加到字节码序列的，此时我们的 Loop 整体已经编译完成，可以直接计算出距离 Loop 起始点的指令数。只需要在 Loop 开始处记录 `currentChunk()->count`，在结束处调用 `emitLoop` 即可。

### 语法实现

有了指令就可以实现我们的控制流语法了。如果玩过[Human Resource Machine](https://tomorrowcorporation.com/humanresourcemachine)的话，可以非常容易地理解这里的实现。我们可以把 `emitJump` 看做 Jump 箭头的起点，`patchJump` 看做箭头的落点，遇到 `OP_JUMP` 时，会直接跳过其间的所有字节码；遇到 `OP_JUMP_IF_FALSE` 时，则会取出前面的表达式值，根据是否 Falsy 来判断是否跳转。

这里就不细致讲解各语句的实现了，可以使用 `common.h` 中的 `DEBUG_PRINT_CODE` 观察跳转逻辑。

