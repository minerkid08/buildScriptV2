#include "process.hpp"
#include "project.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <sys/stat.h>
#include <vector>

std::string target = "debug";

unsigned long lastCompileTime;

unsigned long getFileWriteTime(const char* filename)
{
	struct stat fileStat;
	stat(filename, &fileStat);
	return fileStat.st_mtime;
}

bool shouldBuildFile(const char* filename, const Project& p, const std::string& includePaths)
{
	if (target == "clean")
		return true;
	if (getFileWriteTime(filename) > lastCompileTime)
		return true;
	std::string args = includePaths;
	args += "-MM ";
	args += filename;
	std::string out;
	runCmd("g++", args.c_str(), &out);

	std::vector<std::string> files;

	std::string str;

	char reading = 0;
	for (int i = 0; i < out.size(); i++)
	{
		if (out[i] == ' ' && reading < 2)
		{
			reading++;
			continue;
		}
		if (reading > 1)
		{
			if (out[i] == ' ')
			{
				if (str.size() > 0)
				{
					files.push_back(str);
					str = "";
				}
			}
			else if (out[i] != '\\' && out[i] != '\n')
				str += out[i];
		}
	}
	files.push_back(str);

	for (const std::string& file : files)
	{
		if (std::filesystem::exists(file))
		{
			if (getFileWriteTime(file.c_str()) > lastCompileTime)
				return true;
		}
	}

	return false;
}

bool buildFile(const std::filesystem::path& file, const Project& project, const std::string& includePaths)
{
	std::string args = includePaths + "-c -g " + file.string() + " -o ";
	std::filesystem::path outFile = file;
	outFile = std::filesystem::relative(outFile, project.path);
	outFile.replace_extension(".o");
	outFile = project.path / "cache" / outFile;
	args += outFile.string();
	std::filesystem::remove(outFile);
	std::filesystem::create_directories(outFile.parent_path());
	std::cout << "building: " << std::filesystem::relative(file, project.path).string();
	std::cout.flush();
	std::string out;
	runCmd("g++", args.c_str(), &out);
	if (std::filesystem::exists(outFile))
	{
		std::cout << " done\n";
		return false;
	}
	else
	{
		std::cout << out << '\n';
		return true;
	}
}

void buildProject(Project& project, const std::vector<Project>& projects);
void buildProjectTree(Project& project, std::vector<Project>& projects);

int main(int argc, const char** argv)
{
	if (argc != 2 && argc != 3)
	{
		std::cout << "expected 2 or 3 args, got " << argc << '\n';
		return 1;
	}

	std::string proj;

	proj = argv[1];
	if (argc == 3)
		target = argv[2];

	JsonObjects objects;
	objects.projectPath = std::filesystem::absolute("./");
	objects.libPath = std::filesystem::absolute("./");

	nlohmann::json cacheJson;
	if (std::filesystem::exists("cache.json"))
	{
		std::ifstream ifstream("cache.json");
		ifstream >> cacheJson;
		lastCompileTime = cacheJson["time"];
	}

	nlohmann::json json;
	if (!std::filesystem::exists("project.json"))
	{
		std::cerr << "cant find \'project.json\'\n";
		return 1;
	}
	std::ifstream ifstream("project.json");
	ifstream >> json;
	objects.projectJson = json["projects"];
	objects.libJson = json["projects"];

	std::vector<Project> projects;
	projects.push_back({});

	findProject(proj.c_str(), objects, &projects[0]);

	std::vector<std::string> libsToFind;
	for (const std::string& s : projects[0].libs)
		libsToFind.push_back(s);

	while (libsToFind.size() > 0)
	{
		std::string s = libsToFind[libsToFind.size() - 1];
		libsToFind.pop_back();
		bool found = false;
		for (const Project& p : projects)
		{
			if (p.name == s)
			{
				found = true;
				break;
			}
		}
		if (found)
			continue;
		projects.push_back({});
		Project* p = &projects[projects.size() - 1];
		findProject(s.c_str(), objects, p);
		for (const std::string& s : p->libs)
			libsToFind.push_back(s);
	}

	unsigned long time = 0;
	if (cacheJson.contains("time"))
		time = cacheJson["time"];

	buildProjectTree(projects[0], projects);

	std::cout << "build sucessful\n";

	long now = std::time(0);
	struct tm* tm = gmtime(&now);
	time = timegm(tm);
	cacheJson["time"] = time;
	std::ofstream("cache.json") << cacheJson.dump(2);
}

void buildProjectTree(Project& project, std::vector<Project>& projects)
{
	for (const std::string& lib : project.libs)
	{
		for (Project& p : projects)
		{
			if (p.name == lib)
			{
				if (!p.built)
					buildProjectTree(p, projects);
				break;
			}
		}
	}
	buildProject(project, projects);
}

void buildProject(Project& project, const std::vector<Project>& projects)
{
	project.built = true;
	if (project.type == ProjectType::Header)
		return;
	std::vector<std::filesystem::path> toBuild;
	std::vector<std::filesystem::path> toProcess;

	for (const std::string& str : project.build)
	{
		toProcess.push_back(str);
	}
	std::string includePaths;
	for (const std::filesystem::path& str : project.include)
		includePaths += std::string("-I") + str.string() + ' ';

	for (const Project& p : projects)
	{
		for (const std::string& s : project.libs)
		{
			if (p.name == s)
			{
				for (const std::filesystem::path& str : p.exportPaths)
					includePaths += std::string("-I") + str.string() + ' ';
				break;
			}
		}
	}

	std::string linkerArgs;
	while (toProcess.size() > 0)
	{
		std::filesystem::path curPath = toProcess[toProcess.size() - 1];
		toProcess.pop_back();
		for (const std::filesystem::path& p : std::filesystem::directory_iterator(curPath))
		{
			if (std::filesystem::is_directory(p))
				toProcess.push_back(p);
			else
			{
				if (p.extension() == ".cpp" || p.extension() == ".c")
				{
					if (shouldBuildFile(p.string().c_str(), project, includePaths))
						toBuild.push_back(p);
					std::filesystem::path linkerFile = p;
					linkerFile = std::filesystem::relative(linkerFile, project.path);
					linkerFile.replace_extension(".o");
					linkerFile = project.path / "cache" / linkerFile;
					linkerArgs += linkerFile.string() + ' ';
				}
			}
		}
	}

	if (toBuild.size() > 0)
	{
		std::cout << "--- Building Target \'" << project.name << "\' ---\n";

		for (const std::filesystem::path& file : toBuild)
		{
			if (buildFile(file, project, includePaths))
			{
				std::cout << "build failed\n";
				exit(1);
			}
		}

		linkerArgs += "-o bin/" + project.out;

		std::string out;
		std::filesystem::create_directories(std::filesystem::path("bin/" + project.out).parent_path());
		std::cout << "linking ";
		std::cout.flush();
		runCmd("g++", linkerArgs.c_str(), &out);

		if (std::filesystem::exists("bin/" + project.out))
			std::cout << "done\n";
		else
		{
			std::cout << "\n" << out << "\nbuild failed\n";
			exit(1);
		}
	}
}