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

// menubar 옵션 관리를 위한 전역 변수
static UIItems g_menu_items;
// FIXME :: 추후 설정을 통해 merge_num을 대체하는 str을 사용할 것
// 병합된 이미지와 이미지 정보 파일을 위한 idx 값
static int merge_num = 1;
// 이미지 병합 시 사용되는 마진 값. gl mipmap 옵션을 위해 이미지 병합 시 상하좌우 IMG_MARGIN_PIXEL 만큼 마진 값을 생성 ( alpha 100% )
#define IMG_MARGIN_PIXEL 8

typedef uint32_t fourcc_t;

#define fourcc_(a,b,c,d) (\
			(((fourcc_t)(d)) << 24u) | (((fourcc_t)(c)) << 16u) | \
			(((fourcc_t)(b)) << 8u) | (fourcc_t)(a) \
		)

//@breif glfw error 콜백 함수
//@param error error code
//@param description error description
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

//@breif 이미지의 width 기준 내림차순 정렬 함수
//@param a 정렬 비교 대상 a
//@param b 정렬 비교 대상 b
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

//@breif 이미지의 height 기준 내림차순 정렬 함수
//@param a 정렬 비교 대상 a
//@param b 정렬 비교 대상 b
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
//@breif 픽셀 좌표를 UV 좌표로 변환하는 함수
//@param tex_size 픽셀 기준이 될 텍스처 크기
//@param pos 변환할 좌표 위치
//@param size  UV 기준이 될 텍스처 크기
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

