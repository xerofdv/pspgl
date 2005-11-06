/**
 *  This file handles all the PSP-specific kernel setup and exit stuff.
 *
 *  Is there some general interest for this file, so that we can place it
 *  somewhere in the compiler toolchain include path? 
 *
 *  Usage: Simply add 
 *            -DMODULE_NAME="your-module-name" psp-setup.c
 *         to the LFLAGS or LDFLAGS of your project, so that this file is
 *         compiled in when gcc collects and links the final ELF binary.
 *
 *  Options:
 *         -DMODULE_NAME="name" -- set the name (default NONAME)
 *         -DMODULE_ATTR=0      -- module attributes (default 0)
 *         -DVERSION_MAJOR=1    -- version 1.x (default 1)
 *         -DVERSION_MINOR=0    -- version x.0 (default 0)
 *
 *  Note:  The linker flags and library lists need to be placed after this
 *         entry on the LFLAG or LDFLAGS command line, otherwise gcc won't
 *         be able to to resolve all symbols.
 */


#if !defined(MODULE_NAME)
	#define MODULE_NAME NONAME
#endif


#if !defined(MODULE_VERSION_MAJOR)
	#define MODULE_VERSION_MAJOR 1
#endif


#if !defined(MODULE_VERSION_MINOR)
	#define MODULE_VERSION_MINOR 0
#endif


#if !defined(MODULE_ATTR)
	#define MODULE_ATTR 0
#endif


/*** we need to declare everything we need locally, we can't include the system includes while linking! ***/

typedef struct _scemoduleinfo {
        unsigned short          modattribute;
        unsigned char           modversion[2];
        char                    modname[27];
        char                    terminal;
        void *                  gp_value;
        void *                  ent_top;
        void *                  ent_end;
        void *                  stub_top;
        void *                  stub_end;
} _sceModuleInfo;

typedef int SceUID;
typedef unsigned int SceSize;
typedef unsigned int SceUInt;
typedef int (*SceKernelCallbackFunction) (int arg1, int arg2, void *arg);
typedef int (*SceKernelThreadEntry) (SceSize args, void *argp);
typedef const _sceModuleInfo SceModuleInfo;

extern int sceKernelRegisterExitCallback(int cbid);
extern void sceKernelExitGame(void);
extern int sceKernelCreateCallback(const char *name, SceKernelCallbackFunction func, void *arg);
extern void sceKernelSleepThreadCB(void);
extern int sceKernelStartThread(SceUID thid, SceSize args, void *argp);

extern char _gp[];

#define PSP_MODULE_INFO(name, attributes, major_version, minor_version) \
        __asm__ (                                                       \
        "    .set push\n"                                               \
        "    .section .lib.ent.top, \"a\", @progbits\n"                 \
        "    .align 2\n"                                                \
        "    .word 0\n"                                                 \
        "__lib_ent_top:\n"                                              \
        "    .section .lib.ent.btm, \"a\", @progbits\n"                 \
        "    .align 2\n"                                                \
        "__lib_ent_bottom:\n"                                           \
        "    .word 0\n"                                                 \
        "    .section .lib.stub.top, \"a\", @progbits\n"                \
        "    .align 2\n"                                                \
        "    .word 0\n"                                                 \
        "__lib_stub_top:\n"                                             \
        "    .section .lib.stub.btm, \"a\", @progbits\n"                \
        "    .align 2\n"                                                \
        "__lib_stub_bottom:\n"                                          \
        "    .word 0\n"                                                 \
        "    .set pop\n"                                                \
        );                                                              \
        extern char __lib_ent_top[], __lib_ent_bottom[];                \
        extern char __lib_stub_top[], __lib_stub_bottom[];              \
        SceModuleInfo module_info                                       \
                __attribute__((section(".rodata.sceModuleInfo"),        \
                               aligned(16), unused)) = {                \
          attributes, { minor_version, major_version }, name, 0, _gp,   \
          __lib_ent_top, __lib_ent_bottom,                              \
          __lib_stub_top, __lib_stub_bottom                             \
        }


#define __stringify(s)	__tostring(s)
#define __tostring(s)	#s

PSP_MODULE_INFO(__stringify(MODULE_NAME), MODULE_ATTR, MODULE_VERSION_MAJOR, MODULE_VERSION_MINOR);


static
int exit_callback (int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}


extern SceUID sceKernelCreateThread (const char *name, SceKernelThreadEntry entry, int initPriority,
				     int stackSize, SceUInt attr, void *option);

static
int update_thread (SceSize args, void *argp)
{
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, (void *) 0);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}


static void setup_callbacks (void) __attribute__((constructor));
static void setup_callbacks (void)
{
	int id;

	if ((id = sceKernelCreateThread("update_thread", update_thread, 0x11, 0xFA0, 0, 0)) >= 0)
		sceKernelStartThread(id, 0, 0);
}



static void back_to_kernel (void) __attribute__((destructor));
static void back_to_kernel (void)
{
	sceKernelExitGame();
}

