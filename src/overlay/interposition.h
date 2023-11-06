#include <iostream>
#include <fstream>

extern std::ofstream yf;

const char * YELL_LOG = "/tmp/yell.log";

template<typename Arg1>
void recursive_yell(const Arg1 arg1) {
  yf << arg1 << " ";
}

template<typename Arg1, typename ...Args>
void recursive_yell(const Arg1 arg1,const Args... args) {
  yf << arg1 << " ";
  recursive_yell(args...);
}


template<typename Arg1, typename ...Args>
void yell(Arg1 arg, Args... args) {
  if (!yf.is_open()) {
    yf.open(YELL_LOG, std::ofstream::out | std::ofstream::app);
    if (yf.fail()) {
      throw std::runtime_error("Error opening yell.log");
    }
  }

  recursive_yell(arg, args...);
  yf << std::endl;
}

