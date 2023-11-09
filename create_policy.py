#!/usr/bin/env python3
import os
import sys
import fcntl
import glob
import traceback

class Operation:
    def __init__(self, op, args, ret = None):
        self.op = op
        self.args = args
        self.ret = ret

    def is_same(self, other):
        if (self.op != other.op):
            return False

        if (len(self.args) != len(other.args)):
            return False

        for i, x in enumerate(self.args):
            if x != other.args[i]:
                return False

        return True

    def __str__(self):
        return "{} {}".format(self.op, self.args)

class ResourcePolicy:
    def __init__(self, op, args):
        self.create_call = Operation(op, args)
        self.operations = []

    def is_same(self, rp):
        return self.create_call.is_same(rp.create_call)

    def add_operation(self, op, args, fd_location=0):
        args[fd_location] = "$FD"
        operation = Operation(op, args)
        for i, x in enumerate(self.operations):
            if x.is_same(operation):
                return self.operations[i]

        self.operations.append(operation)
        return self.operations[-1]

    def merge(self, other):
        assert(self.is_same(other))
        added = []
        for x in other.operations:
            found = False
            for y in self.operations:
                if y.is_same(x):
                    found = True
                    break
            # There was a global call that was the same, continue
            if found:
                continue

            added.append(x)

        self.operations.extend(added)

    def __str__(self):
        final = "{} {{\n".format(self.create_call)
        for r in self.operations:
            final += "\t{},\n".format(r)
        final += "}}\n"
        return final

class ProcessPolicy:
    def __init__(self):
        self.resources = []
        self.global_calls = []
        self.fdb = {}
        self.fd_cache = {}

    def add_databound(self, op, args, ret):
        self.fdb[get_var()] = Operation(op, args, ret)

    def add_global(self, op, args):
        operation = Operation(op, args)
        for i, x in enumerate(self.global_calls):
            if x.is_same(operation):
                return self.global_calls[i]

        self.global_calls.append(operation)
        return self.global_calls[-1]

    def add_policy(self, op, args):
        rp = ResourcePolicy(op, args) 
        for i, x in enumerate(self.resources):
            if x.is_same(rp):
                return self.resources[i]

        self.resources.append(rp)
        return self.resources[-1]

    def merge(self, other):
        # Go through all globals and dedupe
        added = []
        for x in other.global_calls:
            found = False
            for y in self.global_calls:
                if y.is_same(x):
                    found = True
                    break
            # There was a global call that was the same, continue
            if found:
                continue

            added.append(x)

        self.global_calls.extend(added)

        added = [] 
        for x in other.resources:
            found = False
            same_as = None
            for y in self.resources:
                if x.is_same(y):
                    same_as = y
                    found = True
                    break

            if found:
                y.merge(x)
            else:
                added.append(x)

        self.resources.extend(added)

        self.fdb = {**self.fdb, **other.fdb}
        

    def __str__(self):
        fin = ""
        for var, op in self.fdb.items():
            fin += "{} = {}\n".format(var, op)

        for x in self.global_calls:
            fin += str(x) + "\n"

        for x in self.resources:
            fin += str(x)
        
        return fin

policies = {}

def DEFAULT_CREATE(policy, op, args, ret):
    if (int(ret) != -1):
        rp = policy.add_policy(op, args)
        policy.fd_cache[ret] = rp

def DEFAULT_FD_0(policy, op, args, ret):
    if (int(ret) != -1):
        fd = args[0]
        policy.fd_cache[fd].add_operation(op, args)

def fcntl_rule(policy, op, args, ret):
    if (int(ret) != -1):
        fd = args[0]
        cmd = args[1]
        if (int(cmd) == fcntl.F_SETLK):
            args[2] = "0xADDRESS"
        policy.fd_cache[fd].add_operation(op, args)

# Make sure that we handle dup cases
# They are the same underlying resource
def dup_rule(policy, op, args, ret):
    if (int(ret) != -1):
        old = args[0]
        newfd = ret
        op = policy.fd_cache[old].add_operation(op, args)
        policy.fd_cache[newfd] = policy.fd_cache[old]

def dup2_rule(policy, op, args, ret):
    if (int(ret) != -1):
        old = args[0]
        newfd = args[1] 
        op = policy.fd_cache[old].add_operation(op, args)
        policy.fd_cache[newfd] = policy.fd_cache[old]

