## 启动过程

- 上电后PC置为0xbfc0_0000，该处为pmon，pmon会读取sd卡第一个扇区的代码进行执行
- arch/mips/boot/bootblock.S: 位于image的第一个扇区，由pmon加载，负责加载kernel到内存上并执行kernel
- init/main.c: 位于image的第二个扇区及之后，kernel主函数，负责初始化系统，系统初始化结束后开中断，在时钟中断触发时调度到其他程序

## 基本模块

- arch/mips
  - boot: bootblock，kernel大小由createimage动态填充
  - kernel: kernel相关汇编实现，主要包括上下文切换和异常处理
  - lock: ll - sc实现的fetch_and_set
  - pmon: 调用pmon实现的库函数
- drivers: mac驱动和串口驱动
- init: kernel主函数，负责初始化系统
- kernel
  - exception:
    - irq: 处理硬件中断，包括网卡和时钟中断
    - syscall: 系统调用
    - tlb: 处理tlb异常
  - fs
    - fs: 文件系统实现
  - lock
    - barrier: 屏障实现
    - cond: 信号量实现
    - lock: 互斥锁、自旋锁实现
    - sem: 信号量实现
  - mm
    - memory: 页表和物理内存管理
  - sched
    - sched: 负责进程调度和管理
    - queue: 队列实现
    - time: 时间记录和查询
  - software
    - io_proc: 负责swap的读入和写出
    - run: 负责加载和执行可执行文件
    - shell: 负责解析和执行用户命令
