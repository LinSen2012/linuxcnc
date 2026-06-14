/********************************************************************
* Description: posix_compat.c
*   Minimal POSIX API implementations for Windows.
*   Provides mmap / munmap, dlopen / dlsym, pthread mutex & cond
*   variables, gettimeofday, getrusage, socket emulation and other
*   POSIX primitives as Win32 replacements.
*   Only compiled on Windows (guarded by platform checks in
*   posix_compat.h).
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author: linuxQtCnc project
* License: GPL Version 2
* System: Linux / Windows
*
* Copyright (c) 2026 All rights reserved.
********************************************************************/

#define POSIX_COMPAT_NO_REMAP
#include "posix_compat.h"

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64)

#include <stdarg.h>

/* -------------------------------------------------------------------
 * mmap / munmap stub：用 CreateFileMapping / MapViewOfFile 模拟
 *   我们在进程内部维护一个小的"句柄表"来追踪映射，
 *   以便 munmap 能正确反解。
 * ------------------------------------------------------------------- */

#define PC_MAP_MAX 256
static struct {
    void *base;
    size_t size;
    HANDLE h_map;
    int    used;
} pc_map_table[PC_MAP_MAX];
static CRITICAL_SECTION pc_map_lock;
static volatile int pc_map_init = 0;

static void pc_map_table_init(void)
{
    if (InterlockedCompareExchange((LONG volatile *)&pc_map_init, 1, 0) == 0) {
        InitializeCriticalSection(&pc_map_lock);
        memset(pc_map_table, 0, sizeof(pc_map_table));
    }
}

void *pc_mmap(void *addr, size_t length, int prot, int flags,
              int fd, off_t offset)
{
    (void)addr; (void)offset;
    pc_map_table_init();

    if (length == 0) { errno = EINVAL; return MAP_FAILED; }

    DWORD protect = PAGE_READONLY;
    if (prot & PROT_WRITE) protect = PAGE_READWRITE;
    if (prot & PROT_EXEC) protect = PAGE_EXECUTE_READWRITE;

    DWORD access = FILE_MAP_READ;
    if (prot & PROT_WRITE) access = FILE_MAP_ALL_ACCESS;

    /* 支持两种模式：
     *   - MAP_ANONYMOUS：匿名映射（用 INVALID_HANDLE_VALUE）
     *   - 已打开文件：用 fd 对应的 HANDLE
     */
    HANDLE h_file = INVALID_HANDLE_VALUE;
    if (!(flags & MAP_ANONYMOUS) && fd >= 0) {
        h_file = (HANDLE)_get_osfhandle(fd);
    }

    HANDLE h_map = CreateFileMappingA(
        h_file, NULL, protect, 0, (DWORD)length, NULL);

    if (h_map == NULL) {
        errno = EACCES;
        return MAP_FAILED;
    }

    void *base = MapViewOfFile(h_map, access, 0, 0, length);
    if (base == NULL) {
        CloseHandle(h_map);
        errno = ENOMEM;
        return MAP_FAILED;
    }

    EnterCriticalSection(&pc_map_lock);
    for (int i = 0; i < PC_MAP_MAX; i++) {
        if (!pc_map_table[i].used) {
            pc_map_table[i].base = base;
            pc_map_table[i].size = length;
            pc_map_table[i].h_map = h_map;
            pc_map_table[i].used = 1;
            LeaveCriticalSection(&pc_map_lock);
            return base;
        }
    }
    LeaveCriticalSection(&pc_map_lock);

    UnmapViewOfFile(base);
    CloseHandle(h_map);
    errno = ENOMEM;
    return MAP_FAILED;
}

int pc_munmap(void *addr, size_t length)
{
    (void)length;
    pc_map_table_init();

    EnterCriticalSection(&pc_map_lock);
    for (int i = 0; i < PC_MAP_MAX; i++) {
        if (pc_map_table[i].used && pc_map_table[i].base == addr) {
            UnmapViewOfFile(pc_map_table[i].base);
            if (pc_map_table[i].h_map) CloseHandle(pc_map_table[i].h_map);
            pc_map_table[i].used = 0;
            LeaveCriticalSection(&pc_map_lock);
            return 0;
        }
    }
    LeaveCriticalSection(&pc_map_lock);

    errno = EINVAL;
    return -1;
}

