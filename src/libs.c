#define GB_INI_IMPLEMENTATION
#include "gb_ini.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_ONLY_PNG
#define STB_ONLY_BMP
#define STB_ONLY_JPEG
#define STB_ONLY_TGA
#define STB_ONLY_GIF
#include "stb_image.h"

#ifdef _TEST
#define UTEST_C_IMPLEMENTATION
#include "utest.h"
#endif