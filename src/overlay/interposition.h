#include <iostream>
#include <fstream>

extern std::ofstream yf;
extern int YF_FD;

const char * YELL_LOG = "/tmp/yell.log";
extern "C" {
ssize_t __sys_write(int, const void *, size_t);
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
		YF_FD = open(YELL_LOG, O_WRONLY | O_APPEND | O_CREAT, 0666);
		if (YF_FD == -1 && errno == EEXIST) {
			YF_FD = open(YELL_LOG, O_APPEND);
		}
		if (YF_FD == -1)
			throw std::runtime_error("Error opening yell.log");
	}

	recursive_yell(ss, arg, args...);
	ss << std::endl;
	__sys_write(YF_FD, ss.str().c_str(), ss.str().size());
}

