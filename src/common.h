#pragma once
#ifndef __COMMON_H__
#define __COMMON_H__

//system header
#include <string>
//user header
#include "image_info.h"

#define SEPARATE_STR "\\"
#define SEPARATE_STR_LENGTH 1
#define FILE_EXTENSION_LENGTH 4

// FIXME :: 유연하게 다른 타입의 이미지 포맷을 지원할 수 있도록 추후 수정할 것
struct _ImageItem {
	std::string path;								// image file path

	_ImageFormat img_format			= _ImageFormat::kFormatNone;		// image format
	_ImageColorType img_color_type	= _ImageColorType::kColorNone;	// image color conversion
	unsigned char *data = nullptr;					// image raw data
	int width = 0;									// image width
	int height = 0;									// image height
	int channels = 0;								// image channel , ex ) RGB == 3 RGBA == 4 YUV == 3
}typedef ImageItem;

#endif //#ifndef __COMMON_H__