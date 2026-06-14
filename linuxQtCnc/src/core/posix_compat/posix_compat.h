/********************************************************************
* Description: posix_compat.h
*   POSIX API compatibility layer for Windows (non-POSIX platforms).
*   Provides mmap / munmap, dlopen / dlclose, pthread mutex/cond,
*   getrusage, gettimeofday, unix-domain sockets, semaphores,
*   and other POSIX primitives as drop-in replacements on Windows.
*   Only used when NOT building on Linux/FreeBSD/macOS.
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#ifndef POSIX_COMPAT_H
#define POSIX_COMPAT_H

#if !defined(__linux__) && !defined(__FreeBSD__) && !defined(__APPLE__)
/* =========================================================================
 * 仅在非 POSIX 平台（Windows）下生效
 * 注意：本文件使用 WIN32 宏（MSVC/MinGW 均会定义）
 * ========================================================================= */

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)

#define POSIX_COMPAT_WINDOWS 1

/* -------------------------------------------------------------------
 * Windows 基础头
 * ------------------------------------------------------------------- */
#include <windows.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <sys/stat.h>
#include <sys/types.h>

/* -------------------------------------------------------------------
 * <sys/mman.h> — 内存映射 / POSIX 共享内存
 *   用 CreateFileMapping + MapViewOfFile 实现
 * ------------------------------------------------------------------- */

#ifndef PROT_NONE
#define PROT_NONE     0x00
#endif
#ifndef PROT_READ
#define PROT_READ     0x01
#endif
#ifndef PROT_WRITE
#define PROT_WRITE    0x02
#endif
#ifndef PROT_EXEC
#define PROT_EXEC     0x04
#endif

#ifndef MAP_SHARED
#define MAP_SHARED    0x01
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE   0x02
#endif
#ifndef MAP_FIXED
#define MAP_FIXED     0x04
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x08
#endif
#ifndef MAP_ANON
#define MAP_ANON      MAP_ANONYMOUS
#endif

#ifndef MAP_FAILED
#define MAP_FAILED    ((void *)-1)
#endif

/* mmap: stub — 不推荐在 Windows 下直接调用，用 rcs_shm_open 系列 */
void *pc_mmap(void *addr, size_t length, int prot, int flags,
              int fd, off_t offset);
int  pc_munmap(void *addr, size_t length);

int  pc_shm_open(const char *name, int oflag, unsigned int mode);
int  pc_shm_unlink(const char *name);

/* 简单的"命名共享内存"句柄跟踪（rcs_shm_* 系列）*/
typedef struct rcs_shm_s {
    HANDLE h_map;
    void  *base;
    size_t size;
    char   name[256];
} rcs_shm_t;

rcs_shm_t *rcs_shm_open(const char *name, size_t size, int create);
int        rcs_shm_close(rcs_shm_t *shm);

/* -------------------------------------------------------------------
 * <sys/ipc.h> + <sys/shm.h> — System V IPC（用 Windows file mapping 实现最小映射）
 * ------------------------------------------------------------------- */

typedef int key_t;

#ifndef IPC_CREAT
#define IPC_CREAT   01000
#endif
#ifndef IPC_EXCL
#define IPC_EXCL    02000
#endif
#ifndef IPC_NOWAIT
#define IPC_NOWAIT  04000
#endif
#ifndef IPC_PRIVATE
#define IPC_PRIVATE ((key_t)-1)
#endif
#ifndef IPC_RMID
#define IPC_RMID    0
#endif
#ifndef SHM_R
#define SHM_R       0400
#endif
#ifndef SHM_W
#define SHM_W       0200
#endif

/* shmget / shmat / shmdt / shmctl - 最小 stub */
int pc_shmget(key_t key, size_t size, int shmflg);
void *pc_shmat(int shmid, const void *shmaddr, int shmflg);
int pc_shmdt(const void *shmaddr);
int pc_shmctl(int shmid, int cmd, void *buf);

/* -------------------------------------------------------------------
 * <dlfcn.h> — 动态库加载（用 LoadLibrary/GetProcAddress 实现）
 * ------------------------------------------------------------------- */

#ifndef RTLD_LAZY
#define RTLD_LAZY    0x0001
#endif
#ifndef RTLD_NOW
#define RTLD_NOW     0x0002
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL  0x0100
#endif
#ifndef RTLD_LOCAL
#define RTLD_LOCAL   0x0200
#endif

void *pc_dlopen(const char *filename, int flags);
int   pc_dlclose(void *handle);
void *pc_dlsym(void *handle, const char *symbol);
char *pc_dlerror(void);

