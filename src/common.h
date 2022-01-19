#pragma once
#ifndef __COMMON_H__
#define __COMMON_H__

//system header
#include <string>
//user header
#include "image_info.h"

#define SEPARATE_STR "\\"
#define SEPARATE_STR_LENGTH 1
#define PNG_EXTENSION_LENGTH 4	// png file extension length

// FIXME :: �����ϰ� �ٸ� Ÿ���� �̹��� ������ ������ �� �ֵ��� ���� ������ ��
struct _ImageItem {
	std::string path;								// image file path

	ImageFormat img_format = ImageFormat::kNone;	// image format
	unsigned char *data = nullptr;					// image raw data
	int width;										// image width
	int height;										// image height
	int channels;									// image channel , ex ) RGB == 3 RGBA == 4 YUV == 3
}typedef ImageItem;

#endif //#ifndef __COMMON_H__