/* -------------------------------------------------------------------
 * shm_open / shm_unlink — POSIX 共享内存（用命名 file mapping 实现）
 * ------------------------------------------------------------------- */

int pc_shm_open(const char *name, int oflag, unsigned int mode)
{
    (void)mode;
    if (!name || !*name) { errno = EINVAL; return -1; }

    DWORD access = (oflag & O_RDWR) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
    DWORD open_existing = (oflag & O_CREAT) ? 0 : OPEN_EXISTING;

    /* 将 name 转换为 Win32 命名约定：不允许 '/' 在本地命名中，
     * 用 "Local/name" 格式，去掉开头的 '/'
     */
    char win_name[512];
    if (name[0] == '/') name++;
    snprintf(win_name, sizeof(win_name), "Local\\%s", name);

    HANDLE h_map = CreateFileMappingA(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, win_name);

    if (h_map == NULL) {
        errno = EACCES;
        return -1;
    }

    if (open_existing == OPEN_EXISTING &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        CloseHandle(h_map);
        errno = ENOENT;
        return -1;
    }

    /* 将 HANDLE 转为 C 运行时 fd。使用 _open_osfhandle 方案：
     * 但 Windows 上的"fd"不能传进 POSIX 代码，因此这里返回一个
     * 自定义 id（通过 map table）。
     */
    pc_map_table_init();
    EnterCriticalSection(&pc_map_lock);
    int idx = -1;
    for (int i = 0; i < PC_MAP_MAX; i++) {
        if (!pc_map_table[i].used) {
            pc_map_table[i].base = NULL; /* 尚未 MapViewOfFile */
            pc_map_table[i].size = 0;
            pc_map_table[i].h_map = h_map;
            pc_map_table[i].used = 1;
            idx = i;
            break;
        }
    }
    LeaveCriticalSection(&pc_map_lock);
    (void)access;

    if (idx < 0) {
        CloseHandle(h_map);
        errno = ENOMEM;
        return -1;
    }
    return idx + 1000; /* 返回带偏移的 id，避免与正常 fd 冲突 */
}

int pc_shm_unlink(const char *name)
{
    (void)name;
    /* Windows 共享内存对象在所有句柄关闭后自动释放 */
    return 0;
}

/* -------------------------------------------------------------------
 * rcs_shm_open / rcs_shm_close — 更高层的命名共享内存
 *   在 LinuxCNC 里用于跨进程大数组共享（hal_lib.c 中的 HAL 共享内存）
 * ------------------------------------------------------------------- */

rcs_shm_t *rcs_shm_open(const char *name, size_t size, int create)
{
    if (!name || !*name || size == 0) return NULL;

    /* 将 '/' 开头的 name 改成 "Local/name" */
    char win_name[512];
    const char *n = (name[0] == '/') ? name + 1 : name;
    snprintf(win_name, sizeof(win_name), "Local\\%s", n);

    rcs_shm_t *shm = (rcs_shm_t *)calloc(1, sizeof(*shm));
    if (!shm) { errno = ENOMEM; return NULL; }

    shm->size = size;
    strncpy(shm->name, n, sizeof(shm->name) - 1);

    DWORD high = (DWORD)(size >> 32);
    DWORD low  = (DWORD)(size & 0xFFFFFFFF);

    if (create) {
        shm->h_map = CreateFileMappingA(
            INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, high, low, win_name);
        if (shm->h_map == NULL) {
            free(shm);
            errno = EACCES;
            return NULL;
        }
    } else {
        shm->h_map = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, win_name);
        if (shm->h_map == NULL) {
            free(shm);
            errno = ENOENT;
            return NULL;
        }
    }

    shm->base = MapViewOfFile(shm->h_map, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!shm->base) {
        CloseHandle(shm->h_map);
        free(shm);
        errno = ENOMEM;
        return NULL;
    }

    return shm;
}