/* -------------------------------------------------------------------
 * <unistd.h> — getpid/getuid/sleep/usleep 等
 * ------------------------------------------------------------------- */

pid_t pc_getpid(void);
int   pc_getuid(void);
unsigned int pc_sleep(unsigned int seconds);
int   pc_usleep(unsigned long usec);

/* -------------------------------------------------------------------
 * <sched.h> — 调度策略宏 + sched_param
 * ------------------------------------------------------------------- */

#ifndef SCHED_OTHER
#define SCHED_OTHER    0
#endif
#ifndef SCHED_FIFO
#define SCHED_FIFO     1
#endif
#ifndef SCHED_RR
#define SCHED_RR       2
#endif

struct pc_sched_param {
    int sched_priority;
};

/* -------------------------------------------------------------------
 * <sys/prctl.h> — PR_SET_NAME / PR_GET_NAME（用 SetThreadDescription 替代）
 * ------------------------------------------------------------------- */

#ifndef PR_SET_NAME
#define PR_SET_NAME    15
#endif
#ifndef PR_GET_NAME
#define PR_GET_NAME    16
#endif

int pc_prctl(int option, ...);

/* -------------------------------------------------------------------
 * <sys/socket.h> + <sys/un.h> — Unix domain socket 最小 stub
 *   Windows 原生不支持 AF_UNIX（Win10 17063 以后有 AF_UNIX 但不稳），
 *   这里提供最小 stub。Windows 下实际建议用 TCP localhost。
 * ------------------------------------------------------------------- */

#ifndef AF_UNIX
#define AF_UNIX       1
#endif
#ifndef PF_UNIX
#define PF_UNIX       AF_UNIX
#endif
#ifndef SOCK_STREAM
#define SOCK_STREAM   1
#endif
#ifndef SOCK_DGRAM
#define SOCK_DGRAM    2
#endif

#define PC_SUN_PATH_MAX 108
struct pc_sockaddr_un {
    unsigned short sun_family;
    char           sun_path[PC_SUN_PATH_MAX];
};

typedef SOCKET pc_socket_t;
#define PC_INVALID_SOCKET INVALID_SOCKET

pc_socket_t pc_socket(int domain, int type, int protocol);
int pc_bind(pc_socket_t sockfd, const struct pc_sockaddr_un *addr, int addrlen);
int pc_listen(pc_socket_t sockfd, int backlog);
pc_socket_t pc_accept(pc_socket_t sockfd, struct pc_sockaddr_un *addr, int *addrlen);
int pc_connect(pc_socket_t sockfd, const struct pc_sockaddr_un *addr, int addrlen);
int pc_send(pc_socket_t sockfd, const void *buf, int len, int flags);
int pc_recv(pc_socket_t sockfd, void *buf, int len, int flags);
int pc_close_socket(pc_socket_t sockfd);

/* -------------------------------------------------------------------
 * <sys/resource.h> — getrusage（用 GetProcessTimes 近似）
 * ------------------------------------------------------------------- */

#ifndef RUSAGE_SELF
#define RUSAGE_SELF   0
#endif

struct pc_rusage {
    FILETIME ru_utime; /* user time used */
    FILETIME ru_stime; /* system time used */
};

int pc_getrusage(int who, struct pc_rusage *usage);

/* -------------------------------------------------------------------
 * <sys/time.h> — gettimeofday
 * ------------------------------------------------------------------- */

struct pc_timeval {
    long tv_sec;
    long tv_usec;
};

int pc_gettimeofday(struct pc_timeval *tv, void *tz);

/* -------------------------------------------------------------------
 * <fcntl.h> — O_* 宏（Windows 已有若干定义，补充缺失）
 * ------------------------------------------------------------------- */

#ifndef O_CREAT
#define O_CREAT       0x0100
#endif
#ifndef O_EXCL
#define O_EXCL        0x0400
#endif
#ifndef O_TRUNC
#define O_TRUNC       0x0200
#endif
#ifndef O_RDONLY
#define O_RDONLY      0x0000
#endif
#ifndef O_WRONLY
#define O_WRONLY      0x0001
#endif
#ifndef O_RDWR
#define O_RDWR        0x0002
#endif

/* -------------------------------------------------------------------
 * <signal.h> — SIG* （Windows 已有 SIGTERM/SIGINT/SIGABRT）
 * ------------------------------------------------------------------- */

#ifndef SIGTERM
#define SIGTERM       15
#endif
#ifndef SIGKILL
#define SIGKILL       9
#endif

/* -------------------------------------------------------------------
 * <pthread.h> — 最小 pthread 替代（用 CRITICAL_SECTION / CONDITION_VARIABLE）
 * ------------------------------------------------------------------- */

