#include <stdio.h>
#include "onnxruntime_c_api.h"

const OrtApiBase* g_ortbase = NULL;
const OrtApi* g_ort = NULL;
const char* g_ortversion = NULL;

int main() {
  g_ortbase = OrtGetApiBase();
  g_ort = g_ortbase->GetApi(ORT_API_VERSION);
  g_ortversion = g_ortbase->GetVersionString();

  printf("ONNX runtime version: %s\n", g_ortversion);
  return 0;
}
