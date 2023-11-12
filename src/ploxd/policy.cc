#include <plox.h>

#include "policy.h"

PloxPolicy::PloxPolicy(std::string &&filename) {
	this->policyFile = filename;
}

/*
 * Policy Setup Here!
 */
int PloxPolicy::setupPolicy(int pid)
{
	plox_register(pid);
	return 0;
}
