# Chaptor 26: Garbage Collection

这一章主要实现的内容：
- 执行垃圾回收逻辑的`collectGarbage()`函数；
- 添加追踪内存使用量的功能，并根据内存使用量决定是否触发垃圾回收；

## 垃圾回收

这里的 GC 策略是经典的标记-收集方法，核心要义是标记所有可以被引用的堆内存对象，然后 free 剩余没有被标记的对象。在申请内存时，我们会将所有对象放进`vm.objects`链表，这里就是对象的全集，然后我们开始标记对象。

标记对象时，我们需要从 root 节点开始，这里的 root 节点指的是堆内存对象本身的直接引用。存储可以直接访问对象的有以下几个位置：

- `vm.stack`栈中的所有 Value；
- `vm.frames`中的所有 CallFrame；
- `vm.openUpvalues`中的所有 Upvalue；
- `vm.globals`中的所有全局变量，和它的 Key；
- 所有的 `compiler` 实例和 `compiler->function`；

标记根节点之后，我们需要标记根节点所引用的相关数据。我们先将所有已经被标记的数据作为灰色节点，表示还需要进一步查找。然后遍历所有灰色节点，找到下属节点。

标记了所有可引用对象后，将剩余对象全部清理即可。

值得一提的是，我们的 string intern 部分，`vm.string` 也需要被处理。可以看到我们之前并没有标记这里面的字符串，因为这部分是【弱引用】，不需要被标记。但弱引用需要在对象被清理后同步清理，不然就会变成空指针。

## 何时触发垃圾收集？

这里使用的方法是，计算当前已经被分配的内存量，当超出设定的限制后，就触发一次垃圾收集，并扩大之前的限制。

## 处理垃圾收集相关的 Bug

当我们扩展 Table 或是 ValueArray 时，需要调用 `reallocate()`，可能会触发 GC，但我们向 Table 和 ValueArray 中新增加的数据并没有被标记，所以他们可能会立即被清除。

这里的解决方法是临时将他们添加到 `vm.stack` 中，通过危险阶段之后再 pop 即可。
