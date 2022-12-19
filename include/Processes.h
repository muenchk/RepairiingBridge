
#ifndef H_PROCESSES
#define H_PROCESSES

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <unistd.h>
#include "Log.h"

using namespace std::chrono_literals;

////////////////// Original Author : CAIORSS
// C++ wrapper for the exevp() library-call 
// It replaces the current process image with a new one 
// from other executable. 
void execvp_cpp(std::string app
	, std::vector<std::string> args);

std::pair<bool, int> fork_exec(std::string app, std::vector<std::string> args, int timelimitsec, std::string outfile);

#endif