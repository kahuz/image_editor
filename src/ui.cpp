#include "ui.h"
// system header
#include <memory>
#include <Logger.h>
#include <iostream>
// 3rdparty header
#include <imgui.h>
#include <nfd.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// video preview시 frame 인덱스 관리를 위한 변수
int g_preview_video_idx = 0;
// modal에 표시될 string 변수
std::string g_modal_str;
// 특정 액션에 따라 modal을 띄우기 위한 boolean 변수
bool g_open_modal = false;
// image list view에서 visible이 활성화된 image item을 관리하기 위한 bool array
bool g_image_table_visible[UI_IMAGE_LIST_VIEW_ITEM_MAX];
//Image List View에서 선택된 아이템을 확인하기 위한 index value
int g_selected_list_item = -1;

//@breif UI help marker 함수
// (?) 로 표신된다
static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

//@brief : rgb, rgba raw 데이터로 gl texture를 생성하는 함수
//@param img_data 이미지 raw 데이터
//@param width 이미지 width
//@param height 이미지 height
//@param channel 이미지 channel , rgb = 3 rgba = 4
GLuint GetTextureId(const void *img_data, int width, int height, int channel)
{
	GLint prev_texture;
	GLuint tex_id;

	//현재 그리고(Rendering) 있는 텍스처 아이디를 가져온다.
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_texture);

	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	switch (channel)
	{
	case 3: // RGB
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);
		break;

	case 4: // RGBA
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img_data);
		break;
	default:
		break;
	}
	// Restore state
	// 위에서 연결된 icon 텍스처가 아닌 원래 텍스터를 다시 연결해준다.
	glBindTexture(GL_TEXTURE_2D, prev_texture);

	return tex_id;
}

