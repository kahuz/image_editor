# image_editor
> OpenGL 또는 image processing에 필요한 이미지 편집 툴
OpenGL 프로그램 개발에 필요한 텍스처 병합 혹은 image processing과 관련되 이미지 변환과 같은 기능을 제공하는 툴

## 미리보기
<iframe width="2200" height="1247" src="https://www.youtube.com/embed/fy84PfPDSso" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

> 사진 클릭 시 Youtube 데모 영상 페이지로 넘어갑니다

## Menu bar
### Files
#### Open - Direct Merge 
File dialog에서 선택된 image list들을 바로 병합하는 기능

#### Open - Direct Raw Convert
File dialog에서 선택된 image list들을 바로 raw 변환하는 기능

#### Open Images
편집할 이미지들을 File dialog로 불러오는 기능
png, jpg, bmp 와 같은 이미지 파일과 raw 확장자를 가진 이미지 로우 데이터를 불러올 수 있다
raw 확장자를 선택한 경우 이미지 크기 ( width & height ), 이미지 포맷 ( yuv, rgb ), color conversion 정보를 입력하여 이미지를 불러올 수 있다


### Image List Action
#### Merged Select Images
Image List View 에서 Visible 체크 박스가 활성화된 이미지들을 병합하는 기능

#### Raw Converted SElect Images
Image List View 에서 Visible 체크 박스가 활성화된 이미지들을 Raw 파일로 변환하는 기능  

### Image View
File dialog로 불러온 이미지나 Image List View에서 선택된 이미지, 병합 처리한 이미지 등의 미리보기를 지원

### Image Property View
이미지 속성 정보를 보여주는 뷰
이름, 크기, 포맷 정보를 보여준다

### Image List View
File dialog로 불러온 이미지에 대한 작은 크기의 미리보기와 경로, 미리보기 여부 등을 선택할 수 있게 해주는 뷰


## 추가 예정
 - Video 파일에 대한 리소스 처리 지원에 대해 고민 중
 - 이미지 리소스의 변환 ( scale, rotate, format convert, etc...) 기능 지원에 대해 고민 중
