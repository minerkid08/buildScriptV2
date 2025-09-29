#include "process.hpp"
#include "project.hpp"
#include <cstdio>
#include <iostream>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <vector>

struct Task
{
	std::string filename;
	std::string out;
	std::string includePaths;
	bool status;
	const Project* project;
	std::string target;
};

struct Thread
{
	Task* task = 0;
	pthread_t thread;
	bool runningTask;
	bool threadRunning;
	int readPipe;
	int writePipe;
};

int resPipe;

void* threadFunc(void* arg)
{
	Thread* thread = (Thread*)arg;
	while (thread->threadRunning)
	{
		char buf;
		if (read(thread->readPipe, &buf, 1) != 1)
			perror("read in task thread failed");

		if (thread->task == 0)
			perror("tried to run null task");

		std::string args = thread->task->includePaths + "-c -g " + thread->task->filename + " -o ";
		std::filesystem::path outFile = thread->task->filename;
		outFile = std::filesystem::relative(outFile, thread->task->project->path);
		outFile.replace_extension(".o");
		outFile = thread->task->project->path / "cache" / thread->task->target / outFile;
		args += outFile.string();
		if (thread->task->project->configArgs.find(thread->task->target) != thread->task->project->configArgs.end())
		{
			args += ' ';
			args += thread->task->project->configArgs.at(thread->task->target);
		}

		for (const std::string& s : thread->task->project->compArgs)
		{
			args += s + ' ';
		}
		std::filesystem::remove(outFile);
		std::filesystem::create_directories(outFile.parent_path());
    thread->task->out = "";
		if (thread->task->project->lang == ProjectLang::CPP)
			runCmd("g++", args.c_str(), &thread->task->out);
		else
			runCmd("gcc", args.c_str(), &thread->task->out);
		if (std::filesystem::exists(outFile))
			thread->task->status = true;
		else
			thread->task->status = false;
		thread->task = 0;
		write(resPipe, &buf, 1);
	}
	thread->threadRunning = 0;
	return 0;
}

void initThread(Thread* thread)
{
	thread->threadRunning = 1;
	thread->task = 0;
	int pipes[2];
	pipe(pipes);
	thread->readPipe = pipes[0];
	thread->writePipe = pipes[1];
	pthread_create(&thread->thread, 0, threadFunc, thread);
}

void destroyThread(Thread* thread)
{
	thread->threadRunning = 0;
}

void runTask(Thread* thread, Task* task)
{
	if (thread->task != 0)
		perror("tried to reassign task to thread");
	thread->task = task;
	char c = 1;
	write(thread->writePipe, &c, 1);
}

std::vector<Task*> tasks;
std::vector<Thread*> threads;

void addCompileTask(const std::string& fileName, const std::string& includePath, const Project* project,
					const std::string& target)
{
	tasks.emplace_back(new Task());
	Task* task = tasks[tasks.size() - 1];

	task->project = project;
	task->includePaths = includePath;
	task->filename = fileName;
	task->target = target;
}

void initThreads(int threadc)
{
	for (int i = 0; i < threadc; i++)
	{
		threads.emplace_back(new Thread());
		initThread(threads[i]);
	}
}

bool runTasks()
{

	int resPipes[2];
	pipe(resPipes);

	int resRead = resPipes[0];
	resPipe = resPipes[1];

	int i = 0;

	for (int j = 0; j < threads.size(); j++)
	{
		if (i < tasks.size())
		{
			runTask(threads[j], tasks[i++]);
			printf("\033[K> %s\n", threads[j]->task->filename.c_str());
		}
		else
			printf("\033[K>\n");
	}
	fflush(stdout);

	while (true)
	{
		char buf;
		read(resRead, &buf, 1);

		int threadRunning = 0;

		printf("\033[%zuA", threads.size());
		// printf("\n");
		for (int j = 0; j < threads.size(); j++)
		{
			Thread* thread = threads[j];
			if (thread->task == 0 && i < tasks.size())
				runTask(thread, tasks[i++]);
			if (thread->task != 0)
			{
				threadRunning = 1;
				printf("\033[K> %s\n", thread->task->filename.c_str());
			}
			else
				printf("\033[K>\n");
		}
		fflush(stdout);

		if (threadRunning == 0 && i == tasks.size())
			break;
	}

	bool failed = true;
	for (Task* task : tasks)
	{
		if (task->status == false)
		{
			std::cout << "building " << task->filename << " failed\n";
			std::cout << task->out << "\n";
			failed = false;
		}
		else if (task->out != "")
		{
			std::cout << task->filename << '\n';
			std::cout << task->out << "\n";
		}

		delete task;
	}
	tasks.resize(0);
	return failed;
}