//@brief : 마우스가 위치한 ImGui Window Widget에서 특정 item을 선택했는지 확인하기 위한 함수
bool IsClickItem(ImVec2 mouse_pos, ImVec2 item_area)
{
	bool ret = false;

	if (mouse_pos.x > 0 && mouse_pos.x <= item_area.x && mouse_pos.y > 0 && mouse_pos.y <= item_area.y)
	{
		auto io = ImGui::GetIO();

		int count = IM_ARRAYSIZE(io.MouseDown);

		for (int i = 0; i < count; i++)
		{
			if (ImGui::IsMouseReleased(i))
			{
				ret = true;
			}
		}
		return ret;
	}
	else
	{
		return false;
	}
}
//@breif 현재 마우스가 위치한 ImGui Window widget에 releative한 위치를 반환하는 함수
ImVec2 GetMousePosInCurrentView()
{
	ImVec2 cur_mouse_pos(0, 0);

	auto io = ImGui::GetIO();

	cur_mouse_pos.x = io.MousePos.x - ImGui::GetCursorScreenPos().x;
	cur_mouse_pos.y = io.MousePos.y - ImGui::GetCursorScreenPos().y;

	return cur_mouse_pos;
}
//@breif Image List View에서 선택된 Item의 프로퍼티 정보를 보여주는 UI
//@param items Menubar에서 선택한 옵션들을 확인하기 위한 변수
void DrawPropertyView(UIItems *items)
{
	static bool open = true;

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

	// Item Property view 의 위치, 크기 설정
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + UI_PROPERTY_VIEW_POS_X, main_viewport->WorkPos.y + UI_PROPERTY_VIEW_POS_Y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(UI_PROPERTY_VIEW_SIZE_W, UI_PROPERTY_VIEW_SIZE_H), ImGuiCond_FirstUseEver);
	// UI에 라운딩 추가, 이뻐서
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, UI_VIEW_DEFALUT_ROUND);

	ImGui::Begin(UI_PROPERTY_VIEW_TITLE, &open);
	{
		if (g_selected_list_item >= 0)
		{
			auto property_item = items->v_img_item.at(g_selected_list_item);
			std::string item_name_str = "Item name : " + property_item.path;
			std::string item_size_str = "Width : " + std::to_string(property_item.width) + "\tHeight : " + std::to_string(property_item.height);
			std::string item_format_str = "Image Format : ";
			item_format_str.append(image_format_str[property_item.img_format]);

			ImGui::Text(item_name_str.c_str());
			ImGui::Text(item_size_str.c_str());
			ImGui::Text(item_format_str.c_str());
		}
	}
	ImGui::PopStyleVar();
	ImGui::End();
}
//@breif MenuBar를 통해 open한 이미지 리스트를 보여주는 함수
//@param items Menubar에서 선택한 옵션들을 확인하기 위한 변수
void DrawImageListView(UIItems *items)
{
	static bool open = true;
	ImGuiWindowFlags img_view_flags = ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar;

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	// image list view 의 위치, 크기 설정
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + UI_IMAGE_LIST_VIEW_POS_X, main_viewport->WorkPos.y + UI_IMAGE_LIST_VIEW_POS_Y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(UI_IMAGE_LIST_VIEW_SIZE_W, UI_IMAGE_LIST_VIEW_SIZE_H), ImGuiCond_FirstUseEver);
	// UI에 라운딩 추가, 이뻐서
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, UI_VIEW_DEFALUT_ROUND);

	ImGui::Begin(UI_IMAGE_LIST_VIEW_TITLE, &open, img_view_flags);
	{
		// image list를 구성하는 테이블 요소 수
		// image, name, visible로 구성됨
		static int image_table_columns = 3;

		// image list를 표현하기 위한 table ui
		if (ImGui::BeginTable("Image Tables", image_table_columns, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
		{
			// image list를 표현하기 위한 table ui 설정
            ImGui::TableSetupColumn("Image", ImGuiTableColumnFlags_WidthFixed, UI_IMAGE_LIST_VIEW_SIZE_W / image_table_columns);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Visible", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			int cur_idx = 0;
			int active_item_num = 0;

			// open한 이미지 리스트 ( v_img_item ) 로부터 image list에 보여줄 이미지 아이템 ( item ) 를 호출
			for (auto item : items->v_img_item)
			{
				float table_column_size = UI_IMAGE_LIST_VIEW_SIZE_W / image_table_columns;
				//image list에 보여줄 tabe item의 이미지에 대해 image list 크기에 따라 비율을 결정하기 위한 부분
				float image_ratio = item.width > item.height ? item.width / table_column_size : item.height / table_column_size;

				// Image 에 해당되는 UI
				ImGui::TableNextColumn();
				{
					ImVec2 mouse_pos = GetMousePosInCurrentView();

					if (g_image_table_visible[cur_idx])
					{
						GLuint tex_id = GetTextureId(item.data, item.width, item.height, item.channels);
						ImVec2 item_area(item.width / image_ratio, item.height / image_ratio);
						//table item를 선택하였다면 select tint를 아니라면 default tint
						ImVec4 image_tint = g_selected_list_item == cur_idx ? DRAW_SELECT_TINT : DRAW_DEFAULT_TINT;

						if (IsClickItem(mouse_pos, item_area))
						{
							if (g_selected_list_item == cur_idx)
							{
								g_selected_list_item = -1;
							}
							else
							{
								image_tint = DRAW_SELECT_TINT;
								g_selected_list_item = cur_idx;
							}
						}

						ImGui::Image(
							(ImTextureID)(intptr_t)tex_id,
							item_area,
							DRAW_DEFAULT_UV_MIN,
							DRAW_DEFAULT_UV_MAX,
							image_tint,
							DRAW_DEFAULT_BORDER
						);
					}
				}

				// Name 에 해당하는 UI
				ImGui::TableNextColumn();
				{
					// for left-center alinment
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + item.height / image_ratio / 3);
					ImGui::TextWrapped(item.path.c_str());
				}

				// Visible에 해당하는 UI
				ImGui::TableNextColumn();
				{
					std::string check_item_id = item.path;
					// make checkbox cell item id
					check_item_id.insert(0,"##");

					// for center alinment
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + UI_IMAGE_LIST_VIEW_SIZE_W / image_table_columns / 3);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + item.height / image_ratio / 3);

					// visible 여부를 g_image_table_visible에 저장/관리
					ImGui::Checkbox(check_item_id.c_str(), &g_image_table_visible[cur_idx]);

					if (g_image_table_visible[cur_idx])
					{
						active_item_num++;
					}
				}
				cur_idx++;

				if (cur_idx >= UI_IMAGE_LIST_VIEW_ITEM_MAX)
				{
					break;
				}
			}

			// visible된 이미지가 하나라도 있다면, menubar 변수의 exist_active_list_item 을 true로 활성화
			if (active_item_num > 0)
			{
				items->exist_active_list_item = true;
			}
			// visible된 이미지가 없다면, menubar 변수의 exist_active_list_item 을 false로 비활성화
			else
			{
				items->exist_active_list_item = false;
			}

			ImGui::EndTable();
		}
	}
	ImGui::PopStyleVar();
	ImGui::End();
}