def fork_rule(policy, op, args, ret):
    policy.add_global(op, args)
    # Pass our FD Cache forward - its okay if some flags are CLOEXEC
    # We will just overwrite them on the create call
    create_policy("/tmp/yell.{}.log".format(ret), policy.fd_cache.copy(), policy.fdb.copy())

def dlopen_rule(policy, op, args, ret):
    policy.add_global(op, args)

VARIABLE_NUM = 0
def get_var():
    global VARIABLE_NUM
    VARIABLE_NUM += 1
    return "$FDB{}".format(VARIABLE_NUM)

def gethostbyname_rule(policy, op, args, ret):
    policy.add_databound(op, args, ret)

def getaddrinfo_rule(policy, op, args, ret):
    policy.add_databound(op, args, ret)

def connect_rule(policy, op, args, ret):
    fd = args[0]
    ip = args[5][:-1]
    for var, fdbop in policy.fdb.items():
        if fdbop.op in ["gethostbyname", "getaddrinfo"]:
            # We found a forward binding rule here
            if (ip == fdbop.ret):
                args[5] = var + "}"

    args = [args[0]] + [" ".join(args[1:6])] + args[6:]
    policy.fd_cache[fd].add_operation(op, args)


operation_rules = {
        "open": DEFAULT_CREATE,
        "openat": DEFAULT_CREATE,
        "socket": DEFAULT_CREATE,
        "accept": DEFAULT_CREATE,
        "accept4": DEFAULT_CREATE,
        "connect": connect_rule,
        "kqueue": DEFAULT_CREATE,
        "dlopen": dlopen_rule,
        "kevent": DEFAULT_FD_0,
        "read": DEFAULT_FD_0,
        "write": DEFAULT_FD_0,
        "fstat": DEFAULT_FD_0,
        "fsync": DEFAULT_FD_0,
        "fcntl": fcntl_rule,
        "dup": dup_rule,
        "dup2": dup2_rule,
        "fork": fork_rule,
        "bind": DEFAULT_FD_0,
        "listen": DEFAULT_FD_0,


        # Forward Data Binding 
        "gethostbyname": gethostbyname_rule,
        "getaddrinfo": getaddrinfo_rule,
}

missing_errors = []

def create_policy(trace_file, fd_cache_init = None, forward_data_bind_init = None):
    with open(trace_file) as file:
        data = file.read()

    # Get every call, lets throw away the trace for now
    data = [ d.split("\n")[1] for d in data.split("CALL_SITE:") if len(d) != 0]
    
    first_process = -1
    for operation in data:
        operation = operation.split()
        if not len(operation):
            continue
        first_process = operation[0]
        break

    policies[first_process] = ProcessPolicy()
    policy = policies[first_process]
    # Setup Stdin, stdout, stderr, these are innate FD's
    if fd_cache_init is None:
        rp = policy.add_policy("STDIN", [])
        policy.fd_cache["0"] = rp
        rp = policy.add_policy("STDOUT", [])
        policy.fd_cache["1"] = rp
        rp = policy.add_policy("STDERR", [])
        policy.fd_cache["2"] = rp
    else:
        policy.fd_cache = fd_cache_init

    if forward_data_bind_init is not None:
        policy.fdb = forward_data_bind_init

    # Keep track of what resources our FD's actually point to.
    # Iterate over every operation now
    for operation in data:
        operation = operation.split()
        if not len(operation):
            continue

        process = operation[0]
        policy = policies[process]
        op = operation[1]
        args = []

        for d in operation[2:]:
            if d == "=":
                break
            args.append(d)

        ret = operation[-1]
        try:
            operation_rules[op](policy, op, args, ret)
        except Exception as e:
            missing_errors.append((op, args, e))

def merge_policies():
    global policies
    policies = [ x[1] for x in list(policies.items())]
    merging = policies[0]
    for p in policies[1:]:
        merging.merge(p)

    return merging

DNS_FILES = ["/etc/resolv.conf", "/etc/nsswitch.conf", "/etc/hosts"]
def purge_dns(policy):
    policy.resources = [ x for x in policy.resources 
                        if x.create_call.op != "open" or x.create_call.args[0] not in DNS_FILES ]

if __name__ == "__main__":
    create_policy("/tmp/yell.root.log")
    policy = merge_policies()
    purge_dns(policy)
    print(policy)
    print()
    print("Missing Calls:")
    print(missing_errors)
        
