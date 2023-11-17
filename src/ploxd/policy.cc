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
	int error = plox_register(pid);
  if (error)
    perror("Error settting up policy\n");
	return 0;
}
