#include "export/project.hpp"
#include "export/settings.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

void parseSettings(Settings* settings, const nlohmann::json& json)
{
  if(json.contains("noLibs"))
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
			parseProject(json, project);
			return;
		}
	}
	std::cerr << "cant find project \'" << name << "\'\n";
	exit(1);
}

void parseProject(const nlohmann::json& json, Project* p)
{
	p->name = json["name"];
	if (json.contains("out"))
		p->out = json["out"];

	const std::string& type = json["type"];
	if (type == "header")
		p->type = ProjectType::Header;
	else if (type == "program")
		p->type = ProjectType::Program;
	else if (type == "static")
		p->type = ProjectType::Static;
	else if (type == "shared")
		p->type = ProjectType::Shared;

	if (json.contains("build"))
	{
		for (const std::string& str : json["build"])
			p->build.push_back(p->path / str);
		p->build.push_back(p->path / "src");
		p->include.push_back(p->path / "src");
	}

	if (json.contains("include"))
	{
		for (const std::string& str : json["include"])
			p->include.push_back(p->path / str);
	}

	if (json.contains("export"))
	{
		for (const std::string& str : json["export"])
			p->exportPaths.push_back(p->path / str);
	}

	if (json.contains("libs"))
	{
		for (const std::string& str : json["libs"])
			p->libs.push_back(str);
	}

	if (json.contains("compArgs"))
	{
		for (const std::string& str : json["compArgs"])
			p->compArgs.push_back(str);
	}

	if (json.contains("linkArgs"))
	{
		for (const std::string& str : json["linkArgs"])
			p->linkArgs.push_back(str);
	}

	if (json.contains("lang"))
	{
		std::string lang = json["lang"];
		if (lang == "cpp")
			p->lang = ProjectLang::CPP;
		else
			p->lang = ProjectLang::C;
	}
	else
	{
		p->lang = ProjectLang::CPP;
	}
  
  if(json.contains("autoBuild"))
  {
    p->autoBuild = json["autoBuild"];
  }
}