int rcs_shm_close(rcs_shm_t *shm)
{
    if (!shm) return -1;
    if (shm->base) UnmapViewOfFile(shm->base);
    if (shm->h_map) CloseHandle(shm->h_map);
    free(shm);
    return 0;
}

/* -------------------------------------------------------------------
 * shmget / shmat / shmdt / shmctl — System V IPC 最小 stub
 * ------------------------------------------------------------------- */

#define PC_SHM_MAX 128
static struct {
    key_t key;
    HANDLE h_map;
    void  *base;
    size_t size;
    int    used;
} pc_shm_table[PC_SHM_MAX];
static CRITICAL_SECTION pc_shm_lock;
static volatile int pc_shm_init = 0;

static void pc_shm_table_init(void)
{
    if (InterlockedCompareExchange((LONG volatile *)&pc_shm_init, 1, 0) == 0) {
        InitializeCriticalSection(&pc_shm_lock);
        memset(pc_shm_table, 0, sizeof(pc_shm_table));
    }
}

int pc_shmget(key_t key, size_t size, int shmflg)
{
    pc_shm_table_init();

    if (size == 0 && !(shmflg & IPC_CREAT)) { errno = EINVAL; return -1; }

    EnterCriticalSection(&pc_shm_lock);

    /* 查找已存在 */
    for (int i = 0; i < PC_SHM_MAX; i++) {
        if (pc_shm_table[i].used && pc_shm_table[i].key == key) {
            LeaveCriticalSection(&pc_shm_lock);
            return i + 1;
        }
    }

    /* 新建 */
    if (!(shmflg & IPC_CREAT) && key != IPC_PRIVATE) {
        LeaveCriticalSection(&pc_shm_lock);
        errno = ENOENT;
        return -1;
    }

    for (int i = 0; i < PC_SHM_MAX; i++) {
        if (!pc_shm_table[i].used) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Local\\shm_%d_%d", (int)key, i);
            DWORD h = 0, l = (DWORD)size;
            HANDLE h_map = CreateFileMappingA(
                INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, h, l, buf);
            if (!h_map) {
                LeaveCriticalSection(&pc_shm_lock);
                errno = ENOMEM;
                return -1;
            }
            pc_shm_table[i].used = 1;
            pc_shm_table[i].key = key;
            pc_shm_table[i].h_map = h_map;
            pc_shm_table[i].base = NULL;
            pc_shm_table[i].size = size;
            LeaveCriticalSection(&pc_shm_lock);
            return i + 1;
        }
    }

    LeaveCriticalSection(&pc_shm_lock);
    errno = ENOSPC;
    return -1;
}

