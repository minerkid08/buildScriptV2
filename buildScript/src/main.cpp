#include "compat.hpp"
#include "process.hpp"
#include "project.hpp"
#include "settings.hpp"

#include "json/json.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <unistd.h>
#include <vector>

std::string target = "debug";

bool clean = false;

unsigned long lastCompileTime;
unsigned long nowTime;

bool shouldBuildFile(const char* filename, const Project& p, const std::string& includePaths)
{
	if (clean)
		return true;
	if (getFileWriteTime(filename) > lastCompileTime)
		return true;
	std::filesystem::path outFile = filename;
	outFile = std::filesystem::relative(outFile, p.path);
	outFile.replace_extension(".o");
	outFile = p.path / "cache" / target / outFile;
	if (!std::filesystem::exists(outFile))
		return true;
	if (getFileWriteTime(outFile.string().c_str()) < getFileWriteTime(filename))
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
	outFile = project.path / "cache" / target / outFile;
	args += outFile.string();
	if (project.configArgs.find(target) != project.configArgs.end())
	{
		args += ' ';
		args += project.configArgs.at(target);
	}

	for (const std::string& s : project.compArgs)
	{
		args += s + ' ';
	}
	std::filesystem::remove(outFile);
	std::filesystem::create_directories(outFile.parent_path());
	std::cout << "building: " << std::filesystem::relative(file, project.path).string();
	std::cout.flush();
	std::string out;
	if (project.lang == ProjectLang::CPP)
		runCmd("g++", args.c_str(), &out);
	else
		runCmd("gcc", args.c_str(), &out);
	if (std::filesystem::exists(outFile))
	{
		std::cout << " done\n";
		if (out.length() > 0)
			std::cout << out << '\n';
		return false;
	}
	else
	{
		if (project.lang == ProjectLang::CPP)
			std::cout << "\ng++ " << args << '\n';
		else
			std::cout << "\ngcc " << args << '\n';
		std::cout << out << '\n';
		return true;
	}
}

void buildProject(Project& project, const std::vector<Project>& projects, bool mainProj = false);
void buildProjectTree(Project& project, std::vector<Project>& projects, bool mainProj = false);

int main(int argc, const char** argv)
{
	if (argc == 1)
	{
		std::cout << "expected at least 1 arg\n";
		return 1;
	}

	if (argc > 1)
	{
		if (strcmp(argv[1], "-c") == 0)
		{
			execvp("buildCfg", nullptr);
			return 0;
		}
	}

	std::string proj;

	proj = argv[1];
	if (argc > 2)
		target = argv[2];

	if (target == "clean")
	{
		clean = true;
		target = "debug";
	}

	if (argc > 3)
	{
		if (strcmp(argv[3], "clean"))
			clean = true;
	}

	JsonObjects objects;
	objects.projectPath = std::filesystem::absolute("./");
	objects.libPath = "../libs";

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
	objects.projectJson = json["projects"];

	Settings settings;
	parseSettings(&settings, json);
	if (!settings.noLibs)
	{
		nlohmann::json json2;
		std::string json2Path = objects.libPath.string() + "/project.json";
		if (!std::filesystem::exists(json2Path))
		{
			std::cerr << "cant find \'" << json2Path << "\'\n";
			return 1;
		}
		std::ifstream ifstream2(json2Path);
		try
		{
			ifstream2 >> json2;
		}
		catch (nlohmann::json::parse_error e)
		{
			std::cerr << '\'' << json2Path << "\' parse error\n" << e.what() << '\n';
			return 1;
		}

		objects.libJson = json2["projects"];
	}
	else
	{
		objects.libJson = objects.projectJson;
		objects.libPath = objects.projectPath;
	}
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

	nowTime = getCurrentTime();

	buildProjectTree(projects[0], projects, true);

	std::cout << "build sucessful\n";
}

void buildProjectTree(Project& project, std::vector<Project>& projects, bool mainProj)
{
	for (const std::string& lib : project.libs)
	{
		for (Project& p : projects)
		{
			if (p.name == lib)
			{
				if (!p.processed)
					buildProjectTree(p, projects);
				break;
			}
		}
	}
	buildProject(project, projects, mainProj);
}