//@breif video preview UI 
//@param visible preview 여부
//@param img_item preview에 보여줄 이미지 아이템
void DrawVideoView(bool visible, std::vector<ImageItem> v_frame_image)
{
	if (visible)
	{
		static int frame_rate = 33;
		// FIXME :: tex_id는 preview 이미지가 변경될 때만 생성하면 된다.
		// image item의 raw data로 gl texure id 생성
		if (g_preview_video_idx >= v_frame_image.size())
		{
			g_preview_video_idx = 0;
		}
		
		ImageItem frame_item = v_frame_image[g_preview_video_idx++];

		GLuint tex_id = GetTextureId(frame_item.data, frame_item.width, frame_item.height, frame_item.channels);
		float img_width = frame_item.width;
		float img_height = frame_item.height;

		// preview의 zoom 배율 변수
		static float zoom_ratio = 1.0;
		auto io = ImGui::GetIO();
		float wheel_num = io.MouseWheel;

		zoom_ratio += io.MouseWheel / IO_MOUSE_WHEEL_RATIO;

		// 0.01 이하로는 스케일 다운되지 않음
		if (zoom_ratio < UI_ZOOM_MIN)
		{
			zoom_ratio = UI_ZOOM_MIN;
		}
		else if (zoom_ratio > UI_ZOOM_MAX)
		{
			zoom_ratio = UI_ZOOM_MAX;
		}

		ImGui::Image(
			(ImTextureID)(intptr_t)tex_id,
			ImVec2(img_width / zoom_ratio, img_height / zoom_ratio),
			DRAW_DEFAULT_UV_MIN,
			DRAW_DEFAULT_UV_MAX,
			DRAW_DEFAULT_TINT,
			DRAW_DEFAULT_BORDER
		);

		Sleep(frame_rate);	// frame rate 만큼 sleep
	}
}
//@breif 병합 혹은 raw 변환된 이미지의 preview를 제공하기 위한 UI
//@param visible preview 여부
//@param img_item preview에 보여줄 이미지 아이템
void DrawImageView(bool visible, ImageItem *img_item)
{
	if (visible)
	{
		// FIXME :: tex_id는 preview 이미지가 변경될 때만 생성하면 된다.
		// image item의 raw data로 gl texure id 생성
		GLuint tex_id = GetTextureId(img_item->data, img_item->width, img_item->height, img_item->channels);
		float img_width = img_item->width;
		float img_height = img_item->height;

		// preview의 zoom 배율 변수
		static float zoom_ratio = 1.0;
		auto io = ImGui::GetIO();
		float wheel_num = io.MouseWheel;

		zoom_ratio += io.MouseWheel / IO_MOUSE_WHEEL_RATIO;

		// 10배 줌으로 제한
		if (zoom_ratio < UI_ZOOM_MIN)
		{
			zoom_ratio = UI_ZOOM_MIN;
		}
		else if (zoom_ratio >= UI_ZOOM_MAX)
		{
			zoom_ratio = UI_ZOOM_MAX;
		}

		ImGui::Image(
			(ImTextureID)(intptr_t)tex_id,
			ImVec2(img_width / zoom_ratio, img_height / zoom_ratio),
			DRAW_DEFAULT_UV_MIN,
			DRAW_DEFAULT_UV_MAX,
			DRAW_DEFAULT_TINT,
			DRAW_DEFAULT_BORDER
		);
	}
}