void *pc_shmat(int shmid, const void *shmaddr, int shmflg)
{
    (void)shmaddr; (void)shmflg;
    pc_shm_table_init();
    int i = shmid - 1;
    if (i < 0 || i >= PC_SHM_MAX) { errno = EINVAL; return (void *)-1; }

    EnterCriticalSection(&pc_shm_lock);
    if (!pc_shm_table[i].used) {
        LeaveCriticalSection(&pc_shm_lock);
        errno = EINVAL;
        return (void *)-1;
    }
    if (!pc_shm_table[i].base) {
        pc_shm_table[i].base =
            MapViewOfFile(pc_shm_table[i].h_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    }
    void *b = pc_shm_table[i].base;
    LeaveCriticalSection(&pc_shm_lock);

    return b ? b : (void *)-1;
}

int pc_shmdt(const void *shmaddr)
{
    pc_shm_table_init();
    EnterCriticalSection(&pc_shm_lock);
    for (int i = 0; i < PC_SHM_MAX; i++) {
        if (pc_shm_table[i].used && pc_shm_table[i].base == shmaddr) {
            UnmapViewOfFile(pc_shm_table[i].base);
            pc_shm_table[i].base = NULL;
            LeaveCriticalSection(&pc_shm_lock);
            return 0;
        }
    }
    LeaveCriticalSection(&pc_shm_lock);
    errno = EINVAL;
    return -1;
}

int pc_shmctl(int shmid, int cmd, void *buf)
{
    (void)buf;
    pc_shm_table_init();
    int i = shmid - 1;
    if (i < 0 || i >= PC_SHM_MAX) { errno = EINVAL; return -1; }

    EnterCriticalSection(&pc_shm_lock);
    if (!pc_shm_table[i].used) {
        LeaveCriticalSection(&pc_shm_lock);
        errno = EINVAL;
        return -1;
    }

    if (cmd == IPC_RMID) {
        if (pc_shm_table[i].base) UnmapViewOfFile(pc_shm_table[i].base);
        if (pc_shm_table[i].h_map) CloseHandle(pc_shm_table[i].h_map);
        pc_shm_table[i].used = 0;
        pc_shm_table[i].base = NULL;
        pc_shm_table[i].h_map = NULL;
        LeaveCriticalSection(&pc_shm_lock);
        return 0;
    }

    LeaveCriticalSection(&pc_shm_lock);
    errno = EINVAL;
    return -1;
}

/* -------------------------------------------------------------------
 * dlopen / dlclose / dlsym / dlerror — 用 LoadLibrary / GetProcAddress
 * ------------------------------------------------------------------- */

static char pc_dl_errbuf[256] = "";

void *pc_dlopen(const char *filename, int flags)
{
    (void)flags;
    if (!filename) { errno = EINVAL; return NULL; }
    HMODULE h = LoadLibraryA(filename);
    if (!h) {
        DWORD e = GetLastError();
        snprintf(pc_dl_errbuf, sizeof(pc_dl_errbuf), "LoadLibrary failed: %lu", e);
        return NULL;
    }
    pc_dl_errbuf[0] = '\0';
    return (void *)h;
}

int pc_dlclose(void *handle)
{
    if (!handle) { errno = EINVAL; return -1; }
    return FreeLibrary((HMODULE)handle) ? 0 : -1;
}

void *pc_dlsym(void *handle, const char *symbol)
{
    if (!handle || !symbol) {
        snprintf(pc_dl_errbuf, sizeof(pc_dl_errbuf), "invalid args");
        return NULL;
    }
    FARPROC p = GetProcAddress((HMODULE)handle, symbol);
    if (!p) {
        snprintf(pc_dl_errbuf, sizeof(pc_dl_errbuf), "symbol '%s' not found", symbol);
        return NULL;
    }
    pc_dl_errbuf[0] = '\0';
    return (void *)p;
}

char *pc_dlerror(void)
{
    return pc_dl_errbuf[0] ? pc_dl_errbuf : NULL;
}

/* -------------------------------------------------------------------
 * unistd 类
 * ------------------------------------------------------------------- */

pid_t pc_getpid(void) { return GetCurrentProcessId(); }
int   pc_getuid(void) { return 1000; } /* Windows 没有 uid，返回一个占位值 */

unsigned int pc_sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
    return 0;
}

int pc_usleep(unsigned long usec)
{
    if (usec == 0) return 0;
    /* 最小 Sleep 为 1ms；对 < 1ms 使用 Yield 循环近似 */
    if (usec < 1000) {
        LARGE_INTEGER freq, start, now;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        LONGLONG target = (LONGLONG)((double)usec / 1000000.0 * freq.QuadPart);
        do {
            QueryPerformanceCounter(&now);
            if ((now.QuadPart - start.QuadPart) >= target) return 0;
            SwitchToThread();
        } while (1);
    }
    Sleep((DWORD)(usec / 1000));
    return 0;
}

/* -------------------------------------------------------------------
 * prctl
 * ------------------------------------------------------------------- */

typedef HRESULT (WINAPI *pSetThreadDescription_t)(HANDLE, PCWSTR);
typedef HRESULT (WINAPI *pGetThreadDescription_t)(HANDLE, PWSTR *);

