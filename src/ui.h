#pragma once
#ifndef __UI_H__
#define __UI_H__
//system header
#include <string>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
// user header
#include "common.h"

#define IO_MOUSE_WHEEL_RATIO	10.0f
#define UI_ZOOM_MIN	0.1f
#define UI_ZOOM_MAX	10.0f

#define DRAW_DEFAULT_UV_MIN		ImVec2(0.0f, 0.0f)
#define DRAW_DEFAULT_UV_MAX		ImVec2(1.0f, 1.0f)
#define DRAW_DEFAULT_TINT		ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
#define DRAW_SELECT_TINT		ImVec4(1.0f, 1.0f, 0.5f, 0.2f)
#define DRAW_DEFAULT_BORDER		ImVec4(1.0f, 1.0f, 1.0f, 0.0f)

#define UI_WINDOW_SIZE_W		1280.0f
#define UI_WINDOW_SIZE_H		720.0f
#define UI_VIEW_DEFALUT_MARGIN	20.0f
#define UI_VIEW_DEFALUT_ROUND	5.0f

#define UI_VIDEO_VIEW_TITLE		"Video View"
#define UI_VIDEO_VIEW_SIZE_W	800.0f
#define UI_VIDEO_VIEW_SIZE_H	600.0f
#define UI_VIDEO_VIEW_POS_X		UI_VIEW_DEFALUT_MARGIN
#define UI_VIDEO_VIEW_POS_Y		40.0f

#define UI_PREVIEW_TITLE		"PreView"
#define UI_PREVIEW_TAB_TITLE	"##PreView Tab bar"
#define UI_PREVIEW_SIZE_W		800.0f
#define UI_PREVIEW_SIZE_H		600.0f
#define UI_PREVIEW_POS_X		UI_VIEW_DEFALUT_MARGIN
#define UI_PREVIEW_POS_Y		40.0f

#define UI_IMAGE_VIEW_TITLE		"Image View"
#define UI_IMAGE_VIEW_SIZE_W	800.0f
#define UI_IMAGE_VIEW_SIZE_H	600.0f
#define UI_IMAGE_VIEW_POS_X		UI_VIEW_DEFALUT_MARGIN
#define UI_IMAGE_VIEW_POS_Y		40.0f

#define UI_PROPERTY_VIEW_TITLE "Item Property View"
#define UI_PROPERTY_VIEW_SIZE_W	400.0f
#define UI_PROPERTY_VIEW_SIZE_H	200.0f
#define UI_PROPERTY_VIEW_POS_X	UI_IMAGE_VIEW_POS_X + UI_IMAGE_VIEW_SIZE_W + UI_VIEW_DEFALUT_MARGIN
#define UI_PROPERTY_VIEW_POS_Y	UI_IMAGE_VIEW_POS_Y

#define UI_IMAGE_LIST_VIEW_TITLE	"Image List View"
#define UI_IMAGE_LIST_VIEW_SIZE_W	400.0f
#define UI_IMAGE_LIST_VIEW_SIZE_H	UI_IMAGE_VIEW_SIZE_H - UI_PROPERTY_VIEW_SIZE_H - UI_VIEW_DEFALUT_MARGIN
#define UI_IMAGE_LIST_VIEW_POS_X	UI_PROPERTY_VIEW_POS_X
#define UI_IMAGE_LIST_VIEW_POS_Y	UI_IMAGE_VIEW_POS_Y + UI_PROPERTY_VIEW_SIZE_H + UI_VIEW_DEFALUT_MARGIN

// open image list¿« max value
#define UI_IMAGE_LIST_VIEW_ITEM_MAX 100

#define UI_PREVIEW_TAB_IMAGE		0
#define UI_PREVIEW_TAB_VIDEO		1

enum PREVIEW_TAB_ITEMS {
	kImageView = 0,
	kViewView,
	kPreviewItemMax
};

struct _UIItems {
	std::vector<std::string> v_open_png_path;
	std::vector<ImageItem> v_img_item;
	std::vector<ImageItem> v_raw_item;
	std::string open_raw_path;

	bool active_direct_merge = false;
	bool active_list_merge = false;
	bool active_raw_convert = false;
	bool active_list_raw_convert = false;
	bool active_preview_image = false;
	bool active_preview_video = false;

	bool exist_active_list_item = false;

	bool is_open_png_files = false;
	bool is_make_raw = false;
	bool is_make_merge = false;

}typedef UIItems;

void DrawPropertyView(UIItems *items);
void DrawImageListView(UIItems *items);
void DrawPreView(UIItems *items, ImageItem *view_item);
void DrawVideoView(bool visible, std::vector<ImageItem> v_frame_image);
void DrawImageView( bool visible, ImageItem *img_item);

void DrawMenuBar(UIItems *items);
void DrawFilesMenuBar(UIItems *items);

void InitUIMember();

extern bool g_image_table_visible[UI_IMAGE_LIST_VIEW_ITEM_MAX];
#endif //#ifndef __UI_H__