#include <stdio.h>
#include "onnxruntime_c_api.h"
#include "image_file_libpng.h"

#define ORT_ABORT_ON_ERROR(expr)                             \
  do {                                                       \
    OrtStatus* onnx_status = (expr);                         \
    if (onnx_status != NULL) {                               \
      const char* msg = ort->GetErrorMessage(onnx_status); \
      fprintf(stderr, "%s\n", msg);                          \
      ort->ReleaseStatus(onnx_status);                     \
      abort();                                               \
    }                                                        \
  } while (0);

/**
 * convert input from HWC format to CHW format
 * \param input A single image. The byte array has length of 3*h*w
 * \param h image height
 * \param w image width
 * \param output A float array. should be freed by caller after use
 * \param output_count Array length of the `output` param
 */
void hwc_to_chw(const uint8_t* input, size_t h, size_t w, float** output, size_t* output_count) {
  size_t stride = h * w;
  *output_count = stride * 3;
  float* output_data = (float*)malloc(*output_count * sizeof(float));
  for (size_t i = 0; i != stride; ++i) {
    for (size_t c = 0; c != 3; ++c) {
      output_data[c * stride + i] = input[i * 3 + c];
    }
  }
  *output = output_data;
}

/**
 * convert input from CHW format to HWC format
 * \param input A single image. This float array has length of 3*h*w
 * \param h image height
 * \param w image width
 * \param output A byte array. should be freed by caller after use
 */
static void chw_to_hwc(const float* input, size_t h, size_t w, uint8_t** output) {
  size_t stride = h * w;
  uint8_t* output_data = (uint8_t*)malloc(stride * 3);
  for (size_t c = 0; c != 3; ++c) {
    size_t t = c * stride;
    for (size_t i = 0; i != stride; ++i) {
      float f = input[t + i];
      if (f < 0.f || f > 255.0f) f = 0;
      output_data[i * 3 + c] = (uint8_t)f;
    }
  }
  *output = output_data;
}

int run_inference(OrtApi* ort, OrtSession* session, const ORTCHAR_T* input_file, const ORTCHAR_T* output_file) {
  size_t input_height;
  size_t input_width;
  float* model_input;
  size_t model_input_ele_count;

  if (read_image_file(input_file, &input_height, &input_width, &model_input, &model_input_ele_count) != 0) {
    printf("read image file fail\n");
    return -1;
  }
  if (input_height != 720 || input_width != 720) {
    printf("please resize to image to 720x720\n");
    free(model_input);
    return -1;
  }
  OrtMemoryInfo* memory_info;
  ORT_ABORT_ON_ERROR(ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info));
  const int64_t input_shape[] = {1, 3, 720, 720};
  const size_t input_shape_len = sizeof(input_shape) / sizeof(input_shape[0]);
  const size_t model_input_len = model_input_ele_count * sizeof(float);

  OrtValue* input_tensor = NULL;
  ORT_ABORT_ON_ERROR(ort->CreateTensorWithDataAsOrtValue(memory_info, model_input, model_input_len, input_shape,
                                                           input_shape_len, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
                                                           &input_tensor));
  int is_tensor;
  ORT_ABORT_ON_ERROR(ort->IsTensor(input_tensor, &is_tensor));
  ort->ReleaseMemoryInfo(memory_info);
  const char* input_names[] = {"inputImage"};
  const char* output_names[] = {"outputImage"};
  OrtValue* output_tensor = NULL;
  ORT_ABORT_ON_ERROR(ort->Run(session, NULL, input_names, (const OrtValue* const*)&input_tensor, 1, output_names, 1,
                                &output_tensor));
  ORT_ABORT_ON_ERROR(ort->IsTensor(output_tensor, &is_tensor));
  int ret = 0;
  float* output_tensor_data = NULL;
  ORT_ABORT_ON_ERROR(ort->GetTensorMutableData(output_tensor, (void**)&output_tensor_data));
  uint8_t* output_image_data = NULL;
  chw_to_hwc(output_tensor_data, 720, 720, &output_image_data);
  if (write_image_file(output_image_data, 720, 720, output_file) != 0) {
    printf("write image file fail\n");
    ret = -1;
  }
  ort->ReleaseValue(output_tensor);
  ort->ReleaseValue(input_tensor);
  free(model_input);
  return ret;
}

static void usage() { printf("usage: <model_path> <input_file> <output_file>\n"); }

const wchar_t* convert_c_to_wc(const char* c) {
  size_t i;
  wchar_t *converted_c = (wchar_t*)malloc(sizeof(wchar_t*)*100);

  mbstowcs_s(&i, converted_c, (size_t)100, c, (size_t)100 - 1);
  printf( "Convert multibyte string: %ls\n", converted_c);
  return converted_c;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    usage();
    return -1;
  }
  ORTCHAR_T* model_path = convert_c_to_wc(argv[1]);
  ORTCHAR_T* input_file = convert_c_to_wc(argv[2]);
  ORTCHAR_T* output_file = convert_c_to_wc(argv[3]);

  const OrtApiBase* ortBase;
  const OrtApi* ort = NULL;
  const char* ortVersion;
  OrtEnv* env;
  OrtSessionOptions* session_options;
  OrtSession* session;

  ortBase = OrtGetApiBase();
  ort = ortBase->GetApi(ORT_API_VERSION);
  ortVersion = ortBase->GetVersionString();

  if (!ort) {
    printf("Failed to init ONNX Runtime engine.\n");
    return -1;
  }
  printf("ONNX runtime version: %s\n", ortVersion);

  ORT_ABORT_ON_ERROR(ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "test", &env));
  ORT_ABORT_ON_ERROR(ort->CreateSessionOptions(&session_options));
  ORT_ABORT_ON_ERROR(ort->CreateSession(env, model_path, session_options, &session));
  int ret = 0;
  ret = run_inference(ort, session, input_file, output_file);
  ort->ReleaseSessionOptions(session_options);
  ort->ReleaseSession(session);
  ort->ReleaseEnv(env);
  if (ret != 0) {
    fprintf(stderr, "fail\n");
  }
  return ret;
}
