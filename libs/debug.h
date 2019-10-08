#ifndef INCLUDE_DEBUG_H_
#define INCLUDE_DEBUG_H_

/* do nothing, just for break when debug with gdb */
void break_point(void);

void panic(char *s);
void panic_asm(int number);

void debug_cursor();
void debug_wait(int wait_time);

#endif