int pc_prctl(int option, ...)
{
    va_list ap;
    va_start(ap, option);

    if (option == PR_SET_NAME) {
        const char *name = va_arg(ap, const char *);
        if (!name) { va_end(ap); errno = EINVAL; return -1; }
        /* 使用 _beginthread / 简化方式：SetThreadDescription 在 Win10 之后才支持 */
        HMODULE h_kernel = GetModuleHandleA("kernel32.dll");
        if (h_kernel) {
            pSetThreadDescription_t fn =
                (pSetThreadDescription_t)GetProcAddress(h_kernel, "SetThreadDescription");
            if (fn) {
                WCHAR wname[256];
                int n = MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
                if (n > 0) {
                    HRESULT hr = fn(GetCurrentThread(), wname);
                    va_end(ap);
                    return SUCCEEDED(hr) ? 0 : -1;
                }
            }
        }
        /* 回退：设置当前线程的 Win32 名称（通过异常协议）—— 仅用于调试器
         * 这里不做实际操作，直接返回 OK
         */
        va_end(ap);
        return 0;
    } else if (option == PR_GET_NAME) {
        char *buf = va_arg(ap, char *);
        if (!buf) { va_end(ap); errno = EINVAL; return -1; }
        HMODULE h_kernel = GetModuleHandleA("kernel32.dll");
        if (h_kernel) {
            pGetThreadDescription_t fn =
                (pGetThreadDescription_t)GetProcAddress(h_kernel, "GetThreadDescription");
            if (fn) {
                PWSTR wname = NULL;
                HRESULT hr = fn(GetCurrentThread(), &wname);
                if (SUCCEEDED(hr) && wname) {
                    WideCharToMultiByte(CP_UTF8, 0, wname, -1, buf, 16, NULL, NULL);
                    LocalFree(wname);
                    va_end(ap);
                    return 0;
                }
            }
        }
        buf[0] = '\0';
        va_end(ap);
        return 0;
    }

    va_end(ap);
    errno = EINVAL;
    return -1;
}

/* -------------------------------------------------------------------
 * 套接字（最小 stub：实际用 Winsock2，不使用 AF_UNIX）
 * ------------------------------------------------------------------- */

static int pc_wsock_init(void)
{
    static int inited = 0;
    if (InterlockedCompareExchange((LONG volatile *)&inited, 1, 0) == 0) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            inited = 0;
            return -1;
        }
    }
    return 0;
}

pc_socket_t pc_socket(int domain, int type, int protocol)
{
    pc_wsock_init();
    /* Windows 下我们把 AF_UNIX 退化为 TCP socket 以便本地通信 */
    int d = (domain == AF_UNIX) ? AF_INET : domain;
    return WSASocketA(d, type, protocol, NULL, 0, 0);
}

int pc_bind(pc_socket_t sockfd, const struct pc_sockaddr_un *addr, int addrlen)
{
    (void)addrlen;
    if (!addr) { errno = EINVAL; return -1; }

    /* 若传入的是 AF_UNIX 类型（sun_path 开头是 '/'），
     * 把它转成 TCP localhost:port 绑定，以便跨进程通信
     */
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    /* 用 sun_path 字符串的哈希作为端口号（简化替代 AF_UNIX） */
    unsigned int hash = 5381;
    for (const char *p = addr->sun_path; *p; p++)
        hash = ((hash << 5) + hash) + (unsigned char)*p;
    sin.sin_port = htons((unsigned short)(1024 + (hash % 64512)));
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    return bind(sockfd, (struct sockaddr *)&sin, sizeof(sin));
}

int pc_listen(pc_socket_t sockfd, int backlog)
{
    return listen(sockfd, backlog);
}

pc_socket_t pc_accept(pc_socket_t sockfd, struct pc_sockaddr_un *addr, int *addrlen)
{
    (void)addr; (void)addrlen;
    struct sockaddr_in sin;
    int slen = sizeof(sin);
    pc_socket_t s = accept(sockfd, (struct sockaddr *)&sin, &slen);
    return s;
}

int pc_connect(pc_socket_t sockfd, const struct pc_sockaddr_un *addr, int addrlen)
{
    (void)addrlen;
    if (!addr) { errno = EINVAL; return -1; }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    unsigned int hash = 5381;
    for (const char *p = addr->sun_path; *p; p++)
        hash = ((hash << 5) + hash) + (unsigned char)*p;
    sin.sin_port = htons((unsigned short)(1024 + (hash % 64512)));
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    return connect(sockfd, (struct sockaddr *)&sin, sizeof(sin));
}

