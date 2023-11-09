/* Use LLVM Libunwind */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <libunwind.h>
#include <nsswitch.h>

#include <sstream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>

#include "interposition.h"

// Start Globals 
int kcov_device = -1;
std::ofstream yf;
int YF_FD = -1;
char self_path[256];
const char *ONLY_IP = "ONLY_IP";
const char *NSDISPATCH = "nsdispatch";

#define YELL(...) \
	do { \
		auto env_opt = std::getenv("OVERLAY_OPT"); \
		if (env_opt && strcmp(env_opt, ONLY_IP) == 0) { \
			yell("CALL_SITE:"); \
			yell(__VA_ARGS__); \
			yell(get_backtrace(ONLY_IP)); \
		} else { \
			yell("CALL_SITE:"); \
			yell(__VA_ARGS__); \
			yell(get_backtrace(NULL)); \
		} \
	} while(false); 


std::string 
sockaddr_tostring(const struct sockaddr *addr, socklen_t size)
{
	std::stringstream ss;
	if (size == sizeof(struct sockaddr_in)) {
		struct sockaddr_in *in = (struct sockaddr_in *)addr;
		int port = ntohs(in->sin_port);
		ss << "sockaddr {" << (int)in->sin_len << " " << (int)in->sin_family;
		ss << " " << port << " " << in->sin_addr.s_addr << "}";
	} else if (size == sizeof(sockaddr_in6)) {
		struct sockaddr_in6 *in = (struct sockaddr_in6 *)addr;
		ss << "AF_INET6";
	} else {
		ss << "UNKNOWN";
	}

	return ss.str();
}

template<typename... Args>
int plox_syscall(int number, Args... args) {
#if defined(WITH_KCOV)
	if (kcov_device == -1) {
		return syscall(number, args...);
	}

	if (number == SYS_CALL_NUM) {

#if defined(PC_TRACE)
		if (ioctl(kcov_device, KIOENABLE, KCOV_MODE_TRACE_PC) == -1) {
			ERR("Failure to enable");
		}
#endif // PC_TRACE

#if defined(CMP_TRACE)
		if (ioctl(kcov_device, KIOENABLE, KCOV_MODE_TRACE_CMP) == -1) {
			ERR("Failure to enable");
		}
#endif // CMP_TRACE
	}

	int ret = syscall(number, args...);
	if (number == SYS_CALL_NUM) {
		if (ioctl(kcov_device, KIODISABLE, 0) == -1) {
			ERR("Failure to disable");
		}
	}
	return ret;

#else
	return syscall(number, args...);
#endif // WITH_KCOV
}


