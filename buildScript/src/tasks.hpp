#pragma once
#include <string>
#include "project.hpp"

void addCompileTask(const std::string& fileName, const std::string& includePath, const Project* project, const std::string& target);
void initThreads(int threadc);
bool runTasks();
