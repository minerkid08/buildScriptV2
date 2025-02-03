#pragma once

#include "json/json.hpp"

struct Settings
{
  bool noLibs = false;
};

void parseSettings(Settings* settings, const nlohmann::json& json);
