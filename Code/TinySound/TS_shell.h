#ifndef TS_SHELL
#define TS_SHELL

#include <stdarg.h>             // for va_list var arg functions

void Shell_Init(void);
void Shell_BufferOverflow(void);
void Shell_Restart(void);
bool Shell_CheckCommand(void);

void xprintf(const char *fmt, ...);

#endif /* TS_SHELL */
