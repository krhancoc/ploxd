#ifndef __UTIL__H
#define __UTIL__H
#include <filesystem>
#include <unordered_map>
#include <vector>

#include <openssl/ssl.h>

#define KB (1024)
#define MB (1024 * KB)
#define MAX_ARG_SIZE (2 * MB)
#define MS_TO_NS (1000000)

namespace fs = std::filesystem;
namespace ch = std::chrono;

#define OFFSET_SUB(ptr, amount, cast) ((cast)(uintptr_t)ptr - amount)
#define OFFSET_ADD(ptr, amount, cast) ((cast)(uintptr_t)ptr + amount)

#define BIND_REF(_name)                                                        \
  __strong_reference(__plox_##_name, _name);                                   \
  __strong_reference(__plox_##_name, _##_name);

template<typename Stream, typename Arg1>
void _row(Stream &ss, int width, Arg1 arg) {
  ss << std::left << std::setw(width) << arg;
}

template<typename Stream, typename T>
void _row(Stream &ss, int width, std::tuple<T, int> arg) {
  T first;
  int second;
  std::tie(first, second) = arg;
  ss << std::left << std::setw(second) << first;
}

template<typename Stream, typename Arg1, typename ...Arg>
void _row(Stream &ss, int width, Arg1 arg, Arg... args) {
  _row(ss, width, arg);
  _row(ss, width, args...);
}

template<typename Stream, typename ...Arg>
void row(Stream &ss, int width, int indent, Arg... args)
{
  std::string in = "";
  for (int i = 0; i < indent; i++) {
    in += " ";
  }
  _row(ss << in, width, args...);
  ss << std::endl;
}


struct TimerStats {
  char name[128]; 
  double stdev; 
  int count; 
  int min; 
  int max; 
  double avg; 
}; 

class Timer {
public:
  Timer (std::string name) : name(name) {};
  void start();
  void stop();

  int count();
  int min();
  int max();

  double stdev();
  double avg();

  TimerStats get_stats() {
    auto s = TimerStats {
      .stdev = stdev(),
      .count = count(),
      .min = min(),
      .max = max(),
      .avg = avg()
    };
    strcpy(s.name, name.c_str());
    return s;
  }
private:
  ch::time_point<ch::high_resolution_clock> st;
  std::string name;
  std::vector<int> counts;
};

typedef std::shared_ptr<Timer> TPtr;

class PerfCollecter {
public:
  PerfCollecter() = default;
  void register_timer(std::string name);
  TPtr get_timer(std::string name);
  std::string stats_string();
  void set_name(std::string name) { this->name = name; };
  auto begin() {
    return timers.begin();
  }
  auto end() {
    return timers.end();
  }
private:
  std::unordered_map<std::string, TPtr> timers;
  std::string name = "No Name Set";
};

int ssl_sendfile(SSL *ssl, fs::path from);
int ssl_recvfile(SSL *ssl, fs::path to);
int ssl_recvdata(SSL *ssl, void *buf, size_t size);
int ssl_senddata(SSL *ssl, void *buf, size_t size);
int senddata(int fd, void *buf, size_t size);
int recvdata(int fd, void *buf, size_t size);


#endif
