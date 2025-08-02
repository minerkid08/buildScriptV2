#include "project.hpp"
#include "settings.hpp"

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

	// for (const nlohmann::json& j : objects.projectJson)
	//{
	//	Project p;
	//	if (j.is_string())
	//		findProject(((std::string)j).c_str(), objects, &p);
	//	else
	//	{
	//		p.path = objects.projectPath / j["name"];
	//		parseProject(j, &p);
	//	}

	//	printf("\n--- Project: '%s' ---\n", p.name.c_str());
	//	switch (p.type)
	//	{
	//	case ProjectType::Program:
	//		printf("Type: program\n");
	//		break;
	//	case ProjectType::Static:
	//		printf("Type: static\n");
	//		break;
	//	case ProjectType::Header:
	//		printf("Type: header\n");
	//		break;
	//	case ProjectType::Shared:
	//		printf("Type: shared\n");
	//		break;
	//	}
	//	std::vector<Project> libs;
	//	for (const std::string& lib : p.libs)
	//	{
	//		libs.emplace_back();
	//		Project* l = &libs[libs.size() - 1];
	//		findProject(lib.c_str(), objects, l);
	//	}
	//	printf("Include Paths:\n");
	//	for (const std::filesystem::path& str : p.include)
	//	{
	//		printf("  %s\n", str.string().c_str());
	//	}
	//	for (const Project lib : libs)
	//	{
	//		for (const std::filesystem::path& str : lib.include)
	//			printf("  %s\n", str.string().c_str());
	//	}
	//	printf("Build Paths:\n");
	//	for (const std::filesystem::path& str : p.build)
	//	{
	//		printf("  %s\n", str.string().c_str());
	//	}
	//	printf("Link Args:\n");
	//	for (const std::filesystem::path& str : p.linkArgs)
	//	{
	//		printf("  %s\n", str.string().c_str());
	//	}
	//	for (const Project lib : libs)
	//	{
	//		if (lib.type == ProjectType::Header || lib.type == ProjectType::Static)
	//		{
	//			for (const std::filesystem::path& str : lib.linkArgs)
	//				printf("  %s\n", str.string().c_str());
	//		}
	//	}
	//}
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
		std::string out = "CompileFlags:\n  Add: [\n";
		for (const std::filesystem::path& str : p.libs)
		{
			Project p2;
			findProject(str.string().c_str(), objects, &p2);
			for (const std::filesystem::path& path : p2.exportPaths)
			{
				out += "    -I";
				out += std::filesystem::absolute(path).string();
				out += ",\n";
			}
		}
		for (const std::filesystem::path& path : p.include)
		{
			out += "    -I";
			out += std::filesystem::absolute(path).string();
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
#ifdef _WIN64
	std::ofstream compileFlags(".\\compile_flags.txt");
	compileFlags << "--target=x86_64-w64-mingw64";
#endif
}
