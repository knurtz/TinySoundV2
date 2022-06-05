#ifndef TS_SHELL
#define TS_SHELL

void Shell_Init(void);
void Shell_BufferOverflow(void);
void Shell_Restart(void);
bool Shell_CheckCommand(void);

#endif /* TS_SHELL */
