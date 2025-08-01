#include "export/project.hpp"
#include "export/settings.hpp"
#include "json/json.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

#ifdef __linux
#define OSSTR "linux"
#endif

#ifdef _WIN64
#define OSSTR "windows"
#endif

#define getField(name, res)                                                                                            \
	if (osJson.contains(name))                                                                                         \
		res = osJson[name];                                                                                            \
	else                                                                                                               \
		res = json[name];

#define tryGetField(name, res)                                                                                         \
	if (osJson.contains(name))                                                                                         \
		res = osJson[name];                                                                                            \
	else if (json.contains(name))                                                                                      \
		res = json[name];

#define hasField(name) osJson.contains(name) | json.contains(name)

void parseSettings(Settings* settings, const nlohmann::json& json)
{
	if (json.contains("noLibs"))
		settings->noLibs = json["noLibs"];
}

void findProject(const char* name, const JsonObjects& objects, Project* project)
{
	for (const nlohmann::json& j : objects.projectJson)
	{
		std::string str;
		if (j.is_string())
		{
			str = j;
		}
		else
		{
			str = j["name"];
		}
		if (str == name)
		{
			nlohmann::json json;
			if (j.is_string())
			{
				std::ifstream stream(objects.projectPath / str / "project.json");
				stream >> json;
			}
			else
				json = j;
			project->path = objects.projectPath / str;
			parseProject(json, project);
			return;
		}
	}

	for (const nlohmann::json& j : objects.libJson)
	{
		std::string str;
		if (j.is_string())
		{
			str = j;
		}
		else
		{
			str = j["name"];
		}
		if (str == name)
		{
			nlohmann::json json;
			if (j.is_string())
			{
				std::ifstream stream(objects.libPath / str / "project.json");
				stream >> json;
			}
			else
				json = j;
			project->path = objects.libPath / str;

			if (json.contains(OSSTR))
				parseProject(json[OSSTR], project);
			else
				parseProject(json, project);
			return;
		}
	}
	std::cerr << "cant find project \'" << name << "\'\n";
	exit(1);
}

void parseProject(const nlohmann::json& json, Project* p)
{
	nlohmann::json osJson;
	if (json.contains(OSSTR))
		osJson = json[OSSTR];

	getField("name", p->name);
	tryGetField("out", p->out);

	std::string type;
	getField("type", type);
	if (type == "header")
		p->type = ProjectType::Header;
	else if (type == "program")
		p->type = ProjectType::Program;
	else if (type == "static")
		p->type = ProjectType::Static;
	else if (type == "shared")
		p->type = ProjectType::Shared;

	if (hasField("build"))
	{
		nlohmann::json build;
		getField("build", build);
		for (const std::string& str : build)
			p->build.push_back(p->path / str);
		p->build.push_back(p->path / "src");
		p->include.push_back(p->path / "src");
	}

	if (hasField("include"))
	{
		nlohmann::json include;
		getField("include", include);
		for (const std::string& str : include)
			p->include.push_back(p->path / str);
	}

	if (hasField("export"))
	{
		nlohmann::json export2;
		getField("export", export2);
		for (const std::string& str : export2)
			p->exportPaths.push_back(p->path / str);
	}

	if (hasField("libs"))
	{
		nlohmann::json libs;
		getField("libs", libs);
		for (const std::string& str : libs)
			p->libs.push_back(str);
	}

	if (hasField("compArgs"))
	{
		nlohmann::json compArgs;
		getField("compArgs", compArgs);
		for (const std::string& str : compArgs)
			p->compArgs.push_back(str);
	}

	if (hasField("linkArgs"))
	{
		nlohmann::json linkArgs;
		getField("linkArgs", linkArgs);
		for (const std::string& str : linkArgs)
			p->linkArgs.push_back(str);
	}

	if (hasField("lang"))
	{
		std::string lang;
		getField("lang", lang);
		if (lang == "cpp")
			p->lang = ProjectLang::CPP;
		else
			p->lang = ProjectLang::C;
	}
	else
	{
		p->lang = ProjectLang::CPP;
	}

	if (osJson.contains("targetArgs"))
	{
		const nlohmann::json& targetArgs = osJson["targetArgs"];
		for (const auto& args : targetArgs.items())
			p->configArgs[args.key()] = args.value();
	}
	if (json.contains("targetArgs"))
	{
		const nlohmann::json& targetArgs = json["targetArgs"];
		for (const auto& args : targetArgs.items())
		{
			if (p->configArgs.find(args.key()) == p->configArgs.end())
				p->configArgs[args.key()] = args.value();
		}
	}

	tryGetField("autoBuild", p->autoBuild);
}