void buildProject(Project& project, const std::vector<Project>& projects, bool mainProj)
{
	project.processed = true;
	if (!mainProj && !project.autoBuild)
		return;
	if (project.type == ProjectType::Header)
		return;

	nlohmann::json cacheJson;
	if (std::filesystem::exists(project.path / "cache" / target / "cache.json"))
	{
		std::ifstream ifstream(project.path / "cache" / target / "cache.json");
		ifstream >> cacheJson;
		lastCompileTime = cacheJson["time"];
		ifstream.close();
	}
	else
		lastCompileTime = 0;

	std::vector<std::filesystem::path> toBuild;
	std::vector<std::filesystem::path> toProcess;

	for (const std::filesystem::path& str : project.build)
	{
		toProcess.push_back(str);
	}
	std::string includePaths;
	std::string linkerLibs;

	for (const std::filesystem::path& str : project.include)
		includePaths += std::string("-I") + str.string() + ' ';

	bool builtSomething = false;
	for (const Project& p : projects)
	{
		for (const std::string& s : project.libs)
		{
			if (p.name == s)
			{
				for (const std::filesystem::path& str : p.exportPaths)
					includePaths += std::string("-I") + str.string() + ' ';
				if (p.type == ProjectType::Static)
				{
					linkerLibs += ' ';
					linkerLibs += (p.path / "bin" / p.out).string();
				}
				builtSomething |= p.built;
				break;
			}
		}
	}

	std::string linkerArgs;

	for (const std::string& s : project.linkArgs)
	{
		linkerArgs += ' ' + s;
	}

	std::cout << "scanning for files to build \'" << project.name << "\' ";

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
					linkerFile = project.path / "cache" / target / linkerFile;
					linkerArgs += ' ';
					linkerArgs += linkerFile.string();
				}
			}
		}
	}

	cacheJson["time"] = nowTime;
	std::ofstream ofstream(project.path / "cache" / target / "cache.json");
	ofstream << cacheJson.dump(2);
	ofstream.close();

	std::cout << "done\n";

	nlohmann::json cacheJson2;
	if (std::filesystem::exists(project.path / "cache/cache.json"))
	{
		std::ifstream ifstream(project.path / "cache/cache.json");
		ifstream >> cacheJson2;
		std::string prevTarget = cacheJson2["prevTarget"];
		ifstream.close();
		if (prevTarget != target)
			builtSomething = true;
	}
	cacheJson2["prevTarget"] = target;
	std::ofstream ofstream2(project.path / "cache/cache.json");
	ofstream2 << cacheJson2.dump(2);
	ofstream2.close();

	if (toBuild.size() > 0 || builtSomething)
	{
		project.built = true;
		std::cout << "\n--- Building Target \'" << project.name << "\' ---\n";

		for (const std::filesystem::path& file : toBuild)
		{
			if (buildFile(file, project, includePaths))
			{
				std::cout << "build failed\n";
				exit(1);
			}
		}

		std::filesystem::path outFile = project.path / "bin" / project.out;

		linkerArgs += linkerLibs;

		std::string out;
		std::filesystem::create_directories(outFile.parent_path());
		std::cout << "linking ";
		std::cout.flush();
		if (project.type == ProjectType::Static)
		{
			linkerArgs = "rcs " + outFile.string() + linkerArgs;
			runCmd("ar", linkerArgs.c_str(), &out);
		}
		else
		{
			linkerArgs += " -o " + outFile.string();
			if (project.type == ProjectType::Shared)
				linkerArgs += " -shared";
			if (project.lang == ProjectLang::CPP)
				runCmd("g++", linkerArgs.c_str() + 1, &out);
			else
				runCmd("gcc", linkerArgs.c_str() + 1, &out);
		}

#ifdef _WIN64
		if (project.type == ProjectType::Program)
		{
			outFile = outFile.replace_extension(".exe");
		}
#endif

		if (std::filesystem::exists(outFile))
			std::cout << "done\n";
		else
		{
			if (project.type == ProjectType::Static)
				std::cout << "\nar " << linkerArgs << '\n' << out << "\nbuild failed\n";
			else
			{
				if (project.lang == ProjectLang::CPP)
					std::cout << "\ng++ " << (linkerArgs.c_str() + 1) << '\n' << out << "\nbuild failed\n";
				else
					std::cout << "\ngcc " << (linkerArgs.c_str() + 1) << '\n' << out << "\nbuild failed\n";
			}
			exit(1);
		}
	}
}
