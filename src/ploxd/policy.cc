#include "policy.h"

PloxPolicy::PloxPolicy(std::string &&filename) {
	this->policyFile = filename;
}

/*
 * Policy Setup Here!
 */
int PloxPolicy::setupPolicy(int pid)
{
	return 0;
}