typedef HANDLE pthread_t;
typedef DWORD pthread_key_t;
typedef struct { CRITICAL_SECTION cs; } pthread_mutex_t;
typedef struct { CONDITION_VARIABLE cv; } pthread_cond_t;
typedef struct { int detachstate; size_t stacksize; int schedpolicy; struct pc_sched_param param; } pthread_attr_t;
typedef struct { int schedpolicy; struct pc_sched_param param; } pthread_mutexattr_t;
typedef int pthread_condattr_t;

#define PTHREAD_MUTEX_INITIALIZER  { (CRITICAL_SECTION) { 0 } }
#define PTHREAD_COND_INITIALIZER   { (CONDITION_VARIABLE) { 0 } }
#define PTHREAD_CREATE_JOINABLE    0
#define PTHREAD_CREATE_DETACHED    1
#define PTHREAD_STACK_MIN          65536
#define PTHREAD_ONCE_INIT          { 0 }
#define PTHREAD_CANCELED           ((void *)-1)

typedef struct { LONG volatile status; } pthread_once_t;

unsigned int pc_pthread_self(void);
int pc_pthread_equal(pthread_t t1, pthread_t t2);
int pc_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                      unsigned int (__stdcall *start_routine)(void *),
                      void *arg);
int pc_pthread_join(pthread_t thread, void **retval);
int pc_pthread_exit(void *retval) __declspec(noreturn);

int pc_pthread_attr_init(pthread_attr_t *attr);
int pc_pthread_attr_destroy(pthread_attr_t *attr);
int pc_pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pc_pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pc_pthread_setschedparam(pthread_t thread, int policy,
                             const struct pc_sched_param *param);
int pc_pthread_getschedparam(pthread_t thread, int *policy,
                             struct pc_sched_param *param);
int pc_pthread_kill(pthread_t thread, int sig);

int pc_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pc_pthread_mutex_lock(pthread_mutex_t *mutex);
int pc_pthread_mutex_unlock(pthread_mutex_t *mutex);
int pc_pthread_mutex_destroy(pthread_mutex_t *mutex);

int pc_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pc_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pc_pthread_cond_signal(pthread_cond_t *cond);
int pc_pthread_cond_broadcast(pthread_cond_t *cond);
int pc_pthread_cond_destroy(pthread_cond_t *cond);

/* -------------------------------------------------------------------
 * <sys/stat.h> / <sys/types.h> — mode_t / off_t
 *   Windows 上 MSVC 已定义 off_t / mode_t
 * ------------------------------------------------------------------- */

#ifndef S_IRUSR
#define S_IRUSR  00400
#endif
#ifndef S_IWUSR
#define S_IWUSR  00200
#endif
#ifndef S_IXUSR
#define S_IXUSR  00100
#endif
#ifndef S_IRWXU
#define S_IRWXU  00700
#endif
#ifndef S_IRGRP
#define S_IRGRP  00040
#endif
#ifndef S_IROTH
#define S_IROTH  00004
#endif

/* -------------------------------------------------------------------
 * 让源文件透明：用宏把标准 POSIX 名映射到 pc_* 实现
 *   这是在 Windows 下"伪装"出 POSIX API 的核心技巧
 * ------------------------------------------------------------------- */

#ifndef POSIX_COMPAT_NO_REMAP

#define mmap(addr, length, prot, flags, fd, offset)  pc_mmap((addr),(length),(prot),(flags),(fd),(offset))
#define munmap(addr, length)                          pc_munmap((addr),(length))
#define shm_open(name, oflag, mode)                   pc_shm_open((name),(oflag),(mode))
#define shm_unlink(name)                              pc_shm_unlink((name))

#define shmget(key, size, shmflg)                     pc_shmget((key),(size),(shmflg))
#define shmat(shmid, shmaddr, shmflg)                 pc_shmat((shmid),(shmaddr),(shmflg))
#define shmdt(shmaddr)                                pc_shmdt((shmaddr))
#define shmctl(shmid, cmd, buf)                       pc_shmctl((shmid),(cmd),(buf))

#define dlopen(filename, flags)                       pc_dlopen((filename),(flags))
#define dlclose(handle)                               pc_dlclose((handle))
#define dlsym(handle, symbol)                         pc_dlsym((handle),(symbol))
#define dlerror()                                     pc_dlerror()

#define getpid()                                      pc_getpid()
#define getuid()                                      pc_getuid()
#define sleep(seconds)                                pc_sleep((unsigned int)(seconds))
#define usleep(usec)                                  pc_usleep((unsigned long)(usec))

#define prctl(option, ...)                            pc_prctl((option), __VA_ARGS__)

