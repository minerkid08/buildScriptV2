#ifdef __linux
#include "process.hpp"
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define pipeRead pipefd[0]
#define pipeWrite pipefd[1]

int runCmd(const char* cmd, const char* args2, std::string* out)
{
	int pipefd[2];

	pid_t pid;

	if (pipe(pipefd) == -1)
	{
		perror("pipe");
		return 1;
	}

	pid = fork();

	if (pid < 0)
	{
		perror("fork");
		return 1;
	}
	else if (pid == 0)
	{
		// Child process

		char* args = (char*)args2;
		int arglen = strlen(args);

		std::vector<char*> data;

		data.push_back((char*)cmd);
		data.push_back(args);
		for (int i = 0; i < arglen; i++)
		{
			if (args[i] == ' ')
			{
				args[i] = '\0';
				data.push_back(args + i + 1);
			}
		}

		data.push_back(nullptr);

		dup2(pipeWrite, STDOUT_FILENO);
		dup2(pipeWrite, STDERR_FILENO);

    close(pipeWrite);
    close(pipeRead);

		execvp(cmd, data.data());
		perror("execlp");
		return 1;
	}
	else
	{
		// Parent process

		close(pipeWrite);
		if (out)
		{
			char buffer[256];
			int bytesRead;

			while ((bytesRead = read(pipeRead, buffer, sizeof(buffer))) > 0)
			{
				*out += std::string(buffer, bytesRead);
			}
		}
		else
		{
      wait(nullptr);
		}

		close(pipeRead);
	}
	return 0;
}
#endif

#ifdef DEBUG
#pragma message("help")
#endif
