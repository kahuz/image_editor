#pragma once
#ifndef __IMAGE_INFO_H__
#define __IMAGE_INFO_H__

enum _ImageFormat {
	kNone = 0,
	kRGB,
	kRGBA,
	kBGR,
	kBGRA,
	kYUV420P,
	kYUV422P,
	kYVU420P,
	kYVU422P,
	kFormatMax
}typedef ImageFormat;

static const char* image_format_str[kFormatMax] = {
	"Invalid",
	"RGB",
	"RGBA",
	"BGR",
	"BGRA",
	"YUV420P",
	"YUU422P",
	"YVU420P",
	"YVU422P"
};

#endif //#ifndef __IMAGE_INFO_H__