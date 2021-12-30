#pragma once
#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>

#define SEPARATE_STR "\\"
#define SEPARATE_STR_LENGTH 1
#define PNG_EXTENSION_LENGTH 4

// FIXME :: 유연하게 다른 타입의 이미지 포맷을 지원할 수 있도록 추후 수정할 것
struct _PNGItem {
	unsigned char *data = nullptr;
	std::string path;
	int width;
	int height;
	int Channels;
}typedef PNGItem;

#endif //#ifndef __COMMON_H__