#pragma once

#include "json/json.hpp"
#include <string>
#include <vector>
#include <filesystem>

enum class ProjectType
{
  Program, Static, Header
};

enum class ProjectLang
{
  CPP, C
};

struct Project
{
	std::string name;
	std::string out;
  ProjectType type;
  ProjectLang lang;
  std::filesystem::path path;
	std::vector<std::string> build;
	std::vector<std::string> libs;
	std::vector<std::string> include;
	std::vector<std::string> exportPaths;

  bool built = false;
  bool processed = false;
};

struct JsonObjects
{
  nlohmann::json projectJson;
  nlohmann::json libJson;
  std::filesystem::path projectPath;
  std::filesystem::path libPath;
};

void parseProject(const nlohmann::json& json, Project* p);
void findProject(const char* name, const JsonObjects& objects, Project* project);
