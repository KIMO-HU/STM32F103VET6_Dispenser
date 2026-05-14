/**
  ******************************************************************************
  * @file    syscalls.c
  * @brief   为 newlib 提供最小系统调用实现
  * @note    此版本为嵌入式简化版，去除对完整标准库的依赖
  ******************************************************************************
  */

#include "stdint.h"
#include "stddef.h"

/* 外部变量 */
extern int errno;
register char * stack_ptr asm("sp");

/* 环境变量 */
char *__env[1] = { 0 };
char **environ = __env;

/* 初始化监控句柄 (保留兼容) */
void initialise_monitor_handles(void)
{
}

/* 获取进程ID */
int _getpid(void)
{
    return 1;
}

/* 终止进程 */
int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = 1;
    return -1;
}

/* 退出程序 */
void _exit(int status)
{
    (void)status;
    while (1) {}
}

/* 读数据 */
__attribute__((weak)) int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

/* 写数据 */
__attribute__((weak)) int _write(int file, char *ptr, int len)
{
    (void)file;
    int DataIdx;
    for (DataIdx = 0; DataIdx < len; DataIdx++)
    {
        /* 可通过 ITM/SWO 或 UART 实现输出 */
    }
    return len;
}

/* 关闭文件 */
int _close(int file)
{
    (void)file;
    return -1;
}

/* 获取文件状态 */
int _fstat(int file, void *st)
{
    (void)file;
    (void)st;
    return 0;
}

/* 检查是否是终端 */
int _isatty(int file)
{
    (void)file;
    return 1;
}

/* 移动文件指针 */
int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

/* 打开文件 */
int _open(char *path, int flags, ...)
{
    (void)path;
    (void)flags;
    return -1;
}

/* 等待子进程 */
int _wait(int *status)
{
    (void)status;
    errno = 10; /* ECHILD */
    return -1;
}

/* 删除文件 */
int _unlink(char *name)
{
    (void)name;
    errno = 2; /* ENOENT */
    return -1;
}

/* 获取时间 */
int _times(void *buf)
{
    (void)buf;
    return -1;
}

/* 获取文件状态 */
int _stat(char *file, void *st)
{
    (void)file;
    (void)st;
    return 0;
}

/* 创建硬链接 */
int _link(char *old, char *new)
{
    (void)old;
    (void)new;
    errno = 31; /* EMLINK */
    return -1;
}

/* 创建进程 */
int _fork(void)
{
    errno = 11; /* EAGAIN */
    return -1;
}

/* 执行程序 */
int _execve(char *name, char **argv, char **env)
{
    (void)name;
    (void)argv;
    (void)env;
    errno = 12; /* ENOMEM */
    return -1;
}

/* 空的 C 库初始化函数 (替代 newlib 的 __libc_init_array) */
void __libc_init_array(void)
{
    /* 在裸机环境中无需初始化 */
}
