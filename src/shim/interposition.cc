#include <fcntl.h>
#include <algorithm>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>

#include <iostream>

#ifdef WITH_KCOV
#include <sys/ioctl.h>
#include <sys/kcov.h>
#include <sys/mman.h>


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#endif

#include "util.h"

extern "C" {
#include "libc_private.h"
#include "netdb_private.h"
}

#define TYPEDEF_INST(return_val, name, ...) \
	typedef return_val (* name)(__VA_ARGS__); \
	name __real_##name = nullptr

// TYPEDEF_INST(char *, getcwd_f, char *, size_t);
// TYPEDEF_INST(int, getaddrinfo_f, const char *codename, const char *servname,
// 		const struct addrinfo *hints, struct addrinfo **res);
//
// TYPEDEF_INST(struct hostent *, gethostbyname_f, const char *host);
//
// TYPEDEF_INST(int, sysctl_f, const int *name, u_int namelen, void *oldp, size_t *oldlenp,
// 		const void *newp, size_t newlen);
// TYPEDEF_INST(int, pthread_create_f, pthread_t *, const pthread_attr_t *, void *(*start_func)(void *), void *);
//
// TYPEDEF_INST(void *, mmap_f, void *addr, size_t len, int prot, int flags, int fd, off_t offset);
// typedef void *(*dlopen_f)(const char *, int);
// dlopen_f __real_dlopen = nullptr;

fs::path current_path = ".";

namespace Plox {

// int kcov_device = -1;
//
// template<typename... Args>
// int plox_syscall(int number, Args... args) {
// #if defined(WITH_KCOV)
// 	if (kcov_device == -1) {
// 		return syscall(number, args...);
// 	}
//
// 	if (number == SYS_CALL_NUM) {
//
// #if defined(PC_TRACE)
// 		if (ioctl(kcov_device, KIOENABLE, KCOV_MODE_TRACE_PC) == -1) {
// 			ERR("Failure to enable");
// 		}
// #endif // PC_TRACE
//        
// #if defined(CMP_TRACE)
// 		if (ioctl(kcov_device, KIOENABLE, KCOV_MODE_TRACE_CMP) == -1) {
// 			ERR("Failure to enable");
// 		}
// #endif // CMP_TRACE
// 	}
//
// 	int ret = syscall(number, args...);
// 	if (number == SYS_CALL_NUM) {
// 		if (ioctl(kcov_device, KIODISABLE, 0) == -1) {
// 			ERR("Failure to disable");
// 		}
// 	}
// 	return ret;
//
// #else
// 	return syscall(number, args...);
// #endif // WITH_KCOV
// }

extern "C" {

int 
open(const char *path, int flags, ...) {
	va_list args;
	int mode;
	if ((flags & O_CREAT) != 0) {
		va_start(args, flags);
		mode = va_arg(args, int);
		va_end(args);
	} else {
		mode = 0;
	}

	return syscall(SYS_open, path, flags, mode);
}

// BIND_REF(stat);
//BIND_REF(open);
// BIND_REF(mkdir);
// BIND_REF(openat);
// BIND_REF(connect);
// BIND_REF(socket);
// BIND_REF(bind);
// BIND_REF(setsockopt);
// BIND_REF(getsockname);
// BIND_REF(__sysctl);
//
#if defined(WITH_KCOV) && defined(MMAP)
//BIND_REF(mmap);
#endif

//__strong_reference(__plox_dlopen, dlopen);

} // EXTERN C


} // namespace Plox

