
#define NO_MEM	((phys_clicks) 0)

#if (CHIP===INTEL && _WORD_SIZE == 2)
#define PAGE_SIZE	16
#define MAX_PAGES	4096
#endif

#define printf	printk

#define INIT_PID	1