//@breif 병합 혹은 raw 변환된 이미지의 preview를 제공하기 위한 UI
//@param items 보여줄 preview view를 확인하기 위한 변수
//@param view_item preview에 보여줄 이미지 아이템
void DrawPreView(UIItems *items, ImageItem *view_item)
{
	static bool open = true;
	static bool opened[kPreviewItemMax] = { true, true }; // Persistent user state
	const char* names[kPreviewItemMax] = { UI_IMAGE_VIEW_TITLE, UI_VIDEO_VIEW_TITLE};
	static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyResizeDown;
	static ImGuiTabItemFlags tab_item_flags = ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;
	static ImGuiWindowFlags preview_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar;

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	// image view 의 위치, 크기 설정
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + UI_PREVIEW_POS_X, main_viewport->WorkPos.y + UI_PREVIEW_POS_Y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(UI_PREVIEW_SIZE_W, UI_PREVIEW_SIZE_H), ImGuiCond_FirstUseEver);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, UI_VIEW_DEFALUT_ROUND);

	ImGui::Begin(UI_PREVIEW_TITLE, &open, preview_flags);
	{
		if (ImGui::BeginTabBar(UI_PREVIEW_TAB_TITLE, tab_bar_flags))
		{
			for (int tab_idx = 0; tab_idx < kPreviewItemMax; tab_idx++)
			{
				// tab item이 close 되지 않도록 open 값을 true로 강제한다
				opened[tab_idx] = true;

				if (ImGui::BeginTabItem(names[tab_idx], &opened[tab_idx]))
				{
					switch (tab_idx)
					{
					case kImageView:
						if (items->active_preview_image)
						{
							DrawImageView(true, view_item);
						}
						break;
					case kViewView:
						if (items->active_preview_video)
						{	
							DrawVideoView(true, items->v_raw_item);
						}
						break;
					}
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::PopStyleVar();
	ImGui::End();
}

//@breif image list view로 선택된 이미지의 병합 혹은 raw 변환을 위한 UI
//@param items Menubar에서 선택한 옵션들을 확인하기 위한 변수
std::string DrawImageListActionMenuBar(UIItems *items)
{
	std::string ret = "";

	// 병합 옵션 선택 구문
	if (ImGui::MenuItem("Merged Select Images"))
	{
		if (items->exist_active_list_item)
		{
			items->active_list_merge = true;
		}
		else
		{
			items->active_list_merge = false;
			ret = "If you wanna create merge texture.\nFirst, you must be open PNG files.";
		}
	}
	// raw 변환 선택 구문
	if (ImGui::MenuItem("Raw Converted Select Images"))
	{
		if (items->exist_active_list_item)
		{
			items->active_list_raw_convert = true;
		}
		else
		{
			items->active_list_raw_convert = false;
			ret = "If you wanna convert .png to .raw.\nFirst, you must be open PNG files.";
		}
	}
	return ret;
}

//@breif Files 메뉴에 해당하는 UI
//@details  File Dialog로 선택된 이미지를 바로 병합/변환 하거나 이미지 리스트에 반영하기 위한 액션을 결정하기 위한 함수
//@param items Menubar에서 선택한 옵션들을 확인하기 위한 변수
void DrawFilesMenuBar(UIItems *items)
{
	// file dialog로부터 이미지를 오픈한 뒤 바로 이미지 병합 하기 위한 구문
	if (ImGui::MenuItem("Open - Direct Merge"))
	{
		nfdpathset_t outPath;
		nfdresult_t result = NFD_OpenDialogMultiple("png", NULL, &outPath);

		if (result == NFD_OKAY)
		{
			size_t i;

			for (i = 0; i < NFD_PathSet_GetCount(&outPath); ++i)
			{
				std::string item_path = (char *)NFD_PathSet_GetPath(&outPath, i);
				items->v_open_png_path.push_back(item_path);

				Log("Open File path : %s", item_path.c_str());
			}

			items->active_direct_merge = true;

			NFD_PathSet_Free(&outPath);
		}
		else if (result == NFD_CANCEL)
		{
			items->active_direct_merge = false;
		}
		else
		{
			items->active_direct_merge = false;

			Log("Error: %s", NFD_GetError());
		}
	}

	// file dialog로부터 이미지를 오픈한 뒤 바로 raw 변환을 하기 위한 구문
	if (ImGui::MenuItem("Open - Direct Raw Convert"))
	{
		nfdchar_t *outPath = NULL;
		nfdresult_t result = NFD_OpenDialog("png", NULL, &outPath);

		if (result == NFD_OKAY)
		{
			items->active_raw_convert = true;
			items->open_raw_path = outPath;

			free(outPath);
		}
		else if (result == NFD_CANCEL)
		{
			items->active_raw_convert = false;
		}
		else
		{
			items->active_raw_convert = false;

			Log("Error: %s", NFD_GetError());
		}
	}
	// file dialog로부터 이미지를 오픈하여 iamge list view에 반영할 수 있게 하기 위한 구문
	if (ImGui::MenuItem("Open Images"))
	{
		nfdpathset_t outPath;
		nfdresult_t result = NFD_OpenDialogMultiple("png", NULL, &outPath);

		if (result == NFD_OKAY)
		{
			size_t i;

			for (i = 0; i < NFD_PathSet_GetCount(&outPath); ++i)
			{
				std::string item_path = (char *)NFD_PathSet_GetPath(&outPath, i);
				items->v_open_png_path.push_back(item_path);

				Log("Open File path : %s", item_path.c_str());
			}

			items->is_open_png_files = true;

			NFD_PathSet_Free(&outPath);
		}
		else if (result == NFD_CANCEL)
		{
			items->is_open_png_files = false;
		}
		else
		{
			items->is_open_png_files = false;

			Log("Error: %s", NFD_GetError());
		}
	}
}

//@breif MenuBar UI를 그리는 함수
//@param items Menubar에서 선택한 옵션들을 확인하기 위한 변수
void DrawMenuBar(UIItems *items)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("[ Files ]"))
		{
			DrawFilesMenuBar(items);

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("[ Image List Action ]"))
		{
			g_modal_str = DrawImageListActionMenuBar(items);

			if (g_modal_str != "")
			{
				g_open_modal = true;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (g_open_modal)
	{
		ImGui::OpenPopup("Warning");

		// Always center this window when appearing
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(300.0f, 110.0f));

		if (ImGui::BeginPopupModal("Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text(g_modal_str.c_str());

			ImGui::NewLine();
			// hard coding. center alinment for button
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 360.0f / 3);

			if (ImGui::Button("Okay"))
			{
				g_open_modal = false;
			}
			ImGui::EndPopup();
		}
	}
}

// UI와 관련된 변수 및 설정 초기화 함수
void InitUIMember()
{
	for (int i = 0; i < UI_IMAGE_LIST_VIEW_ITEM_MAX; i++)
	{
		g_image_table_visible[i] = true;
	}
	setlocale(LC_ALL, "");
}