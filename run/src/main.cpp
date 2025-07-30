#include "project.hpp"
#include "unistd.h"
#include <fstream>
#include <iostream>

int main(int argc, const char** argv)
{
	if (argc < 2)
	{
		std::cerr << "not enough args\n";
		return 1;
	}
	nlohmann::json json;
	if (!std::filesystem::exists("project.json"))
	{
		std::cerr << "cant find \'project.json\'\n";
		return 1;
	}
	std::ifstream ifstream("project.json");
	try
	{
		ifstream >> json;
	}
	catch (nlohmann::json::parse_error e)
	{
		std::cerr << "./project.json parse error\n" << e.what() << '\n';
		return 1;
	}

	JsonObjects objects;
	objects.libJson = json["projects"];
	objects.libPath = "./";
	objects.projectJson = json["projects"];
	objects.projectPath = "./";

  Project proj;
	findProject(argv[1], objects, &proj);

	std::string outFile = proj.out;
	std::vector<char*> args;
	args.push_back((char*)outFile.c_str());
	for (int i = 0; i < argc - 2; i++)
	{
		args.push_back((char*)argv[i]);
	}
	args.push_back(0);

	outFile = "./" + std::string(argv[1]) + "/bin/" + outFile;

	execvp(outFile.c_str(), args.data());
}