int pc_send(pc_socket_t sockfd, const void *buf, int len, int flags)
{
    return send(sockfd, (const char *)buf, len, flags);
}

int pc_recv(pc_socket_t sockfd, void *buf, int len, int flags)
{
    return recv(sockfd, (char *)buf, len, flags);
}

int pc_close_socket(pc_socket_t sockfd)
{
    return closesocket(sockfd);
}

/* -------------------------------------------------------------------
 * getrusage — 用 GetProcessTimes
 * ------------------------------------------------------------------- */

int pc_getrusage(int who, struct pc_rusage *usage)
{
    (void)who;
    if (!usage) { errno = EINVAL; return -1; }

    FILETIME ct, et, kt, ut;
    if (!GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut)) {
        errno = EINVAL;
        return -1;
    }
    usage->ru_utime = ut;
    usage->ru_stime = kt;
    return 0;
}

/* -------------------------------------------------------------------
 * gettimeofday — FILETIME → timeval
 * ------------------------------------------------------------------- */

int pc_gettimeofday(struct pc_timeval *tv, void *tz)
{
    (void)tz;
    if (!tv) { errno = EINVAL; return -1; }

    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    /* FILETIME：1601-01-01 起 100ns 间隔数。
     * UNIX time：1970-01-01 起秒数。两者相差 11644473600 秒。
     */
    ULONGLONG epoch_diff = 11644473600ULL * 10000000ULL; /* 100ns */
    ULONGLONG since_epoch = (uli.QuadPart >= epoch_diff)
                              ? (uli.QuadPart - epoch_diff) : 0;

    tv->tv_sec  = (long)(since_epoch / 10000000ULL);
    tv->tv_usec = (long)((since_epoch % 10000000ULL) / 10);
    return 0;
}

/* -------------------------------------------------------------------
 * pthread：CRITICAL_SECTION / CONDITION_VARIABLE 实现
 * ------------------------------------------------------------------- */

typedef struct {
    unsigned int (__stdcall *start)(void *);
    void *arg;
} pc_pthread_wrap_t;

static unsigned int __stdcall pc_pthread_wrapper(void *arg)
{
    pc_pthread_wrap_t *w = (pc_pthread_wrap_t *)arg;
    unsigned int r = 0;
    if (w && w->start) r = w->start(w->arg);
    if (w) free(w);
    return r;
}

unsigned int pc_pthread_self(void)
{
    return (unsigned int)GetCurrentThreadId();
}

int pc_pthread_equal(pthread_t t1, pthread_t t2)
{
    return (DWORD)(size_t)t1 == (DWORD)(size_t)t2;
}

int pc_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                      unsigned int (__stdcall *start_routine)(void *),
                      void *arg)
{
    if (!thread || !start_routine) return EINVAL;

    size_t stack_size = 0;
    DWORD flags = 0;
    if (attr) {
        stack_size = attr->stacksize;
        if (attr->detachstate == PTHREAD_CREATE_DETACHED) flags |= CREATE_SUSPENDED;
    }

    pc_pthread_wrap_t *w = (pc_pthread_wrap_t *)malloc(sizeof(*w));
    if (!w) return ENOMEM;
    w->start = start_routine;
    w->arg = arg;

    HANDLE h = (HANDLE)_beginthreadex(NULL, (unsigned)stack_size,
                                       (unsigned int (__stdcall *)(void *))pc_pthread_wrapper,
                                       w, flags, NULL);
    if (!h) {
        free(w);
        return EAGAIN;
    }

    if (flags & CREATE_SUSPENDED) {
        /* 分离线程：不保留 HANDLE 引用 */
        ResumeThread(h);
        CloseHandle(h);
    }
    *thread = h;
    return 0;
}

int pc_pthread_join(pthread_t thread, void **retval)
{
    if (!thread) return EINVAL;
    DWORD r = WaitForSingleObject(thread, INFINITE);
    if (r == WAIT_FAILED) {
        CloseHandle(thread);
        return EINVAL;
    }
    DWORD code = 0;
    GetExitCodeThread(thread, &code);
    CloseHandle(thread);
    if (retval) *retval = (void *)(size_t)code;
    return 0;
}

