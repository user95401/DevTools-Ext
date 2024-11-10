#pragma once

#include <imgui.h>
#include <string>
#include <vector>

void applyTheme(std::string const& theme);

std::vector<std::string> getThemeOptions();
size_t getThemeIndex(std::string const& theme);
std::string getThemeAtIndex(size_t index);