extern "C" {

#define BIND_REF(_name)   \
	__strong_reference(__plox_##_name, _name); \
	__strong_reference(__plox_##_name, _##_name);

#define TYPEDEF_INST(return_val, _name, ...) \
  typedef return_val (* _name##_f)(__VA_ARGS__); \
  _name##_f __real_##_name##_f = nullptr; \
  return_val __sys_##_name(__VA_ARGS__)

TYPEDEF_INST(struct hostent *, gethostbyname, const char *host);
TYPEDEF_INST(int, getaddrinfo, const char *, const char *, const struct addrinfo *, struct addrinfo **);
TYPEDEF_INST(int, nsdispatch, void *, const ns_dtab[], const char *, const char *method_name, const ns_src defaults[], ...);
TYPEDEF_INST(ssize_t, write, int fd, const void *buffer, size_t size);
TYPEDEF_INST(int, close, int fd);


int __sys_open(const char *, int, ...);


// End Globals 
static inline std::string
get_backtrace (const char *opt) {
	std::stringstream ss;
	char symbol[256];
	int ret;
	unw_word_t offset;
	Dl_info info;

	unw_cursor_t cursor; unw_context_t uc;
	unw_word_t ip, sp;

	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);

	while (unw_step(&cursor) > 0) {
		unw_get_reg(&cursor,	UNW_REG_IP, &ip);
		unw_get_reg(&cursor,	UNW_REG_SP, &sp);
		ret = dladdr((void *)ip, &info);
		uintptr_t base = -1;
		if (ret) {
			base = (uintptr_t)info.dli_fbase;
		}

		if (unw_get_proc_name(&cursor, symbol, sizeof(symbol), &offset) == 0) {
			if (opt && strcmp(opt, ONLY_IP) == 0) {
				ss  << base << "," << ip << "," << sp << std::endl;
			} else {
				ss << "base=" << base << ", ip=" << ip << ", sp=" << sp << ";";
				ss << "Function: " << symbol << " (offset: 0x" << std::hex << offset << ")" << std::endl;
			}
		} else {
			if (opt && strcmp(opt, ONLY_IP) == 0) {
				ss  << base << "," << ip << "," << sp << std::endl;
			} else {
				ss << "base=" << base << ", ip=" << ip << ", sp=" << sp << ";" << "NONE" << std::endl;
			}
		}
	}

	return ss.str();
}

char * 
get_self_path() {
	int mib[4];
	size_t len;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;  // The current process

	if (sysctl(mib, 4, NULL, &len, NULL, 0) == -1) {
		perror("sysctl");
		_exit(1);
	}

	if (sysctl(mib, 4, self_path, &len, NULL, 0) == -1) {
		perror("sysctl");
		_exit(1);
	}

	return self_path;
}


#define BUFFER_SIZE (1024)
/* 
 * We have to be careful with open here as it used
 * in startup. This is why we have a buffer as malloc may not
 * even work during open leading to large crashes.
 */
int __plox_open(const char *path, int flags, ...) {
	char buffer[BUFFER_SIZE];
	va_list args;
	int mode;
	
	if ((flags & O_CREAT) != 0) {
		va_start(args, flags);
		mode = va_arg(args, int);
		va_end(args);
	} else {
		mode = 0;
	}
	
	if (strncmp(path, "/dev/hpet", 9) == 0) {
		return __sys_open(path, flags, mode);
	}

	if (strcmp(path, YELL_LOG) == 0) {
		// Make sure we dont recrusive on the log;
		return __sys_open(path, flags, mode);
	}

	if (strcmp(path, get_self_path()) == 0) {
		return __sys_open(path, flags, mode);
	}

	auto retfd = __sys_open(path, flags, mode);
	int fd = __sys_open(YELL_LOG, O_WRONLY | O_APPEND | O_CREAT, 0666);
	if (fd < 0 && errno == EEXIST) {
		fd = __sys_open(YELL_LOG, O_WRONLY | O_APPEND);
	}

	if (fd < 0) {
		snprintf(buffer, BUFFER_SIZE, "Error interposing on %s\n", path);
		__sys_write(STDERR_FILENO, buffer, strlen(buffer));
		exit(-2);
	}

	memset(buffer, '\0', BUFFER_SIZE);
	snprintf(buffer, BUFFER_SIZE, "\nCALL_SITE:\nopen %s %d %d = %d\n", path, flags, mode, retfd);
	__sys_write(fd, buffer, strlen(buffer));

	memset(buffer, '\0', BUFFER_SIZE);
	snprintf(buffer, BUFFER_SIZE, "%s\n", get_backtrace(NULL).c_str());

	// Ensure we have the new line character;
	buffer[BUFFER_SIZE - 1] = '\n';
	__sys_write(fd, buffer, strlen(buffer));
	__sys_close(fd);

	return retfd;
}

int
__plox_socket(int domain, int type, int protocol)
{
	auto retfd = plox_syscall(SYS_socket, domain, type, protocol);
	YELL("socket", domain, type, protocol, "=", retfd);
	return retfd;
}



int
__plox_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
	auto ret = plox_syscall(SYS_connect, s, name, namelen);
	YELL("connect", s, sockaddr_tostring(name, namelen), (int)namelen);
	return ret;
}

int nsdispatch_ret;
const char fn[] = "nsdispatch";
const char pfns[] = "nsdispatch %s";

struct registers {
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rsp;
	uint64_t rax;
	uint64_t r9;
	uint64_t r8;
} rg, rg2;


__attribute__((naked)) int
__plox_nsdispatch(void *retval, const ns_dtab dtab[], const char *database,
   const char *method_name, const ns_src defaults[], ...)
{
	asm (
		"push %%rdi\n\t"
		"push %%rsi\n\t"
		"mov %%rdx, %%rsi\n\t"
		"lea %1, %%rdi\n\t"
		"call printf\n\t"
		"pop %%rdi\n\t"
		"pop %%rsi\n\t"
		"call *%0\n\t"
		"ret"
		:
		: "m" (__real_nsdispatch_f), "m" (pfns)
		: "rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9"
	    );
}

struct hostent * __plox_gethostbyname(const char *name)
{
	__real_gethostbyname_f = (gethostbyname_f)dlsym(RTLD_NEXT, "gethostbyname");
	auto hostentret = __real_gethostbyname_f(name);
	YELL("gethostbyname", name);
	return hostentret;
}

int
__plox_getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res)
{
	__real_getaddrinfo_f = (getaddrinfo_f)dlsym(RTLD_NEXT, "getaddrinfo");
	auto ret = __real_getaddrinfo_f(hostname, servname, hints, res);
	YELL("getaddrinfo", hostname);
	return ret;
}


TYPEDEF_INST(ssize_t, read, int fd, void *buffer, size_t size);
ssize_t
__plox_read(int fd, void *buffer, size_t size)
{
	ssize_t ret = __sys_read(fd, buffer, size);
	YELL("read", fd, size, "=", ret);
	return ret;
}

ssize_t
__plox_write(int fd, const void *buffer, size_t size)
{
	ssize_t ret = __sys_write(fd, buffer, size);
	if (fd != YF_FD)
		YELL("write", fd, size, "=", ret);
	return ret;
}

BIND_REF(open);
BIND_REF(connect);
BIND_REF(socket);
BIND_REF(getaddrinfo);
BIND_REF(gethostbyname);
BIND_REF(read);
BIND_REF(write);
//BIND_REF(nsdispatch);
} // EXTERN C