#define sched_param                                   pc_sched_param
#define getrusage(who, usage)                         pc_getrusage((who),(usage))
#define gettimeofday(tv, tz)                          pc_gettimeofday((tv),(tz))

#define sockaddr_un                                   pc_sockaddr_un
#define socket(domain, type, protocol)                pc_socket((domain),(type),(protocol))
#define bind(sockfd, addr, addrlen)                   pc_bind((sockfd),(struct pc_sockaddr_un *)(addr),(addrlen))
#define listen(sockfd, backlog)                       pc_listen((sockfd),(backlog))
#define accept(sockfd, addr, addrlen)                 pc_accept((sockfd),(struct pc_sockaddr_un *)(addr),(addrlen))
#define connect(sockfd, addr, addrlen)                pc_connect((sockfd),(struct pc_sockaddr_un *)(addr),(addrlen))
#define send(sockfd, buf, len, flags)                 pc_send((sockfd),(buf),(len),(flags))
#define recv(sockfd, buf, len, flags)                 pc_recv((sockfd),(buf),(len),(flags))
#define close(sockfd)                                 pc_close_socket((sockfd))

/* pthread 重命名 — 注意很多地方 pthread_create 回调签名需匹配 */
#define pthread_t                                     pthread_t
#define pthread_self()                                ((pthread_t)(size_t)pc_pthread_self())
#define pthread_equal(t1, t2)                         pc_pthread_equal((t1),(t2))
#define pthread_create(thread, attr, start, arg)      pc_pthread_create((thread),(attr),(unsigned int (__stdcall *)(void *))(start),(arg))
#define pthread_join(thread, retval)                  pc_pthread_join((thread),(retval))
#define pthread_exit(retval)                          pc_pthread_exit((retval))
#define pthread_attr_init(attr)                       pc_pthread_attr_init((attr))
#define pthread_attr_destroy(attr)                    pc_pthread_attr_destroy((attr))
#define pthread_attr_setdetachstate(attr, s)          pc_pthread_attr_setdetachstate((attr),(s))
#define pthread_attr_setstacksize(attr, sz)           pc_pthread_attr_setstacksize((attr),(sz))
#define pthread_setschedparam(thread, pol, par)       pc_pthread_setschedparam((thread),(pol),(par))
#define pthread_getschedparam(thread, pol, par)       pc_pthread_getschedparam((thread),(pol),(par))
#define pthread_kill(thread, sig)                     pc_pthread_kill((thread),(sig))

#define pthread_mutex_t                               pthread_mutex_t
#define pthread_mutex_init(m, a)                      pc_pthread_mutex_init((m),(a))
#define pthread_mutex_lock(m)                         pc_pthread_mutex_lock((m))
#define pthread_mutex_unlock(m)                       pc_pthread_mutex_unlock((m))
#define pthread_mutex_destroy(m)                      pc_pthread_mutex_destroy((m))

#define pthread_cond_t                                pthread_cond_t
#define pthread_cond_init(c, a)                       pc_pthread_cond_init((c),(a))
#define pthread_cond_wait(c, m)                       pc_pthread_cond_wait((c),(m))
#define pthread_cond_signal(c)                        pc_pthread_cond_signal((c))
#define pthread_cond_broadcast(c)                     pc_pthread_cond_broadcast((c))
#define pthread_cond_destroy(c)                       pc_pthread_cond_destroy((c))

#endif /* !POSIX_COMPAT_NO_REMAP */

/* -------------------------------------------------------------------
 * 内联辅助：几个极简函数直接在此文件内实现
 * ------------------------------------------------------------------- */

static inline int pc_sched_yield(void) {
    SwitchToThread();
    return 0;
}

static inline int pc_clock_gettime(int clk_id, struct timespec *tp) {
    LARGE_INTEGER freq, counter;
    if (!QueryPerformanceFrequency(&freq)) {
        errno = EINVAL;
        return -1;
    }
    QueryPerformanceCounter(&counter);
    tp->tv_sec = (long)(counter.QuadPart / freq.QuadPart);
    tp->tv_nsec = (long)(((counter.QuadPart % freq.QuadPart) * 1000000000LL) / freq.QuadPart);
    return 0;
}

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME           0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC          1
#endif
#define clock_gettime(clk_id, tp) pc_clock_gettime((clk_id),(tp))

/* sched_yield macro remap */
#define sched_yield()  pc_sched_yield()

#endif /* WIN32 / _WIN32 / _WIN64 */

#else

/* 在真正的 POSIX 平台上：什么都不做，保持原有行为 */

#endif /* !__linux__ && !__FreeBSD__ && !__APPLE__ */

#endif /* POSIX_COMPAT_H */

