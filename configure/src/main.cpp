#include "project.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

int main()
{
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

	for (const nlohmann::json& j : objects.projectJson)
	{
		Project p;
		if (j.is_string())
			findProject(((std::string)j).c_str(), objects, &p);
		else
		{
			p.path = objects.projectPath / j["name"];
			parseProject(j, &p);
		}
		std::cout << "configuring: " << p.name << '\n';
		std::string out = "CompileFlags:\n  Add: [\n\n";
		for (const std::string& str : p.libs)
		{
			Project p2;
			findProject(str.c_str(), objects, &p2);
			for (const std::filesystem::path& path : p2.exportPaths)
			{
				out += "    -I";
				out += path.string();
				out += ",\n";
			}
		}
		for (const std::filesystem::path& path : p.include)
		{
			out += "    -I";
			out += path.string();
			out += ",\n";
		}
    out[out.size() - 2] = '\n';
    out[out.size() - 1] = ']';
    std::filesystem::create_directories(p.path / "src");
    std::ofstream stream(p.path / ".clangd");
    stream << out;
    std::ofstream stream2(p.path / ".gitignore");
    stream2 << "bin/*\ncache/*\ncache.json\n.clangd";
	}
}
