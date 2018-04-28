#include "./rlimit.h"

#include <iostream>
#include <signal.h>
#include <sys/resource.h>
#include <cassert>

namespace {
	void on_signal(int sig) {
		if (sig == SIGXCPU) std::cerr << "Hit CPU time limit -- probably an infinite loop somewhere" << std::endl;
		exit(sig);
	}

	rlimit get_rlimit(int resource) {
		rlimit r;
		getrlimit(resource, &r);
		return r;
	}

	void set_rlimit(int resource, const rlimit& r) {
		int err = setrlimit(resource, &r);
		if (err == -1) {
			perror("bad stuff");
			exit(err);
		}
		assert(err == 0);
	}

	void reduce_soft_limit(int resource, rlim_t lim) {
		rlimit curr = get_rlimit(resource);
		assert(lim < curr.rlim_max);
		curr.rlim_cur = lim;
		set_rlimit(resource, curr);
	}

	void set_soft_limit_to_hard_limit(int resource) {
		rlimit curr = get_rlimit(resource);
		curr.rlim_cur = curr.rlim_max;
		set_rlimit(resource, curr);
	}
}

void set_limits() {
	// Should take less than 1 second
	reduce_soft_limit(RLIMIT_CPU, 1);
	// Should not consume more than 2^27 bytes (~125 megabytes)
	reduce_soft_limit(RLIMIT_AS, 1 << 25);
	signal(SIGXCPU, on_signal);
}

void unset_limits() {
	set_soft_limit_to_hard_limit(RLIMIT_CPU);
	set_soft_limit_to_hard_limit(RLIMIT_AS);
}
