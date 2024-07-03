#pragma once

#include <regex>
#include <Geode/loader/Log.hpp>
#include "../fonts/chars.hpp"

inline std::vector<std::string> lang_list = { "None (eng)", "Russian", "Ukrainian (translator)", "Spanish (translator)" };
inline std::string lang = "None (eng)";
inline void setLang(int listVecPoint) {
	lang = lang_list.size() > listVecPoint ? lang_list.at(listVecPoint) : lang_list.at(0);
};
inline auto locale_str(const char* pChar) {
	//geode::log::debug("{}(pChar \"{}\");", __FUNCTION__, pChar);
	//russian
#define NEXT_LANG_TO_SETUP "Russian" 
#include <lang/setup_next_macro.h>
	//uk_(test)
#define NEXT_LANG_TO_SETUP "Ukrainian (translator)" 
#include <lang/setup_next_macro.h>
	//Spanish (translator)
#define NEXT_LANG_TO_SETUP "Spanish (translator)"
#include <lang/setup_next_macro.h>
	//geode::log::debug("{}(pChar) return pChar \"{}\";", __FUNCTION__, pChar);
	return pChar;
}

#define STR_LOCALE(str) locale_str(str)

inline const char* operator "" _LOCALE(const char* str, size_t) {
	return locale_str(str);
}

#define LOCALE_WTH_FEATHER_ICON(icon, str) locale_str(U8STR((U8STR(icon) + std::string(str##_LOCALE)).c_str()))