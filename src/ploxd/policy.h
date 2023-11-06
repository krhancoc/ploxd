#ifndef __POLICY_H__
#define __POLICY_H__
#include <string>

class PloxPolicy {
public:
	PloxPolicy(std::string &&filename);
	int setupPolicy(int pid);
private:
	std::string policyFile;
};
#endif
