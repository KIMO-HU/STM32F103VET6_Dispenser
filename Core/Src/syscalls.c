/**
  ******************************************************************************
  * @file    syscalls.c
  * @brief   为 newlib 提供最小系统调用实现 (裸机版)
  ******************************************************************************
  */

#include "stdint.h"
#include "stddef.h"

extern int errno;

char *__env[1] = { 0 };
char **environ = __env;

void initialise_monitor_handles(void)
{
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = 1;
    return -1;
}

void _exit(int status)
{
    (void)status;
    while (1) {}
}

__attribute__((weak)) int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

__attribute__((weak)) int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return len;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, void *st)
{
    (void)file;
    (void)st;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _open(char *path, int flags, ...)
{
    (void)path;
    (void)flags;
    return -1;
}

int _wait(int *status)
{
    (void)status;
    errno = 10;
    return -1;
}

int _unlink(char *name)
{
    (void)name;
    errno = 2;
    return -1;
}

int _times(void *buf)
{
    (void)buf;
    return -1;
}

int _stat(char *file, void *st)
{
    (void)file;
    (void)st;
    return 0;
}

int _link(char *old, char *new)
{
    (void)old;
    (void)new;
    errno = 31;
    return -1;
}

int _fork(void)
{
    errno = 11;
    return -1;
}

int _execve(char *name, char **argv, char **env)
{
    (void)name;
    (void)argv;
    (void)env;
    errno = 12;
    return -1;
}

void __libc_init_array(void)
{
}
