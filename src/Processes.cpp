#include "Processes.h"

void execvp_cpp(std::string app
	, std::vector<std::string> args)
{
	std::vector<const char*> pargs;
	pargs.reserve(args.size() + 1);
	pargs.push_back(app.c_str());
	for (auto const& a : args) { pargs.push_back(a.c_str() + '\0'); }
	pargs.push_back(NULL);

	// Signature: int execvp(const char *file, char *const argv[]);

	// execvp(app.c_str(), execvp(app.c_str(), (char* const *) pargs.data() )
	int status = execvp(app.c_str(), (char* const*)pargs.data());

	if (status == -1)
	{
		std::fprintf(stderr, " Error: unable to launch process\n");
		throw std::runtime_error("Error: failed to launch process");
	}
}
std::pair<bool, int> fork_exec(std::string app, std::vector<std::string> args, int timelimitsec, std::string outfile)
{

	std::printf(" [TRACE] <BEFORE FORK> PID of parent process = %d \n", getpid());

	// PID of child process (copy of this process)
	pid_t pid = fork();

	if (pid == -1)
	{
		std::fprintf(stderr, "Error: unable to launch process\n");
		throw std::runtime_error("Error: unable to launch process");
	}
	if (pid == 0) {
		//std::printf(" [TRACE] Running on child process => PID_CHILD = %d \n", getpid());

		/*
		// Close file descriptors, in order to disconnect the process from the terminal.
		// This procedure allows the process to be launched as a daemon (aka service).
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		close(STDIN_FILENO);
		*/

		if (outfile != "") {
			// redirect stdout to file
			auto fd = fopen(outfile.c_str(), "w");
			//auto fd = open(outfile.c_str(), O_CREAT | O_TRUNC, "w");
			dup2(fileno(fd), fileno(stdout));
			dup2(fileno(fd), fileno(stderr));
			//auto fd = open(outfile.c_str(), O_CREAT | O_TRUNC, "w");
			//dup2(fd, STDOUT_FILENO);
			//dup2(fd, STDERR_FILENO);
		}
		// Execvp system call, replace the image of this process with a new one
		// from an executable. 
		execvp_cpp(app, args);
		return { false, -1 };
	}

	std::printf(" [TRACE] <AFTER FORK> PID of parent process = %d \n", getpid());

	// pid_t waitpid(pid_t pid, int *wstatus, int options);
	int status;

	std::printf(" [TRACE] Waiting for child process to finish.\n");

	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();// = std::chrono::system_clock::now();
	bool finished = false;

	// wait until timelimit runs out and check whether the child finished
	while (timelimitsec == 0 || std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() < timelimitsec * 1000)
	{
		// Wait for child process termination.
		// From header: #include <sys/wait.h>
		int ret = waitpid(pid, &status, WNOHANG);
		if (ret == -1)
		{
			logger::error("Processes", "Fork_exec", "Error: cannot wait for child process");
			throw std::runtime_error("Error: cannot wait for child process");
		}
		else if (ret == pid)
		{
			finished = true;
			break;
		}
		else
		{
			if ((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start).count()) % 60 == 0)
				logger::info("Processes", "Fork_exec", "Still waiting for child to finish. Running for: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start).count()) + "s");
			std::this_thread::sleep_for(1s);
		}
	}
	int exitcode = 1;
	if (finished && WIFEXITED(status))
	{
		exitcode = WEXITSTATUS(status);
	}
	else
	{
		// kill child since it exceeded its time limit
		kill(pid, SIGTERM);
	}

	logger::info("Processes", "Fork_exec", "Child has finished after: " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start).count()) + " seconds");

	std::printf(" [TRACE] Child process has been terminated with exitcode: %d\n", exitcode);
	// -------- Parent process ----------------//
	return { finished, exitcode };
}