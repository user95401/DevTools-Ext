if (lang == NEXT_LANG_TO_SETUP) {

#define ADD(_org, _new) if (std::string(pChar) == (_org)) pChar = (_new);
#define ADD_UTF8(_org, _new) if (std::string(pChar) == (_org)) pChar = (U8STR(u8##_new));
#include NEXT_LANG_TO_SETUP
#undef ADD_UTF8
#undef ADD

#undef NEXT_LANG_TO_SETUP

}