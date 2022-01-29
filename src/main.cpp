// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// system header
#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdio.h>
#include <vector>
#include <memory>
#include <Logger.h>
#include <algorithm>
#include <memory>
#include <iostream>
#include <fstream>
// 3rdparty header
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
// user header
#include "ui.h"
#include "common.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// menubar �ɼ� ������ ���� ���� ����
static UIItems g_menu_items;
// FIXME :: ���� ������ ���� merge_num�� ��ü�ϴ� str�� ����� ��
// ���յ� �̹����� �̹��� ���� ������ ���� idx ��
static int merge_num = 1;
// �̹��� ���� �� ���Ǵ� ���� ��. gl mipmap �ɼ��� ���� �̹��� ���� �� �����¿� IMG_MARGIN_PIXEL ��ŭ ���� ���� ���� ( alpha 100% )
#define IMG_MARGIN_PIXEL 8

typedef uint32_t fourcc_t;

#define fourcc_(a,b,c,d) (\
			(((fourcc_t)(d)) << 24u) | (((fourcc_t)(c)) << 16u) | \
			(((fourcc_t)(b)) << 8u) | (fourcc_t)(a) \
		)

//@breif glfw error �ݹ� �Լ�
//@param error error code
//@param description error description
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

//@breif �̹����� width ���� �������� ���� �Լ�
//@param a ���� �� ��� a
//@param b ���� �� ��� b
bool ItemWidthSort(const ImageItem& a, const ImageItem& b)
{
	if (a.width > b.width)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//@breif �̹����� height ���� �������� ���� �Լ�
//@param a ���� �� ��� a
//@param b ���� �� ��� b
bool ItemHeightSort(const ImageItem& a, const ImageItem& b)
{
	if (a.height > b.height)
	{
		return true;
	}
	else
	{
		return false;
	}
}
//@breif �ȼ� ��ǥ�� UV ��ǥ�� ��ȯ�ϴ� �Լ�
//@param tex_size �ȼ� ������ �� �ؽ�ó ũ��
//@param pos ��ȯ�� ��ǥ ��ġ
//@param size  UV ������ �� �ؽ�ó ũ��
ImVec4 Pixel2UV(ImVec2 tex_size, ImVec2 pos, ImVec2 size)
{
	ImVec4 res;

	double w = 1.0f / tex_size.x;
	double h = 1.0f / tex_size.y;

	res.x = (pos.x * w) > 1 ? 1.0f : (pos.x * w);
	res.y = (pos.y * h) > 1 ? 1.0f : (pos.y * h);
	res.w = res.x + (size.x * w) > 1 ? 1.0f : res.x + (size.x * w);
	res.z = res.y + (size.y * h) > 1 ? 1.0f : res.y + (size.y * h);

	return res;
}

//@breif ���յ� �̹����� ���� �⺻ �������� �ʱ�ȭ�ϴ� �Լ�
//@details  ������ �̹��� ����Ʈ ( v_img_item ) ������ �о�鿩 ������ �ؽ�ó�� ���� width, height, channel ����
// ���յ� �̹����� ��θ� �����Ѵ�.
// ���յ� �̹����� �ʺ�� �̹��� ����Ʈ �� �ʺ� ���� ū ������, ���̴� ���ĵ� �̹������� �� ������ �����ȴ�
//@param v_img_item ���տ� ���� �̹��� ����Ʈ
//@param merged_item ���� ��� �̹���
void CreateMergedImageInfo(std::vector<ImageItem> v_img_item, ImageItem * merged_item)
{
	//���յ� �̹����� ������ ��� ����
	merged_item->path = "merged_image_" + std::to_string(merge_num) + ".png";
	//sort�� ���̱⿡ �׻� 0��° �ε����� width�� �ִ밪
	merged_item->width = v_img_item[0].width;
	merged_item->height = 0;
	//���� ������ �� �׻� ���İ� ���Ե� �̹����̹Ƿ� 4channel�� ����
	merged_item->channels = 4;
	int sum_item_width = 0;
	int cur_step_max_height = 0;

	// �̹��� ����Ʈ�κ��� ������ �̹����� ȣ��
	for (auto item : v_img_item)
	{
		// �̹��� ����Ʈ �� �ʺ� ���� ū ���� ���ϱ� ���� ����
		int compare_sum = sum_item_width + item.width;

		//next step
		if (compare_sum > merged_item->width)
		{
			merged_item->height += cur_step_max_height;
			sum_item_width = item.width; // init sum value
			cur_step_max_height = item.height; // init cur_step_max_height
		}
		else
		{
			sum_item_width += item.width;

			if (cur_step_max_height < item.height)
			{
				cur_step_max_height = item.height;
			}
		}
	}

	merged_item->height += cur_step_max_height;
}

//@breif �ϳ��� ���յ� �̹����� ���յ� �̹����� ������ �� ���Ǵ� �̹������� ��ǥ ������ �����ϴ� �Լ�
// ��ǥ ������ ���� ���� �����ϰ� ������ �̹��� ��ǥ�� �����Ѵ�
//@param v_img_item ���տ� ���� �̹��� ����Ʈ
//@param merged_item ���� ��� �̹���
void WriteMergedMarginImage(std::vector<ImageItem> v_img_item, ImageItem *merged_item)
{
	int row_idx = 0;
	int col_idx = 0;
	int prev_max_height = 0;

	std::ofstream merge_info_file;
	std::string merge_file_name = merged_item->path.substr(0, merged_item->path.length() - 4);
	//���յ� �̹����� �����ϴ� �� �̹����� ��ǥ ������ ������ ��� ����
	std::string merge_info_file_name = merge_file_name + "_info" + std::to_string(merge_num) + ".txt";
	merge_info_file.open(merge_info_file_name);

	if (!merge_info_file.is_open())
	{
		Err(" Can't open file ");
		return;
	}

	// ���յ� �̹����� �޸� �Ҵ� �� �ʱ�ȭ
	merged_item->data = (unsigned char*)malloc(sizeof(unsigned char) * merged_item->width * merged_item->height * merged_item->channels);
	memset(merged_item->data, 0, merged_item->width * merged_item->height * merged_item->channels);

	// �̹��� ����Ʈ�κ��� ������ �̹����� ȣ��
	for (auto item : v_img_item)
	{
		std::string item_pos_str;
		std::string item_info_str;

		// ���� �̹����� ������ ��ġ�� ���� �̹��� ( merged_item ) �� �ʺ񺸴� ũ�ٸ�
		// ���� �࿡ �����͸� �����ϱ� ���� ����
		if (col_idx + item.width > merged_item->width)
		{
			// go to next row
			row_idx += prev_max_height;
			//init reference values
			col_idx = 0;
			prev_max_height = 0;

			// ȣ��� �̹��� (item)�� merged_item �� data�� write�ϴ� ����
			for (int item_row_idx = 0; item_row_idx < item.height; item_row_idx++)
			{
				// �̹����� write�� row idx :: prev_max_height ( �������� write�� height idx ) + ���� �̹����� row idx
				int merge_row_idx = row_idx + item_row_idx;

				for (int item_col_idx = 0; item_col_idx < item.width; item_col_idx++)
				{
					// �̹����� write�� column idx :: ���ο� �࿡ ó�� ���̹Ƿ� ���� �̹����� column idx ���� �״�� ������ ��
					int merge_col_idx = col_idx + item_col_idx;

					// item.channels ���̳�  merged_item->channels ���̳� �׻� 4�� ������.
					// �ֳ��ϸ� ������ ����� ���� �̹����� �������ϱ� ����.
					// �̸� ���� item�� ���� �����Ͱ� 3channel�̶�� ���� 100%�� alpha ���� �̸� ä����
					// �����ϱ� ���ϰ� �ϱ� ���� ������Ŵ
					for (int channel_idx = 0; channel_idx < item.channels; channel_idx++)
					{
						merged_item->data[(merge_row_idx * merged_item->width * merged_item->channels) + (merge_col_idx *  item.channels) + channel_idx] =
							item.data[(item_row_idx * item.width * item.channels) + (item_col_idx * item.channels) + channel_idx];
					}
				}
			}

			// ��ǥ ������ ����. �̶�, margin�� �����ϰ� ���� �̹��� ũ�⸦ �ݿ��ϱ� ���� IMG_MARGIN_PIXL ��ŭ SHIFT
			item_pos_str.append(" [ " + std::to_string(col_idx + IMG_MARGIN_PIXEL) + ", " + std::to_string(row_idx + IMG_MARGIN_PIXEL) + ", " 
				+ std::to_string(item.width - IMG_MARGIN_PIXEL * 2) + ", " + std::to_string(item.height - IMG_MARGIN_PIXEL * 2)+ " ]\n");
			item_info_str.append(item.path + item_pos_str);
			
			// item ������ ���Ͽ� ����
			merge_info_file.write(item_info_str.c_str(), item_info_str.length());

			// prev_max_height ���� ���� ����
			if (prev_max_height < item.height)
			{
				prev_max_height = item.height;
			}
			// ���� �̹����� write �� column idx ����
			col_idx += item.width;
		}
		else
		{
			// ȣ��� �̹��� (item)�� merged_item �� data�� write�ϴ� ����, ���� ������ ������� ����
			for (int item_row_idx = 0; item_row_idx < item.height; item_row_idx++)
			{
				int merge_row_idx = row_idx + item_row_idx;

				for (int item_col_idx = 0; item_col_idx < item.width; item_col_idx++)
				{
					int merge_col_idx = col_idx + item_col_idx;

					for (int channel_idx = 0; channel_idx < item.channels; channel_idx++)
					{
						merged_item->data[(merge_row_idx * merged_item->width * merged_item->channels) + (merge_col_idx *  item.channels) + channel_idx] =
						item.data[(item_row_idx * item.width * item.channels) + (item_col_idx * item.channels) + channel_idx];
					}
				}
			}

			item_pos_str.append(" [ " + std::to_string(col_idx + IMG_MARGIN_PIXEL) + ", " + std::to_string(row_idx + IMG_MARGIN_PIXEL) + ", "
				+ std::to_string(item.width - IMG_MARGIN_PIXEL * 2) + ", " + std::to_string(item.height - IMG_MARGIN_PIXEL * 2) + " ]\n");
			item_info_str.append(item.path + item_pos_str);

			merge_info_file.write(item_info_str.c_str(), item_info_str.length());

			if (prev_max_height < item.height)
			{
				prev_max_height = item.height;
			}

			col_idx += item.width;
		}
	}
	//���յ� �̹����� write
	stbi_write_png(merged_item->path.c_str(), merged_item->width, merged_item->height, merged_item->channels, merged_item->data, 0);
}


// brief : ���յ� �̹����� �����ϱ� ���� �Լ�
// v_img_item : ���տ� ���� �̹��� ����Ʈ
// merged_item : ���� ��� �̹���
void CreateMergedImage(std::vector<ImageItem> v_img_item, ImageItem *merged_item)
{
	std::sort(v_img_item.begin(), v_img_item.end(), ItemWidthSort);
	std::sort(v_img_item.begin() + 1, v_img_item.end(), ItemHeightSort);

	CreateMergedImageInfo(v_img_item, merged_item);
	WriteMergedMarginImage(v_img_item, merged_item);

	merge_num++;
}

//@breif �̹����� margin�� �������ִ� �Լ�
//@param item margin ���� ������ �̹���
//@param margin ������ margin ��
//@param margin_item margin ���� ����� �̹��� �����
void CreateMarginImage(ImageItem item, int margin, ImageItem *margin_item)
{
	// item�� ��-�Ͽ� margin ��ŭ ������ �����ϱ� ���� length
	int row_max = item.height + (margin * 2);
	// item�� ��-�쿡 margin ��ŭ ������ �����ϱ� ���� length
	int column_max = item.width + (margin * 2);
	// TODO :: channels ���� �����ϰ� ����� �ʿ䰡 �ִ��� �����غ� ��
	// ������ ������� ä���� 4channel�� ����. �׻� ���� ���� ���Ե� �ؽ�ó�� �ʿ��ϱ� ����
	int margin_channels = 4;

	// ������ �߰��� ������ Ȯ���ϱ� ���� boolean ��
	bool is_insert_row_margin = false;
	// checked 24bit image
	// �������� �̹��� ( ex : RGB ) ���� Ȯ��
	bool is_opaque = (item.channels == 3) ? true : false;

	// ���� ������ �̹��� �޸� �Ҵ�
	if (margin_item->data == nullptr)
	{
		margin_item->data = (unsigned char*)malloc(sizeof(unsigned char) * row_max * column_max * margin_channels);
	}

	// ������ ���Ե� �̹����� �����ϱ� ���� ����
	for (int row_idx = 0; row_idx < row_max; row_idx++)
	{
		// ������ �߰��� �������� ���� �˻�
		if ((row_idx < margin) || (row_idx >= row_max - margin))
		{
			is_insert_row_margin = true;
		}
		else
		{
			is_insert_row_margin = false;
		}

		for (int column_idx = 0; column_idx < column_max; column_idx++)
		{
			// ���� write�� ���� ���� �����̶�� ������ ������ ä��
			if (is_insert_row_margin)
			{
				for (int channel_idx = 0; channel_idx < margin_channels; channel_idx++)
				{
					margin_item->data[(row_idx * column_max * margin_channels) + (column_idx * margin_channels) + channel_idx] = 0;
				}
			}
			// �ƴ϶��
			else
			{
				// ���� write�� ���� ���� �������� �˻��ϰ�, ���� �����̶�� ���� ������ ä��
				if ((column_idx < margin) || (column_idx >= column_max - margin))
				{
					for (int channel_idx = 0; channel_idx < margin_channels; channel_idx++)
					{
						margin_item->data[(row_idx * column_max * margin_channels) + (column_idx * margin_channels) + channel_idx] = 0;
					}
				}
				else
				{
					// �ƴ϶��, ���� ������ write
					for (int channel_idx = 0; channel_idx < margin_channels; channel_idx++)
					{
						int org_row_idx = row_idx - margin;
						int org_column_idx = column_idx - margin;

						if (org_row_idx < 0 || org_column_idx < 0)
						{
							Err("");
						}

						if (is_opaque && channel_idx == 3)
						{
							margin_item->data[(row_idx * column_max * margin_channels) + (column_idx * margin_channels) + channel_idx]
								= 255;
						}
						else
						{
							margin_item->data[(row_idx * column_max * margin_channels) + (column_idx * margin_channels) + channel_idx]
								= item.data[(org_row_idx * item.width * item.channels) + (org_column_idx * item.channels) + channel_idx];
						}
					}
				}
			}
		}
	}

	// ���� �̹����� open �� ��ü ��ο��� ���� �̸��� �ܾ��
	size_t str_first_idx = item.path.rfind(SEPARATE_STR);

	std::string path = item.path.substr(str_first_idx + SEPARATE_STR_LENGTH, item.path.length());

	margin_item->path = path;
	margin_item->width = column_max;
	margin_item->height = row_max;
	margin_item->channels = margin_channels;
}

//@breif ���õ� �̹������� �ϳ��� �ؽ�ó�� �����ϱ� ���� �Լ�
//@param items Menubar���� ������ �ɼǵ��� Ȯ���ϱ� ���� ����
//@param merged_item  ���յ� �̹��� �����
//@param is_direct �̹����� ���ڸ��� ������ ������ ( true ), �̹��� ����Ʈ�� �����Ͽ� ���ϴ� �̹�����θ� ������ ������ ( false ) �����ϴ� ����
void MergedPngImages(UIItems *items, ImageItem *merged_item, bool is_direct)
{
	// direct mode��� ������ �̹��� ������ �ٷ� ���� ó��
	if (is_direct)
	{
		// ������ �̹������� ����Ʈ�κ��� ������ �̹��� ��θ� �����´�
		for (auto img_path : items->v_open_img_path)
		{
			ImageItem input_img_item;
			ImageItem margin_png_item;

			input_img_item.path = img_path;
			// FIXME :: ���� png�� �پ��� Ÿ�� ������ ������ ��
			// stb library�� ���� �̹��� ������ ����
			input_img_item.data = stbi_load(img_path.c_str(), &input_img_item.width, &input_img_item.height, &input_img_item.channels, 0);

			// TODO :: ���� ���� ������ ���� ���� ������ ������ ������ �����غ���
			// IMG_MARGIN_PIXEL ��ŭ ������ ������ ������ �̹��� ����
			CreateMarginImage(input_img_item, IMG_MARGIN_PIXEL, &margin_png_item);
			items->v_img_item.push_back(margin_png_item);
		}
		// ���յ� �̹��� ����� ����
		CreateMergedImage(items->v_img_item, merged_item);
	}
	// image list view�κ��� Ư�� �̹������� �����Ͽ� ���յ� �̹����� ������ ���
	else
	{
		int cur_idx = 0;
		std::vector<ImageItem> v_merge_item;

		for (auto item : items->v_img_item)
		{
			// image list view�κ��� ���õ� �̹����鸸 ��ȸ
			if (g_image_table_visible[cur_idx++])
			{
				ImageItem margin_png_item;

				// TODO :: ���� ���� ������ ���� ���� ������ ������ ������ �����غ���
				// IMG_MARGIN_PIXEL ��ŭ ������ ������ ������ �̹��� ����
				CreateMarginImage(item, IMG_MARGIN_PIXEL, &margin_png_item);

				// ������ �̹��� ����Ʈ�� ���� �̹��� �߰�
				v_merge_item.push_back(margin_png_item);
			}
		}

		// ���յ� �̹��� ����
		CreateMergedImage(v_merge_item, merged_item);
	}
}

//@breif ���� �̹����κ��� raw data�� �����ϴ� �Լ�
//@param items Menubar���� ������ �ɼǵ��� Ȯ���ϱ� ���� ����
//@param item ��ȯ�� �̹��� �����
//@param is_direct �̹����� ���ڸ��� ��ȯ�� ������ ( true ), �̹��� ����Ʈ�� �����Ͽ� ���ϴ� �̹�����θ� ��ȯ�� ������ ( false ) �����ϴ� ����
void RawConvertImages(UIItems *items, ImageItem *item, bool is_direct)
{
	// direct mode��� ������ �̹��� ������ �ٷ� ��ȯ ó��
	if (is_direct)
	{
		ImageItem tmp_item;

		std::ofstream out_file;
		std::string org_path = g_menu_items.open_raw_path;
		size_t str_first_idx = org_path.rfind(SEPARATE_STR);

		std::string file_name = org_path.substr(str_first_idx + SEPARATE_STR_LENGTH, org_path.length());
		file_name = file_name.substr(0, file_name.length() - 4);
		file_name.append(".raw");

		out_file.open(file_name.c_str(), std::ios::out | std::ios::binary);

		tmp_item.path = g_menu_items.open_raw_path;
		// stb library�� �̹����� raw ������ ����
		tmp_item.data = stbi_load(tmp_item.path.c_str(), &tmp_item.width, &tmp_item.height, &tmp_item.channels, 0);

		out_file.write((char*)tmp_item.data, tmp_item.width * tmp_item.height * tmp_item.channels);
		
		// stbi_load ��� �� �Ϻ� png ������ ����� ���� ���ϴ� ��찡 ����. ���� ������ �ѹ� ������ ���ֱ� ����
		// CreateMarginImage�� ȣ���Ͽ� ó������.
		CreateMarginImage(tmp_item, 0, item);

		out_file.close();
	}
	// image list view�κ��� Ư�� �̹������� �����Ͽ� ��ȯ�� �̹����� ������ ���
	else
	{
		int cur_idx = 0;
		std::vector<ImageItem> v_merge_item;

		for (int i = 0; i < items->v_open_img_path.size(); i++)
		{
			// image list view�κ��� ���õ� �̹����鸸 ��ȸ
			if (g_image_table_visible[cur_idx])
			{
				ImageItem tmp_item;
				ImageItem tmp_raw_item;

				std::ofstream out_file;
				std::string item_path = items->v_open_img_path[cur_idx];
				size_t str_first_idx = item_path.rfind(SEPARATE_STR);

				std::string out_path = item_path.substr(str_first_idx + SEPARATE_STR_LENGTH, item_path.length());
				out_path = out_path.substr(0, out_path.length() -4);
				out_path.append(".raw");

				out_file.open(out_path.c_str(), std::ios::out | std::ios::binary);

				tmp_item.path = g_menu_items.open_raw_path;
				// stb library�� �̹����� raw ������ ����
				tmp_item.data = stbi_load(item_path.c_str(), &tmp_item.width, &tmp_item.height, &tmp_item.channels, 0);

				out_file.write((char*)tmp_item.data, tmp_item.width * tmp_item.height * tmp_item.channels);

				CreateMarginImage(tmp_item, IMG_MARGIN_PIXEL, &tmp_raw_item);
				v_merge_item.push_back(tmp_raw_item);

				out_file.close();
			}
			cur_idx++;
		}
		// raw�� ������ �̹��� ����, ��ȯ�� �̹����� ��������� �ϳ��� �ؽ�ó�� ��ģ��
		CreateMergedImage(v_merge_item, item);

		if (v_merge_item.size() > 1)
		{
			std::ofstream out_file;
			
			std::string out_path = item->path;
			out_path = out_path.substr(0, out_path.length() - 4);
			out_path.append(".raw");

			out_file.open(out_path.c_str(), std::ios::out | std::ios::binary);

			out_file.write((char*)item->data, item->width * item->height * item->channels);

			out_file.close();
		}
	}

	g_menu_items.active_list_raw_convert = false;
}

//@brief yvu420p image data�� rgb data�� ��ȯ�ϴ� �Լ�
//@param src_item ��ȯ�ϱ� ���� yvu420 data ������ ���� item
//@param dst_item ��ȯ�� �����Ͱ� ����� item
void YVU420PToRGB(ImageItem src_item, ImageItem dst_item)
{

}

//@brief yvu422p image data�� rgb data�� ��ȯ�ϴ� �Լ�
//@param src_item ��ȯ�ϱ� ���� yvu422 data ������ ���� item
//@param dst_item ��ȯ�� �����Ͱ� ����� item
void YVU422PToRGB(ImageItem src_item, ImageItem dst_item)
{

}

//@brief yuv420p image data�� rgb data�� ��ȯ�ϴ� �Լ�
//@param src_item ��ȯ�ϱ� ���� yuv420 data ������ ���� item
//@param dst_item ��ȯ�� �����Ͱ� ����� item
void YUV420PToRGB(ImageItem src_item, ImageItem dst_item)
{

}

//@brief yuv422p image data�� rgb data�� ��ȯ�ϴ� �Լ�
//@param src_item ��ȯ�ϱ� ���� yuv422 data ������ ���� item
//@param dst_item ��ȯ�� �����Ͱ� ����� item
void YUV422PToRGB(ImageItem src_item, ImageItem dst_item)
{

}

//@brief raw image data�� rgb data�� ��ȯ�ϴ� �Լ�
//@param src_item ��ȯ�ϱ� ���� raw data ������ ���� item
//@param dst_item ��ȯ�� �����Ͱ� ����� item
void RawToRGB(ImageItem src_item, ImageItem dst_item)
{
	switch (src_item.img_format)
	{
	case kYUV420P:
		YUV420PToRGB(src_item, dst_item);
		break;
	case kYUV422P:
		YUV422PToRGB(src_item, dst_item);
		break;
	case kYVU420P:
		YVU420PToRGB(src_item, dst_item);
		break;
	case kYVU422P:
		YVU422PToRGB(src_item, dst_item);
		break;
	default:
		break;
	}
}
//@brief MenuBar�� ���� ������ �̹������ϵ�κ��� �̹��� ���� �����͸� �����ϴ� �Լ�
//@param items Menubar���� ������ �ɼǵ��� Ȯ���ϱ� ���� ����
void CreateImageItems(UIItems *items, bool is_raw)
{
	int image_idx = 0;

	for (auto img_path : items->v_open_img_path)
	{
		if (items->v_img_item.size() > image_idx++)
		{
			continue;
		}

		ImageItem input_img_item;
		ImageItem margin_png_item;

		input_img_item.path = img_path;

		if (is_raw)
		{

		}
		else
		{
			input_img_item.data = stbi_load(img_path.c_str(), &input_img_item.width, &input_img_item.height, &input_img_item.channels, 0);

			// stbi_load ��� �� �Ϻ� png ������ ����� ���� ���ϴ� ��찡 ����. ���� ������ �ѹ� ������ ���ֱ� ����
			// CreateMarginImage�� ȣ���Ͽ� ó������.
			CreateMarginImage(input_img_item, 0, &margin_png_item);

			margin_png_item.img_format = input_img_item.channels == 3 ? ImageFormat::kRGB : ImageFormat::kRGBA;
		}

		items->v_img_item.push_back(margin_png_item);
	}
}

int main(int, char**)
{
#ifdef _WIN32
	SetConsoleCP(CP_UTF8);
#endif // #ifdef _WIN32
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char* glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(UI_WINDOW_SIZE_W, UI_WINDOW_SIZE_H, "Image Editor", NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	io.Fonts->AddFontDefault();

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	static ImageItem view_item;
	
	InitUIMember();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// menu bar�� �׸��� ���� �Լ� ȣ��
		DrawMenuBar(&g_menu_items);

		// menu bar�κ��� Ȱ��ȭ�� action ó��
		// �̹��� ���ϵ��� ���ڸ��� �����ϴ� ����
		if (g_menu_items.active_direct_merge)
		{
			MergedPngImages(&g_menu_items, &view_item, true);
			g_menu_items.active_direct_merge = false;
			g_menu_items.is_make_merge = true;
			g_menu_items.active_preview_image = true;
		}
		// �̹��� ������ open�Ͽ� image list view�� �ݿ�
		else if (g_menu_items.is_open_files)
		{
			CreateImageItems(&g_menu_items, g_menu_items.is_open_raw_file);

			g_menu_items.is_open_files = false;
		}
		// image list view���� Ư�� �̹������� visible�ϰ� merge�� ��� ó���ϴ� ����
		else if (g_menu_items.active_list_merge)
		{
			MergedPngImages(&g_menu_items, &view_item, false);
			g_menu_items.active_list_merge = false;
			g_menu_items.is_make_merge = true;
			g_menu_items.active_preview_image = true;
		}
		// �̹��� ���ϵ��� ���ڸ��� raw ��ȯ�ϴ� ����
		else if (g_menu_items.active_raw_convert)
		{
			RawConvertImages(&g_menu_items, &view_item, true);
			g_menu_items.active_raw_convert = false;
			g_menu_items.is_make_raw = true;
			g_menu_items.active_preview_image = true;
		}
		// image list view���� Ư�� �̹������� visible�ϰ�  raw ��ȯ�� ��� ó���ϴ� ����
		else if (g_menu_items.active_list_raw_convert)
		{
			RawConvertImages(&g_menu_items, &view_item, false);
			g_menu_items.active_list_raw_convert = false;
			g_menu_items.is_make_raw = true;
			g_menu_items.active_preview_image = true;
		}

		// image / video�� ���� preview�� �׸��� �Լ�
		DrawPreView(&g_menu_items, &view_item);
		// image list view�� �׸��� �Լ�
		DrawImageListView(&g_menu_items);
		// Settings view�� �׸��� �Լ�
		DrawPropertyView(&g_menu_items);
		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
