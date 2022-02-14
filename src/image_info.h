#pragma once
#ifndef __IMAGE_INFO_H__
#define __IMAGE_INFO_H__

#define RGBA_R_CAHNNEL	0
#define RGBA_G_CAHNNEL	1
#define RGBA_B_CAHNNEL	2
#define RGBA_A_CAHNNEL	3
#define RGB_CAHNNEL		3
#define RGBA_CAHNNEL	4

#define CLIP(x) (x < 0 ? 0 : (x > 255 ? 255 : x))

enum _ImageFormat {
	kFormatNone = 0,
	kRGB,
	kRGBA,
	kBGR,
	kBGRA,
	kYUV420P,
	kYUV422P,
	kYVU420P,
	kYVU422P,
	kFormatMax
};

enum _ImageColorType {
	kColorNone = 0,
	kITU_R_BT470,
	kITU_R_BT601,
	kITU_R_BT709,
	kColorTypeMax
};

static const char* image_format_str[kFormatMax] = {
	"Invalid",
	"RGB",
	"RGBA",
	"BGR",
	"BGRA",
	"YUV420P",
	"YUV422P",
	"YVU420P",
	"YVU422P"
};

static const char* image_color_type_str[kColorTypeMax] = {
	"Invalid",
	"ITU-R.BT470",
	"ITU-R.BT601",
	"ITU-R.BT709"
};

#endif //#ifndef __IMAGE_INFO_H__