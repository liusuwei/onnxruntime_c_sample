#include "image_file_libpng.h"
#include <png.h>

// The data format is nchw
void normalize(float** data, int data_length) {
  //             {    R,     G,     B}
  float mean[] = {0.485, 0.456, 0.406};
  float stddev[] = {0.229, 0.224, 0.225};
  float* temp = *data;

  int j = 0;
  for (int i = 0; i < data_length; ++i) {
    if (i == data_length / 3 * (j + 1)) {j++;}
    *(*data + i) = (*(*data + i) / 255.0 - mean[2 - j]) / stddev[2 - j];
    //*(*data + (2 - j) * data_length / 3 + i) = (temp[i] / 255.0 - mean[2 - j]) / stddev[2 - j];
  }
}

const char* convert_wc_to_c(const wchar_t* wc) {
  size_t i;
  char *converted_wc = (char*)malloc(sizeof(char*)*100);

  wcstombs_s(&i, converted_wc, (size_t)100, wc, (size_t)100 - 1);
  printf( "Convert wide-character string: %s\n", converted_wc);
  return converted_wc;

  //unsafe version
  //size_t wcsize = wcslen(wc);
  //char converted_wc[100];
  //wcstombs(converted_wc, wc, wcsize + 1);
  //printf("Convert wide-character string: %s\n", converted_wc);
  //return converted_wc;
}

int read_image_file(const ORTCHAR_T* input_file, size_t* height, size_t* width, float** out, size_t* output_count) {
  png_image image; /* The control structure used by libpng */
  /* Initialize the 'png_image' structure. */
  memset(&image, 0, (sizeof image));
  image.version = PNG_IMAGE_VERSION;
  if (png_image_begin_read_from_file(&image, convert_wc_to_c(input_file)) == 0) {
    return -1;
  }
  uint8_t* buffer;
  image.format = PNG_FORMAT_BGR;
  size_t input_data_length = PNG_IMAGE_SIZE(image);
  if (input_data_length != 224 * 224 * 3) {
    printf("input_data_length:%zd\n", input_data_length);
    return -1;
  }
  buffer = (uint8_t*)malloc(input_data_length);
  memset(buffer, 0, input_data_length);
  if (png_image_finish_read(&image, NULL /*background*/, buffer, 0 /*row_stride*/, NULL /*colormap*/) == 0) {
    return -1;
  }
  hwc_to_chw(buffer, image.height, image.width, out, output_count);
  normalize(out, input_data_length);
  free(buffer);
  *width = image.width;
  *height = image.height;
  return 0;
}


int write_image_file(uint8_t* model_output_bytes, unsigned int height,
                     unsigned int width, const ORTCHAR_T* output_file){
  png_image image;
  memset(&image, 0, (sizeof image));
  image.version = PNG_IMAGE_VERSION;
  image.format = PNG_FORMAT_BGR;
  image.height = height;
  image.width = width;
  int ret = 0;
  if (png_image_write_to_file(&image, convert_wc_to_c(output_file), 0 /*convert_to_8bit*/, model_output_bytes, 0 /*row_stride*/,
			      NULL /*colormap*/) == 0) {
    printf("write to '%s' failed:%s\n", convert_wc_to_c(output_file), image.message);
    ret = -1;
  }
  return ret;
}
