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

// video preview�� frame �ε��� ������ ���� ����
int g_preview_video_idx = 0;
// modal�� ǥ�õ� string ����
std::string g_modal_str;
// preview�� zoom ���� ����
float g_zoom_ratio = 1.0;
// Ư�� �׼ǿ� ���� modal�� ���� ���� boolean ����
bool g_open_modal = false;
// image view�� ���� zoom Ȱ��ȭ ���� ����
bool g_zoom_state = false;
// image list view���� ���õ� image item�� �����ϱ� ���� bool array
bool g_image_table_selected[UI_IMAGE_LIST_VIEW_ITEM_MAX];

//@breif UI help marker �Լ�
// (?) �� ǥ�ŵȴ�
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

// brief : rgb, rgba raw �����ͷ� gl texture�� �����ϴ� �Լ�
//@param img_data �̹��� raw ������
//@param width �̹��� width
//@param height �̹��� height
//@param channel �̹��� channel , rgb = 3 rgba = 4
GLuint GetTextureId(const void *img_data, int width, int height, int channel)
{
	GLint prev_texture;
	GLuint tex_id;

	//���� �׸���(Rendering) �ִ� �ؽ�ó ���̵� �����´�.
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
	// ������ ����� icon �ؽ�ó�� �ƴ� ���� �ؽ��͸� �ٽ� �������ش�.
	glBindTexture(GL_TEXTURE_2D, prev_texture);

	return tex_id;
}
//@breif image tool�� ���� ������ �����ϴ� UI
//@param items Menubar���� ������ �ɼǵ��� Ȯ���ϱ� ���� ����
void DrawSettingsView(MenuItems *items)
{
	static bool open = true;
	static std::string zoom_btn_str("Zoom Off");

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	// image list view �� ��ġ, ũ�� ����
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + UI_SETTINGS_VIEW_POS_X, main_viewport->WorkPos.y + UI_SETTINGS_VIEW_POS_Y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(UI_SETTINGS_VIEW_SIZE_W, UI_SETTINGS_VIEW_SIZE_H), ImGuiCond_FirstUseEver);
	// UI�� ���� �߰�, �̻���
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, UI_VIEW_DEFALUT_ROUND);

	ImGui::Begin(UI_SETTINGS_VIEW_TITLE, &open);
	{
		if (g_zoom_state)
		{
			zoom_btn_str = "Zoom On";
		}
		else
		{
			zoom_btn_str = "Zoom off";
		}

		if (ImGui::Button(zoom_btn_str.c_str()))
		{
			g_zoom_state = !g_zoom_state;
		}

	}
	ImGui::PopStyleVar();
	ImGui::End();
}
//@breif MenuBar�� ���� open�� �̹��� ����Ʈ�� �����ִ� �Լ�
//@param items Menubar���� ������ �ɼǵ��� Ȯ���ϱ� ���� ����
void DrawImageListView(MenuItems *items)
{
	static bool open = true;
	ImGuiWindowFlags img_view_flags = ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar;

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	// image list view �� ��ġ, ũ�� ����
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + UI_IMAGE_LIST_VIEW_POS_X, main_viewport->WorkPos.y + UI_IMAGE_LIST_VIEW_POS_Y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(UI_IMAGE_LIST_VIEW_SIZE_W, UI_IMAGE_LIST_VIEW_SIZE_H), ImGuiCond_FirstUseEver);
	// UI�� ���� �߰�, �̻���
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, UI_VIEW_DEFALUT_ROUND);

	ImGui::Begin(UI_IMAGE_LIST_VIEW_TITLE, &open, img_view_flags);
	{
		// image list�� �����ϴ� ���̺� ��� ��
		// image, name, visible�� ������
		static int image_table_columns = 3;

		// image list�� ǥ���ϱ� ���� table ui
		if (ImGui::BeginTable("Image Tables", image_table_columns, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings))
		{
			// image list�� ǥ���ϱ� ���� table ui ����
            ImGui::TableSetupColumn("Image", ImGuiTableColumnFlags_WidthFixed, UI_IMAGE_LIST_VIEW_SIZE_W / image_table_columns);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Visible", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();

			int cur_idx = 0;
			int active_item_num = 0;

			// open�� �̹��� ����Ʈ ( v_png_item ) �κ��� image list�� ������ �̹��� ������ ( item ) �� ȣ��
			for (auto item : items->v_png_item)
			{
				float table_column_size = UI_IMAGE_LIST_VIEW_SIZE_W / image_table_columns;
				float image_ratio = item.width > item.height ? item.width / table_column_size : item.height / table_column_size;

				// Image �� �ش�Ǵ� UI
				ImGui::TableNextColumn();
				{
					if (g_image_table_selected[cur_idx])
					{
						GLuint tex_id = GetTextureId(item.data, item.width, item.height, item.Channels);

						ImGui::Image(
							(ImTextureID)(intptr_t)tex_id,
							ImVec2(item.width / image_ratio, item.height / image_ratio),
							DRAW_DEFAULT_UV_MIN,
							DRAW_DEFAULT_UV_MAX,
							DRAW_DEFAULT_TINT,
							DRAW_DEFAULT_BORDER
						);
					}
				}

				// Name �� �ش��ϴ� UI
				ImGui::TableNextColumn();
				{
					// for left-center alinment
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + item.height / image_ratio / 3);
					ImGui::TextWrapped(item.path.c_str());
				}

				// Visible�� �ش��ϴ� UI
				ImGui::TableNextColumn();
				{
					std::string check_item_id = item.path;
					// make checkbox cell item id
					check_item_id.insert(0,"##");

					// for center alinment
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + UI_IMAGE_LIST_VIEW_SIZE_W / image_table_columns / 3);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + item.height / image_ratio / 3);

					// visible ���θ� g_image_table_selected�� ����/����
					ImGui::Checkbox(check_item_id.c_str(), &g_image_table_selected[cur_idx]);

					if (g_image_table_selected[cur_idx])
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

			// visible�� �̹����� �ϳ��� �ִٸ�, menubar ������ exist_active_list_item �� true�� Ȱ��ȭ
			if (active_item_num > 0)
			{
				items->exist_active_list_item = true;
			}
			// visible�� �̹����� ���ٸ�, menubar ������ exist_active_list_item �� false�� ��Ȱ��ȭ
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
//@param visible preview ����
//@param img_item preview�� ������ �̹��� ������
void DrawVideoView(bool visible, std::vector<PNGItem> v_frame_image)
{
	if (visible)
	{
		// FIXME :: tex_id�� preview �̹����� ����� ���� �����ϸ� �ȴ�.
		// image item�� raw data�� gl texure id ����
		if (g_preview_video_idx >= v_frame_image.size())
		{
			g_preview_video_idx = 0;
		}
		
		PNGItem frame_item = v_frame_image[g_preview_video_idx++];

		GLuint tex_id = GetTextureId(frame_item.data, frame_item.width, frame_item.height, frame_item.Channels);
		float img_width = frame_item.width;
		float img_height = frame_item.height;

		if (g_zoom_state)
		{
			auto io = ImGui::GetIO();
			float wheel_num = io.MouseWheel;

			g_zoom_ratio += io.MouseWheel / IO_MOUSE_WHEEL_RATIO;

			// 0.01 ���Ϸδ� ������ �ٿ���� ����
			if (g_zoom_ratio < UI_ZOOM_MIN)
			{
				g_zoom_ratio = UI_ZOOM_MIN;
			}
			else if (g_zoom_ratio > UI_ZOOM_MAX)
			{
				g_zoom_ratio = UI_ZOOM_MAX;
			}
		}

		ImGui::Image(
			(ImTextureID)(intptr_t)tex_id,
			ImVec2(img_width / g_zoom_ratio, img_height / g_zoom_ratio),
			DRAW_DEFAULT_UV_MIN,
			DRAW_DEFAULT_UV_MAX,
			DRAW_DEFAULT_TINT,
			DRAW_DEFAULT_BORDER
		);

		Sleep(33);
	}
}
//@breif ���� Ȥ�� raw ��ȯ�� �̹����� preview�� �����ϱ� ���� UI
//@param visible preview ����
//@param img_item preview�� ������ �̹��� ������
void DrawImageView(bool visible, PNGItem *img_item)
{
	if (visible)
	{
		// FIXME :: tex_id�� preview �̹����� ����� ���� �����ϸ� �ȴ�.
		// image item�� raw data�� gl texure id ����
		GLuint tex_id = GetTextureId(img_item->data, img_item->width, img_item->height, img_item->Channels);
		float img_width = img_item->width;
		float img_height = img_item->height;
		
		if (g_zoom_state)
		{
			auto io = ImGui::GetIO();
			float wheel_num = io.MouseWheel;

			g_zoom_ratio += io.MouseWheel / IO_MOUSE_WHEEL_RATIO;

			// 0.01 ���Ϸδ� ������ �ٿ���� ����
			if (g_zoom_ratio < UI_ZOOM_MIN)
			{
				g_zoom_ratio = UI_ZOOM_MIN;
			}
			else if (g_zoom_ratio > UI_ZOOM_MAX)
			{
				g_zoom_ratio = UI_ZOOM_MAX;
			}
		}

		ImGui::Image(
			(ImTextureID)(intptr_t)tex_id,
			ImVec2(img_width / g_zoom_ratio, img_height / g_zoom_ratio),
			DRAW_DEFAULT_UV_MIN,
			DRAW_DEFAULT_UV_MAX,
			DRAW_DEFAULT_TINT,
			DRAW_DEFAULT_BORDER
		);
	}
}

//@breif ���� Ȥ�� raw ��ȯ�� �̹����� preview�� �����ϱ� ���� UI
//@param items ������ preview view�� Ȯ���ϱ� ���� ����
//@param view_item preview�� ������ �̹��� ������
void DrawPreView(MenuItems *items, PNGItem *view_item)
{
	static bool open = true;
	static bool opened[kPreviewItemMax] = { true, true }; // Persistent user state
	const char* names[kPreviewItemMax] = { UI_IMAGE_VIEW_TITLE, UI_VIDEO_VIEW_TITLE};
	static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyResizeDown;
	static ImGuiTabItemFlags tab_item_flags = ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;
	static ImGuiWindowFlags preview_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar;

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	// image view �� ��ġ, ũ�� ����
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + UI_PREVIEW_POS_X, main_viewport->WorkPos.y + UI_PREVIEW_POS_Y), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(UI_PREVIEW_SIZE_W, UI_PREVIEW_SIZE_H), ImGuiCond_FirstUseEver);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, UI_VIEW_DEFALUT_ROUND);

	ImGui::Begin(UI_PREVIEW_TITLE, &open, preview_flags);
	{
		if (ImGui::BeginTabBar(UI_PREVIEW_TAB_TITLE, tab_bar_flags))
		{
			for (int tab_idx = 0; tab_idx < kPreviewItemMax; tab_idx++)
			{
				// tab item�� close ���� �ʵ��� open ���� true�� �����Ѵ�
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

//@breif image list view�� ���õ� �̹����� ���� Ȥ�� raw ��ȯ�� ���� UI
//@param items Menubar���� ������ �ɼǵ��� Ȯ���ϱ� ���� ����
std::string DrawImageListActionMenuBar(MenuItems *items)
{
	std::string ret = "";

	// ���� �ɼ� ���� ����
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
	// raw ��ȯ ���� ����
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

//@breif Files �޴��� �ش��ϴ� UI
//@details  File Dialog�� ���õ� �̹����� �ٷ� ����/��ȯ �ϰų� �̹��� ����Ʈ�� �ݿ��ϱ� ���� �׼��� �����ϱ� ���� �Լ�
//@param items Menubar���� ������ �ɼǵ��� Ȯ���ϱ� ���� ����
void DrawFilesMenuBar(MenuItems *items)
{
	// file dialog�κ��� �̹����� ������ �� �ٷ� �̹��� ���� �ϱ� ���� ����
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

	// file dialog�κ��� �̹����� ������ �� �ٷ� raw ��ȯ�� �ϱ� ���� ����
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
	// file dialog�κ��� �̹����� �����Ͽ� iamge list view�� �ݿ��� �� �ְ� �ϱ� ���� ����
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

//@breif MenuBar UI�� �׸��� �Լ�
//@param items Menubar���� ������ �ɼǵ��� Ȯ���ϱ� ���� ����
void DrawMenuBar(MenuItems *items)
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

// UI�� ���õ� ���� �� ���� �ʱ�ȭ �Լ�
void InitUIMember()
{
	for (int i = 0; i < UI_IMAGE_LIST_VIEW_ITEM_MAX; i++)
	{
		g_image_table_selected[i] = true;
	}
	setlocale(LC_ALL, "");
}