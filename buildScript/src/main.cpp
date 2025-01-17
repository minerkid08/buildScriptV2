#include "process.hpp"
#include "project.hpp"

#include "json/json.hpp"
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

enum class Mode
{
	Build,
	Configure
};

Mode mode = Mode::Build;

std::string target = "debug";

unsigned long lastCompileTime;
unsigned long nowTime;

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
	if (project.lang == ProjectLang::CPP)
		runCmd("g++", args.c_str(), &out);
	else
		runCmd("gcc", args.c_str(), &out);
	if (std::filesystem::exists(outFile))
	{
		std::cout << " done\n";
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

void buildProject(Project& project, const std::vector<Project>& projects);
void buildProjectTree(Project& project, std::vector<Project>& projects);

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
      char* c = 0;
      execvp("buildCfg", &c);
		}
	}

	std::string proj;

	proj = argv[1];
	if (argc == 3)
		target = argv[2];

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

	if (mode == Mode::Configure)
	{
		std::string out;
		for (const std::filesystem::path& path : projects[0].include)
		{
			out += "-I" + path.string() + '\n';
		}
		for (const Project& p : projects)
		{
			for (const std::string& s : projects[0].libs)
			{
				if (p.name != s)
					continue;
				for (const std::filesystem::path& path : p.exportPaths)
				{
					out += "-I" + path.string() + '\n';
				}
			}
		}
		std::ofstream stream("compile-flags.txt");
		stream << out;
		stream.close();
	}

	if (mode == Mode::Build)
	{
		long now = std::time(0);
		struct tm* tm = gmtime(&now);
		nowTime = timegm(tm);

		buildProjectTree(projects[0], projects);

		std::cout << "build sucessful\n";
	}
}

void buildProjectTree(Project& project, std::vector<Project>& projects)
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
	buildProject(project, projects);
}

void buildProject(Project& project, const std::vector<Project>& projects)
{
	project.processed = true;
	if (project.type == ProjectType::Header)
		return;

	nlohmann::json cacheJson;
	if (std::filesystem::exists(project.path / "cache.json"))
	{
		std::ifstream ifstream(project.path / "cache.json");
		ifstream >> cacheJson;
		lastCompileTime = cacheJson["time"];
		ifstream.close();
	}
	else
		lastCompileTime = 0;

	std::vector<std::filesystem::path> toBuild;
	std::vector<std::filesystem::path> toProcess;

	for (const std::string& str : project.build)
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
					linkerArgs += ' ';
					linkerArgs += linkerFile.string();
				}
			}
		}
	}

	cacheJson["time"] = nowTime;
	std::ofstream ofstream(project.path / "cache.json");
	ofstream << cacheJson.dump(2);
	ofstream.close();

	if (toBuild.size() > 0 || builtSomething)
	{
		project.built = true;
		std::cout << "--- Building Target \'" << project.name << "\' ---\n";

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
			if (project.lang == ProjectLang::CPP)
				runCmd("g++", linkerArgs.c_str() + 1, &out);
			else
				runCmd("gcc", linkerArgs.c_str() + 1, &out);
		}

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
