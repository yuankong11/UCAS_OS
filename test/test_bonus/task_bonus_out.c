typedef unsigned int uint32_t;

#define OUTSIDE_CALL_BASE 0xa0fde000

void __attribute__((section(".entry_function"))) _start(void)
{
    typedef void (*sys_move_cursor_p)(int x, int y);
    typedef int (*printf_p)(const char *fmt, ...);
    typedef void (*sys_exit_p)();

    uint32_t *base = (uint32_t *)OUTSIDE_CALL_BASE;
    sys_move_cursor_p p0 = (sys_move_cursor_p)base[0];
    printf_p p1 = (printf_p)base[1];
    sys_exit_p p2 = (sys_exit_p)base[2];

    p0(0, 1);
    p1("Hello, world!");

    p2();
}