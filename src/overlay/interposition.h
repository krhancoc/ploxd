#include <iostream>
#include <fstream>

extern std::ofstream yf;
extern int YF_FD;

#define YELL_PATH_SIZE (256)

char YELL_LOG[256] = "/tmp/yell.root.log";
extern "C" {
ssize_t __sys_write(int, const void *, size_t);
}

char * get_yell_name()
{
	if (strlen(YELL_LOG) == 0) {
		snprintf(YELL_LOG, YELL_PATH_SIZE, "/tmp/yell.%d.log", getpid());
	}

	return YELL_LOG;
}

template<typename Arg1>
void recursive_yell(std::stringstream &ss, const Arg1 arg1) {
  ss << arg1 << " ";
}

template<typename Arg1, typename ...Args>
void recursive_yell(std::stringstream &ss, const Arg1 arg1,const Args... args) {
  ss << arg1 << " ";
  recursive_yell(ss, args...);
}


template<typename Arg1, typename ...Args>
void yell(Arg1 arg, Args... args) {
	std::stringstream ss;
	if (YF_FD == -1) {
		std::stringstream ss;
		YF_FD = open(get_yell_name(), O_WRONLY | O_APPEND | O_CREAT, 0666);
		if (YF_FD == -1 && errno == EEXIST) {
			YF_FD = open(get_yell_name(), O_APPEND);
		}
		if (YF_FD == -1)
			throw std::runtime_error("Error opening yell.log");
	}

	recursive_yell(ss, arg, args...);
	ss << std::endl;
	__sys_write(YF_FD, ss.str().c_str(), ss.str().size());
}