//@breif 병합될 이미지를 위한 기본 정보값을 초기화하는 함수
//@details  병합할 이미지 리스트 ( v_img_item ) 정보를 읽어들여 생성할 텍스처에 대한 width, height, channel 값과
// 병합될 이미지의 경로를 설정한다.
// 병합될 이미지의 너비는 이미지 리스트 중 너비가 가장 큰 값으로, 높이는 정렬된 이미지들의 총 합으로 결정된다
//@param v_img_item 병합에 사용될 이미지 리스트
//@param merged_item 병합 결과 이미지
void CreateMergedImageInfo(std::vector<ImageItem> v_img_item, ImageItem * merged_item)
{
	//병합된 이미지를 저장할 경로 설정
	merged_item->path = "merged_image_" + std::to_string(merge_num) + ".png";
	//sort된 값이기에 항상 0번째 인덱스의 width가 최대값
	merged_item->width = v_img_item[0].width;
	merged_item->height = 0;
	//내가 생성할 건 항상 알파값 포함된 이미지이므로 4channel로 설정
	merged_item->channels = 4;
	int sum_item_width = 0;
	int cur_step_max_height = 0;

	// 이미지 리스트로부터 각각의 이미지를 호출
	for (auto item : v_img_item)
	{
		// 이미지 리스트 중 너비가 가장 큰 값을 구하기 위한 변수
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

//@breif 하나로 병합된 이미지와 병합된 이미지를 생성할 때 사용되는 이미지들의 좌표 정보를 저장하는 함수
// 좌표 정보는 마진 값을 무시하고 순수한 이미지 좌표를 저장한다
//@param v_img_item 병합에 사용될 이미지 리스트
//@param merged_item 병합 결과 이미지
void WriteMergedMarginImage(std::vector<ImageItem> v_img_item, ImageItem *merged_item)
{
	int row_idx = 0;
	int col_idx = 0;
	int prev_max_height = 0;

	std::ofstream merge_info_file;
	std::string merge_file_name = merged_item->path.substr(0, merged_item->path.length() - 4);
	//병합된 이미지를 구성하는 각 이미지의 좌표 정보를 저장할 경로 설정
	std::string merge_info_file_name = merge_file_name + "_info" + std::to_string(merge_num) + ".txt";
	merge_info_file.open(merge_info_file_name);

	if (!merge_info_file.is_open())
	{
		Err(" Can't open file ");
		return;
	}

	// 병합될 이미지의 메모리 할당 및 초기화
	merged_item->data = (unsigned char*)malloc(sizeof(unsigned char) * merged_item->width * merged_item->height * merged_item->channels);
	memset(merged_item->data, 0, merged_item->width * merged_item->height * merged_item->channels);

	// 이미지 리스트로부터 각각의 이미지를 호출
	for (auto item : v_img_item)
	{
		std::string item_pos_str;
		std::string item_info_str;

		// 현재 이미지를 저장할 위치가 병합 이미지 ( merged_item ) 의 너비보다 크다면
		// 다음 행에 데이터를 저장하기 위한 구문
		if (col_idx + item.width > merged_item->width)
		{
			// go to next row
			row_idx += prev_max_height;
			//init reference values
			col_idx = 0;
			prev_max_height = 0;

			// 호출된 이미지 (item)을 merged_item 의 data에 write하는 구문
			for (int item_row_idx = 0; item_row_idx < item.height; item_row_idx++)
			{
				// 이미지가 write될 row idx :: prev_max_height ( 이전까지 write된 height idx ) + 현재 이미지의 row idx
				int merge_row_idx = row_idx + item_row_idx;

				for (int item_col_idx = 0; item_col_idx < item.width; item_col_idx++)
				{
					// 이미지가 write될 column idx :: 새로운 행에 처음 쓰이므로 현재 이미지의 column idx 값을 그대로 가지게 됨
					int merge_col_idx = col_idx + item_col_idx;

					// item.channels 값이나  merged_item->channels 값이나 항상 4로 유지됨.
					// 왜냐하면 투명한 배경을 가진 이미지로 만들어야하기 때문.
					// 이를 위해 item의 원본 데이터가 3channel이라면 투명도 100%의 alpha 값을 미리 채워둠
					// 구현하기 편하게 하기 위해 고정시킴
					for (int channel_idx = 0; channel_idx < item.channels; channel_idx++)
					{
						merged_item->data[(merge_row_idx * merged_item->width * merged_item->channels) + (merge_col_idx *  item.channels) + channel_idx] =
							item.data[(item_row_idx * item.width * item.channels) + (item_col_idx * item.channels) + channel_idx];
					}
				}
			}

			// 좌표 정보를 설정. 이때, margin을 무시하고 원본 이미지 크기를 반영하기 위해 IMG_MARGIN_PIXL 만큼 SHIFT
			item_pos_str.append(" [ " + std::to_string(col_idx + IMG_MARGIN_PIXEL) + ", " + std::to_string(row_idx + IMG_MARGIN_PIXEL) + ", " 
				+ std::to_string(item.width - IMG_MARGIN_PIXEL * 2) + ", " + std::to_string(item.height - IMG_MARGIN_PIXEL * 2)+ " ]\n");
			item_info_str.append(item.path + item_pos_str);
			
			// item 정보를 파일에 저장
			merge_info_file.write(item_info_str.c_str(), item_info_str.length());

			// prev_max_height 값을 새로 설정
			if (prev_max_height < item.height)
			{
				prev_max_height = item.height;
			}
			// 다음 이미지가 write 될 column idx 변경
			col_idx += item.width;
		}
		else
		{
			// 호출된 이미지 (item)을 merged_item 의 data에 write하는 구문, 위와 동일한 방법으로 동작
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
	//병합된 이미지를 write
	stbi_write_png(merged_item->path.c_str(), merged_item->width, merged_item->height, merged_item->channels, merged_item->data, 0);
}


// brief : 병합된 이미지를 생성하기 위한 함수
// v_img_item : 병합에 사용될 이미지 리스트
// merged_item : 병합 결과 이미지
void CreateMergedImage(std::vector<ImageItem> v_img_item, ImageItem *merged_item)
{
	std::sort(v_img_item.begin(), v_img_item.end(), ItemWidthSort);
	std::sort(v_img_item.begin() + 1, v_img_item.end(), ItemHeightSort);

	CreateMergedImageInfo(v_img_item, merged_item);
	WriteMergedMarginImage(v_img_item, merged_item);

	merge_num++;
}

//@breif 이미지에 margin을 생성해주는 함수
//@param item margin 값을 적용할 이미지
//@param margin 적용할 margin 값
//@param margin_item margin 값이 적용된 이미지 결과물
void CreateMarginImage(ImageItem item, int margin, ImageItem *margin_item)
{
	// item의 상-하에 margin 만큼 여백을 생성하기 위한 length
	int row_max = item.height + (margin * 2);
	// item의 좌-우에 margin 만큼 여백을 생성하기 위한 length
	int column_max = item.width + (margin * 2);
	// TODO :: channels 값이 유연하게 변경될 필요가 있는지 생각해볼 것
	// 생성될 결과물의 채널을 4channel로 고정. 항상 알파 값이 포함된 텍스처가 필요하기 때문
	int margin_channels = 4;

	// 여백을 추가할 행인지 확인하기 위한 boolean 값
	bool is_insert_row_margin = false;
	// checked 24bit image
	// 불투명한 이미지 ( ex : RGB ) 인지 확인
	bool is_opaque = (item.channels == 3) ? true : false;

	// 새로 생성할 이미지 메모리 할당
	if (margin_item->data == nullptr)
	{
		margin_item->data = (unsigned char*)malloc(sizeof(unsigned char) * row_max * column_max * margin_channels);
	}

	// 여백이 포함된 이미지를 생성하기 위한 구문
	for (int row_idx = 0; row_idx < row_max; row_idx++)
	{
		// 여백이 추가될 행인지에 대해 검사
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
			// 현재 write할 행이 여백 영역이라면 투명한 값으로 채움
			if (is_insert_row_margin)
			{
				for (int channel_idx = 0; channel_idx < margin_channels; channel_idx++)
				{
					margin_item->data[(row_idx * column_max * margin_channels) + (column_idx * margin_channels) + channel_idx] = 0;
				}
			}
			// 아니라면
			else
			{
				// 현재 write할 열이 여백 영역인지 검사하고, 여백 영역이라면 투명 값으로 채움
				if ((column_idx < margin) || (column_idx >= column_max - margin))
				{
					for (int channel_idx = 0; channel_idx < margin_channels; channel_idx++)
					{
						margin_item->data[(row_idx * column_max * margin_channels) + (column_idx * margin_channels) + channel_idx] = 0;
					}
				}
				else
				{
					// 아니라면, 원본 데이터 write
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

	// 현재 이미지를 open 한 정체 경로에서 파일 이름만 긁어옴
	size_t str_first_idx = item.path.rfind(SEPARATE_STR);

	std::string path = item.path.substr(str_first_idx + SEPARATE_STR_LENGTH, item.path.length());

	margin_item->path = path;
	margin_item->width = column_max;
	margin_item->height = row_max;
	margin_item->channels = margin_channels;
}

//@breif 선택된 이미지들을 하나의 텍스처로 병합하기 위한 함수
//@param items Menubar에서 선택한 옵션들을 확인하기 위한 변수
//@param merged_item  병합된 이미지 결과물
//@param is_direct 이미지를 열자마자 병합할 것인지 ( true ), 이미지 리스트를 생성하여 원하는 이미지들로만 병합할 것인지 ( false ) 결정하는 변수
void MergedPngImages(UIItems *items, ImageItem *merged_item, bool is_direct)
{
	// direct mode라면 열기한 이미지 파일을 바로 병합 처리
	if (is_direct)
	{
		// 열기한 이미지들의 리스트로부터 각각의 이미지 경로를 가져온다
		for (auto img_path : items->v_open_img_path)
		{
			ImageItem input_img_item;
			ImageItem margin_png_item;

			input_img_item.path = img_path;
			// FIXME :: 추후 png외 다양한 타입 지원시 수정할 것
			// stb library를 통해 이미지 데이터 추출
			input_img_item.data = stbi_load(img_path.c_str(), &input_img_item.width, &input_img_item.height, &input_img_item.channels, 0);

			// TODO :: 추후 여백 정보에 대해 설정 탭으로 조정할 것인지 생각해보자
			// IMG_MARGIN_PIXEL 만큼 투명한 여백을 가지는 이미지 생성
			CreateMarginImage(input_img_item, IMG_MARGIN_PIXEL, &margin_png_item);
			items->v_img_item.push_back(margin_png_item);
		}
		// 병합된 이미지 결과물 생성
		CreateMergedImage(items->v_img_item, merged_item);
	}
	// image list view로부터 특정 이미지들을 선택하여 병합된 이미지를 생성할 경우
	else
	{
		int cur_idx = 0;
		std::vector<ImageItem> v_merge_item;

		for (auto item : items->v_img_item)
		{
			// image list view로부터 선택된 이미지들만 조회
			if (g_image_table_visible[cur_idx++])
			{
				ImageItem margin_png_item;

				// TODO :: 추후 여백 정보에 대해 설정 탭으로 조정할 것인지 생각해보자
				// IMG_MARGIN_PIXEL 만큼 투명한 여백을 가지는 이미지 생성
				CreateMarginImage(item, IMG_MARGIN_PIXEL, &margin_png_item);

				// 병합할 이미지 리스트에 여백 이미지 추가
				v_merge_item.push_back(margin_png_item);
			}
		}

		// 병합된 이미지 생성
		CreateMergedImage(v_merge_item, merged_item);
	}
}

//@breif 원본 이미지로부터 raw data만 추출하는 함수
//@param items Menubar에서 선택한 옵션들을 확인하기 위한 변수
//@param item 변환된 이미지 결과물
//@param is_direct 이미지를 열자마자 변환할 것인지 ( true ), 이미지 리스트를 생성하여 원하는 이미지들로만 변환할 것인지 ( false ) 결정하는 변수
void RawConvertImages(UIItems *items, ImageItem *item, bool is_direct)
{
	// direct mode라면 열기한 이미지 파일을 바로 변환 처리
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
		// stb library로 이미지의 raw 데이터 추출
		tmp_item.data = stbi_load(tmp_item.path.c_str(), &tmp_item.width, &tmp_item.height, &tmp_item.channels, 0);

		out_file.write((char*)tmp_item.data, tmp_item.width * tmp_item.height * tmp_item.channels);
		
		// stbi_load 사용 시 일부 png 파일을 제대로 읽지 못하는 경우가 있음. 따라서 포맷을 한번 재정렬 해주기 위해
		// CreateMarginImage를 호출하여 처리해줌.
		CreateMarginImage(tmp_item, 0, item);

		out_file.close();
	}
	// image list view로부터 특정 이미지들을 선택하여 변환된 이미지를 생성할 경우
	else
	{
		int cur_idx = 0;
		std::vector<ImageItem> v_merge_item;

		for (int i = 0; i < items->v_open_img_path.size(); i++)
		{
			// image list view로부터 선택된 이미지들만 조회
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
				// stb library로 이미지의 raw 데이터 추출
				tmp_item.data = stbi_load(item_path.c_str(), &tmp_item.width, &tmp_item.height, &tmp_item.channels, 0);

				out_file.write((char*)tmp_item.data, tmp_item.width * tmp_item.height * tmp_item.channels);

				CreateMarginImage(tmp_item, IMG_MARGIN_PIXEL, &tmp_raw_item);
				v_merge_item.push_back(tmp_raw_item);

				out_file.close();
			}
			cur_idx++;
		}
		// raw로 구성된 이미지 생성, 변환할 이미지가 여러개라면 하나의 텍스처로 합친다
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

//@brief yvu420p image data를 rgb data로 변환하는 함수
//@param src_item 변환하기 위한 yvu420 data 정보를 담은 item
//@param dst_item 변환된 데이터가 저장될 item
void YVU420PToRGB(ImageItem src_item, ImageItem dst_item)
{

}

//@brief yvu422p image data를 rgb data로 변환하는 함수
//@param src_item 변환하기 위한 yvu422 data 정보를 담은 item
//@param dst_item 변환된 데이터가 저장될 item
void YVU422PToRGB(ImageItem src_item, ImageItem dst_item)
{

}

//@brief yuv420p image data를 rgb data로 변환하는 함수
//@param src_item 변환하기 위한 yuv420 data 정보를 담은 item
//@param dst_item 변환된 데이터가 저장될 item
void YUV420PToRGB(ImageItem src_item, ImageItem dst_item)
{

}

//@brief yuv422p image data를 rgb data로 변환하는 함수
//@param src_item 변환하기 위한 yuv422 data 정보를 담은 item
//@param dst_item 변환된 데이터가 저장될 item
void YUV422PToRGB(ImageItem src_item, ImageItem dst_item)
{

}

//@brief raw image data를 rgb data로 변환하는 함수
//@param src_item 변환하기 위한 raw data 정보를 담은 item
//@param dst_item 변환된 데이터가 저장될 item
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
//@brief MenuBar를 통해 열기한 이미지파일들로부터 이미지 원본 데이터를 생성하는 함수
//@param items Menubar에서 선택한 옵션들을 확인하기 위한 변수
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

			// stbi_load 사용 시 일부 png 파일을 제대로 읽지 못하는 경우가 있음. 따라서 포맷을 한번 재정렬 해주기 위해
			// CreateMarginImage를 호출하여 처리해줌.
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

		// menu bar를 그리기 위한 함수 호출
		DrawMenuBar(&g_menu_items);

		// menu bar로부터 활성화된 action 처리
		// 이미지 파일들을 열자마자 병합하는 구문
		if (g_menu_items.active_direct_merge)
		{
			MergedPngImages(&g_menu_items, &view_item, true);
			g_menu_items.active_direct_merge = false;
			g_menu_items.is_make_merge = true;
			g_menu_items.active_preview_image = true;
		}
		// 이미지 파일을 open하여 image list view에 반영
		else if (g_menu_items.is_open_files)
		{
			CreateImageItems(&g_menu_items, g_menu_items.is_open_raw_file);

			g_menu_items.is_open_files = false;
		}
		// image list view에서 특정 이미지들을 visible하고 merge할 경우 처리하는 구문
		else if (g_menu_items.active_list_merge)
		{
			MergedPngImages(&g_menu_items, &view_item, false);
			g_menu_items.active_list_merge = false;
			g_menu_items.is_make_merge = true;
			g_menu_items.active_preview_image = true;
		}
		// 이미지 파일들을 열자마자 raw 변환하는 구문
		else if (g_menu_items.active_raw_convert)
		{
			RawConvertImages(&g_menu_items, &view_item, true);
			g_menu_items.active_raw_convert = false;
			g_menu_items.is_make_raw = true;
			g_menu_items.active_preview_image = true;
		}
		// image list view에서 특정 이미지들을 visible하고  raw 변환할 경우 처리하는 구문
		else if (g_menu_items.active_list_raw_convert)
		{
			RawConvertImages(&g_menu_items, &view_item, false);
			g_menu_items.active_list_raw_convert = false;
			g_menu_items.is_make_raw = true;
			g_menu_items.active_preview_image = true;
		}

		// image / video에 대한 preview를 그리는 함수
		DrawPreView(&g_menu_items, &view_item);
		// image list view를 그리는 함수
		DrawImageListView(&g_menu_items);
		// Settings view를 그리는 함수
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
