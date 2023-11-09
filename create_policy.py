#!/usr/bin/env python3
import os
import sys
CREATE_CALLS = ["open", "socket"]
EDGE_CALLS = ["mmap"]
END_CALLS = ["close"]
FB_CALLS = ["gethostbyname", "getaddrinfo"]


class Operation:
    def __init__(self):
        pass


class ResourcePolicy:
    def __init__(self, op, args):
        self.op = op
        self.args = args
        self.allow_operations = []

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


class GlobalPolicy:
    def __init__(self):
        self.resources = []
        self.global_calls = []

    def add_policy(self, op, args):
        rp = ResourcePolicy(op, args) 
        for i, x in enumerate(self.resources):
            if x.is_same(rp):
                return self.resources[i]

        self.resources.append(rp)
        return self.resources[-1]


def create_policy(trace_file):
    with open(trace_file) as file:
        data = file.read()

    # Get every call, lets throw away the trace for now
    data = [ d.split("\n")[1] for d in data.split("CALL_SITE:")]

    # Iterate over every operation now
    FD_CACHE = {}
    policy = GlobalPolicy()
    for operation in data:
        operation = operation.split()
        if not len(operation):
            continue

        op = operation[0]
        args = []

        for d in operation[1:]:
            if d == "=":
                break
            args.append(d)

        ret = operation[-1]
        if op in CREATE_CALLS:
            # Update our FD cache
            rp = policy.add_policy(op, args)
            FD_CACHE[ret] = rp

        elif op in EDGE_CALLS:
            pass
        elif op in END_CALLS:
            pass
        elif op in FB_CALLS:
            pass
        else:
            pass

    print(FD_CACHE)
    for p in policy.resources:
        print(p)


if __name__ == "__main__":
    if not os.path.exists(sys.argv[1]):
        print("{} does not exist".format(sys.argv[1]))
        sys.exit(-1)

    create_policy(sys.argv[1])