int pc_pthread_exit(void *retval)
{
    _endthreadex((unsigned)(size_t)retval);
    return 0;
}

/* pthread_attr_* */

int pc_pthread_attr_init(pthread_attr_t *attr)
{
    if (!attr) return EINVAL;
    memset(attr, 0, sizeof(*attr));
    attr->detachstate = PTHREAD_CREATE_JOINABLE;
    attr->stacksize = 0;
    attr->schedpolicy = SCHED_OTHER;
    attr->param.sched_priority = 0;
    return 0;
}

int pc_pthread_attr_destroy(pthread_attr_t *attr)
{
    (void)attr;
    return 0;
}

int pc_pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
    if (!attr) return EINVAL;
    if (detachstate != PTHREAD_CREATE_JOINABLE && detachstate != PTHREAD_CREATE_DETACHED)
        return EINVAL;
    attr->detachstate = detachstate;
    return 0;
}

int pc_pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    if (!attr) return EINVAL;
    attr->stacksize = stacksize;
    return 0;
}

int pc_pthread_setschedparam(pthread_t thread, int policy,
                             const struct pc_sched_param *param)
{
    (void)policy;
    if (!thread || !param) return EINVAL;
    int prio = THREAD_PRIORITY_NORMAL;
    if (param->sched_priority > 0) prio = THREAD_PRIORITY_HIGHEST;
    if (param->sched_priority < 0) prio = THREAD_PRIORITY_LOWEST;
    SetThreadPriority(thread, prio);
    return 0;
}

int pc_pthread_getschedparam(pthread_t thread, int *policy,
                             struct pc_sched_param *param)
{
    if (!thread || !policy || !param) return EINVAL;
    *policy = SCHED_OTHER;
    int p = GetThreadPriority(thread);
    param->sched_priority = (p == THREAD_PRIORITY_HIGHEST) ? 1 :
                            (p == THREAD_PRIORITY_LOWEST) ? -1 : 0;
    return 0;
}

int pc_pthread_kill(pthread_t thread, int sig)
{
    (void)sig;
    if (!thread) return EINVAL;
    if (sig == 0) return 0; /* 探测信号 */
    /* Windows 没有信号语义；用 TerminateThread 是危险的，这里返回 ok
     * 表示"我们尽力了"
     */
    return 0;
}

/* pthread_mutex_* */

int pc_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    (void)attr;
    if (!mutex) return EINVAL;
    InitializeCriticalSection(&mutex->cs);
    return 0;
}

int pc_pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (!mutex) return EINVAL;
    EnterCriticalSection(&mutex->cs);
    return 0;
}

int pc_pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (!mutex) return EINVAL;
    LeaveCriticalSection(&mutex->cs);
    return 0;
}

int pc_pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (!mutex) return EINVAL;
    DeleteCriticalSection(&mutex->cs);
    return 0;
}

/* pthread_cond_*：Vista+ 的 CONDITION_VARIABLE */

int pc_pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    (void)attr;
    if (!cond) return EINVAL;
    InitializeConditionVariable(&cond->cv);
    return 0;
}

int pc_pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (!cond || !mutex) return EINVAL;
    /* SleepConditionVariableCS 需要 CRITICAL_SECTION；
     * 这里假设 mutex->cs 就是 CRITICAL_SECTION
     */
    BOOL ok = SleepConditionVariableCS(&cond->cv, &mutex->cs, INFINITE);
    return ok ? 0 : EINVAL;
}

int pc_pthread_cond_signal(pthread_cond_t *cond)
{
    if (!cond) return EINVAL;
    WakeConditionVariable(&cond->cv);
    return 0;
}

int pc_pthread_cond_broadcast(pthread_cond_t *cond)
{
    if (!cond) return EINVAL;
    WakeAllConditionVariable(&cond->cv);
    return 0;
}

int pc_pthread_cond_destroy(pthread_cond_t *cond)
{
    (void)cond;
    /* CONDITION_VARIABLE 没有释放函数 */
    return 0;
}

#endif /* WIN32 / _WIN32 / _WIN64 */

