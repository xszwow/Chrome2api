#include <windows.h>
#include <psapi.h>
#include <wincodec.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <exception>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "tag_148_headers/dawn/dawn_proc_table_generated.h"

namespace ml {

enum class ModelBackendType {
  kGpuBackend,
  kApuBackend,
  kCpuBackend,
};

enum class ModelPerformanceHint {
  kHighestQuality,
  kFastestInference,
};

}  // namespace ml

using ChromeMLModel = uintptr_t;
using ChromeMLSession = uintptr_t;
using ChromeMLCancel = uintptr_t;
using ChromeMLConstraint = uintptr_t;
using PlatformFile = void*;
using ChromeMLFatalErrorFn = void (*)(const char* msg);
using ChromeMLScheduleFn = void (*)(uintptr_t context, void* task);

constexpr uint32_t kReserveTokensForSafety = 2;

static inline PlatformFile InvalidPlatformFile() {
  return reinterpret_cast<PlatformFile>(static_cast<intptr_t>(-1));
}

struct ChromeMLModelData {
  PlatformFile weights_file = InvalidPlatformFile();
  uint32_t file_id_value = 0;
  bool file_id_has_value = false;
  uint8_t file_id_padding[3] = {};
  PlatformFile cache_file = InvalidPlatformFile();
  PlatformFile encoder_cache_file = InvalidPlatformFile();
  PlatformFile adapter_cache_file = InvalidPlatformFile();
  const char* model_path = nullptr;
  const char* sentencepiece_model_path = nullptr;
};

struct ChromeMLModelDescriptor {
  ml::ModelBackendType backend_type;
  const ChromeMLModelData* model_data;
  uint32_t max_tokens;
  float temperature;
  int top_k;
  int num_draft_tokens;
  const void* ts_data;
  size_t ts_size;
  const void* ts_spm_data;
  size_t ts_spm_size;
  size_t ts_dimension;
  const uint32_t* adaptation_ranks;
  size_t adaptation_ranks_size;
  bool prefer_texture_weights;
  bool enable_host_mapped_pointer;
  bool use_low_power;
  bool allow_fp16;
  ml::ModelPerformanceHint performance_hint;
};

struct ChromeMLAdaptationDescriptor {
  const ChromeMLModelData* model_data;
  uint32_t max_tokens;
  uint32_t top_k;
  float temperature;
  bool enable_speculative_decoding;
  bool enable_image_input;
  bool enable_audio_input;
};

struct ChromeMLPerformanceInfo {
  float input_speed = 0.0f;
  float output_speed = 0.0f;
  bool is_integrated_gpu = false;
  uint64_t device_heap_size = 0;
  uint64_t max_buffer_size = 0;
};

struct ChromeMLCapabilities {
  bool image_input = false;
  bool audio_input = false;
};

struct ChromeFunctionRaw;

enum class ChromeMLGenerateStatus {
  kInProgress,
  kComplete,
  kInvalidConstraint,
};

struct ChromeMLToolCall {
  const char* call_id;
  const char* name;
  const char* arguments_json;
};

struct ChromeMLGenerateOutput {
  ChromeMLGenerateStatus status;
  const char* text;
  const ChromeMLToolCall* tool_calls = nullptr;
  size_t tool_calls_size = 0;
};

struct ChromeMLAppendOptions {
  const void* input;
  size_t input_size;
  uint32_t max_tokens;
  const ChromeFunctionRaw* context_saved_fn;
  int input_source;
};

struct ChromeMLGenerateOptions {
  uint32_t max_output_tokens;
  ChromeMLConstraint constraint;
  const ChromeFunctionRaw* output_fn;
};

struct ChromeMLMetricsFns {
  void (*RecordExactLinearHistogram)(const char* name,
                                     int sample,
                                     int exclusive_max);
  void (*RecordCustomCountsHistogram)(const char* name,
                                      int sample,
                                      int min,
                                      int exclusive_max,
                                      size_t buckets);
  void (*RecordMediumTimesHistogram)(const char* name, int64_t milliseconds);
};

struct ChromeMLConstraintMask {
  uint32_t* sample_mask;
  bool is_stop;
};

struct ChromeMLConstraintFns {
  void (*Delete)(ChromeMLConstraint constraint);
  bool (*ComputeMask)(ChromeMLConstraint constraint, ChromeMLConstraintMask* mask);
  bool (*CommitToken)(ChromeMLConstraint constraint, uint32_t token);
  bool (*IsStopped)(ChromeMLConstraint constraint);
  const char* (*GetError)(ChromeMLConstraint constraint);
  ChromeMLConstraint (*Clone)(ChromeMLConstraint constraint);
};

struct ChromeFunctionPolicy {
  void* clone;
  void* destroy;
  bool is_null;
  uint8_t padding[7];
  const void* type_info;
};

struct ChromeFunctionRaw {
  void* unused0;
  void* storage[2];
  void* invoker;
  const ChromeFunctionPolicy* policy;
};
static_assert(sizeof(ChromeFunctionRaw) == 40);

static const ChromeFunctionPolicy kChromeFunctionPolicy = {
    nullptr, nullptr, false, {}, nullptr};

struct ChromeString {
  const char* data;
  size_t size;
  size_t capacity_with_long_flag;
};
static_assert(sizeof(ChromeString) == 24);

struct ChromeStringHolder {
  std::string text;
  ChromeString chrome = {};

  void Init(std::string value) {
    text = std::move(value);
    chrome.data = text.data();
    chrome.size = text.size();
    chrome.capacity_with_long_flag =
        (static_cast<size_t>(1) << (sizeof(size_t) * 8 - 1)) | text.size();
  }
};

struct ChromeVectorFloat {
  const float* begin = nullptr;
  const float* end = nullptr;
  const float* cap = nullptr;
};
static_assert(sizeof(ChromeVectorFloat) == 24);

struct ChromeAudioBufferView {
  int32_t sample_rate_hz = 0;
  int32_t num_channels = 0;
  int32_t num_frames = 0;
  int32_t padding = 0;
  ChromeVectorFloat data;
};
static_assert(sizeof(ChromeAudioBufferView) == 40);

struct AudioInputData {
  int32_t sample_rate_hz = 0;
  int32_t num_channels = 1;
  int32_t num_frames = 0;
  std::vector<float> samples;

  ChromeAudioBufferView View() const {
    ChromeAudioBufferView view;
    view.sample_rate_hz = sample_rate_hz;
    view.num_channels = num_channels;
    view.num_frames = num_frames;
    view.data.begin = samples.data();
    view.data.end = samples.data() + samples.size();
    view.data.cap = samples.data() + samples.size();
    return view;
  }
};

struct ChromeSkColorInfo {
  void* color_space = nullptr;
  int32_t color_type = 4;  // kRGBA_8888_SkColorType
  int32_t alpha_type = 3;  // kUnpremul_SkAlphaType
};
static_assert(sizeof(ChromeSkColorInfo) == 16);

struct ChromeSkImageInfo {
  ChromeSkColorInfo color_info;
  int32_t width = 0;
  int32_t height = 0;
};
static_assert(sizeof(ChromeSkImageInfo) == 24);

struct ChromeSkPixmap {
  const void* pixels = nullptr;
  size_t row_bytes = 0;
  ChromeSkImageInfo info;
};
static_assert(sizeof(ChromeSkPixmap) == 40);

struct ChromeSkBitmapView {
  void* pixel_ref = nullptr;
  ChromeSkPixmap pixmap;
};
static_assert(sizeof(ChromeSkBitmapView) == 48);

static uintptr_t FakeSkVirtualReturnZero() {
  return 0;
}

static uintptr_t FakeSkVirtualReturnPixelRef() {
  return 1;  // SkPixelStorage::Type::kPixelRef
}

static void** FakeSkPixelRefVTable() {
  static void* table[64] = {};
  static bool initialized = false;
  if (!initialized) {
    for (void*& entry : table) {
      entry = reinterpret_cast<void*>(&FakeSkVirtualReturnZero);
    }
    table[1] = reinterpret_cast<void*>(&FakeSkVirtualReturnPixelRef);
    initialized = true;
  }
  return table;
}

struct ChromeFakeSkPixelRef {
  void** vtable = nullptr;         // +0x00
  int32_t ref_count = 0x40000000;  // +0x08, Chrome's optimized ref path
  uint32_t storage_id = 0x434d4c50;
  void** secondary_vtable = nullptr;  // +0x10, SkPixelStorage/SkRefCnt MI builds
  int32_t secondary_ref_count = 0x40000000;
  uint32_t secondary_id = 0x5049584c;
  int32_t width = 0;  // +0x20
  int32_t height = 0;
  void* pixels = nullptr;  // +0x28
  size_t row_bytes = 0;    // +0x30
  uint32_t tagged_gen_id = 2;
  uint32_t flags = 0;
  uint8_t tail[64] = {};

  void Init(int32_t w, int32_t h, void* pixel_data, size_t rb) {
    vtable = FakeSkPixelRefVTable();
    secondary_vtable = FakeSkPixelRefVTable();
    ref_count = 0x40000000;
    secondary_ref_count = 0x40000000;
    width = w;
    height = h;
    pixels = pixel_data;
    row_bytes = rb;
    tagged_gen_id = 2;
    flags = 0;
  }
};
static_assert(offsetof(ChromeFakeSkPixelRef, ref_count) == 0x08);
static_assert(offsetof(ChromeFakeSkPixelRef, secondary_vtable) == 0x10);
static_assert(offsetof(ChromeFakeSkPixelRef, width) == 0x20);
static_assert(offsetof(ChromeFakeSkPixelRef, height) == 0x24);
static_assert(offsetof(ChromeFakeSkPixelRef, pixels) == 0x28);
static_assert(offsetof(ChromeFakeSkPixelRef, row_bytes) == 0x30);

struct ImageInputData {
  int32_t width = 0;
  int32_t height = 0;
  std::vector<uint8_t> rgba;
  mutable ChromeFakeSkPixelRef pixel_ref;

  ChromeSkBitmapView View() const {
    ChromeSkBitmapView view;
    const size_t row_bytes = static_cast<size_t>(width) * 4;
    pixel_ref.Init(width, height, const_cast<uint8_t*>(rgba.data()), row_bytes);
    view.pixel_ref = const_cast<ChromeFakeSkPixelRef*>(&pixel_ref);
    view.pixmap.pixels = rgba.data();
    view.pixmap.row_bytes = row_bytes;
    view.pixmap.info.width = width;
    view.pixmap.info.height = height;
    return view;
  }
};

using GetChromeMLAPI = void* const* (*)(bool enable_litert_lm);
using SetFatalErrorFn = void (*)(ChromeMLFatalErrorFn error_fn);
using SetMetricsFns = void (*)(const ChromeMLMetricsFns* fns);
using GetEstimatedPerformanceFn = bool (*)(ChromeMLPerformanceInfo* info);
using GetCapabilitiesFn = bool (*)(PlatformFile file,
                                   ChromeMLCapabilities& capabilities);
using QueryGPUAdapterFn =
    bool (*)(void (*adapter_callback_fn)(WGPUAdapter adapter, void* userdata),
             void* userdata);
using SessionCreateModelFn =
    ChromeMLModel (*)(const ChromeMLModelDescriptor* descriptor,
                      uintptr_t context,
                      ChromeMLScheduleFn schedule);
using DestroyModelFn = void (*)(ChromeMLModel model);
using SessionAppendFn = bool (*)(ChromeMLSession session,
                                 const ChromeMLAppendOptions* options,
                                 ChromeMLCancel cancel);
using SessionGenerateFn = bool (*)(ChromeMLSession session,
                                   const ChromeMLGenerateOptions* options,
                                   ChromeMLCancel cancel);
using CreateSessionFn =
    ChromeMLSession (*)(ChromeMLModel model,
                        const ChromeMLAdaptationDescriptor* descriptor);
using DestroySessionFn = void (*)(ChromeMLSession session);
using CreateCancelFn = ChromeMLCancel (*)();
using DestroyCancelFn = void (*)(ChromeMLCancel cancel);
using CancelExecuteModelFn = void (*)(ChromeMLCancel cancel);
using SessionSizeInTokensFn = void (*)(ChromeMLSession session,
                                       const ChromeString& text,
                                       const ChromeFunctionRaw* fn);
using SessionSizeInTokensInputPieceFn = void (*)(ChromeMLSession session,
                                                 ChromeMLModel model,
                                                 const void* input,
                                                 size_t input_size,
                                                 const ChromeFunctionRaw* fn);
using SessionScoreFn = void (*)(ChromeMLSession session,
                                const ChromeString& text,
                                const ChromeFunctionRaw* fn);
using SetConstraintFns = void (*)(const ChromeMLConstraintFns* fns);
using InitDawnProcsFn = void (*)(const DawnProcTable& procs);
using TerminateProcessFn = BOOL(WINAPI*)(HANDLE, UINT);
using ExitProcessFn = VOID(WINAPI*)(UINT);
using RaiseFailFastExceptionFn = VOID(WINAPI*)(PEXCEPTION_RECORD,
                                               PCONTEXT,
                                               DWORD);
using RaiseExceptionFn = VOID(WINAPI*)(DWORD, DWORD, DWORD,
                                        const ULONG_PTR*);
using CreateFileMappingAFn =
    HANDLE(WINAPI*)(HANDLE, LPSECURITY_ATTRIBUTES, DWORD, DWORD, DWORD, LPCSTR);
using MapViewOfFileFn = LPVOID(WINAPI*)(HANDLE, DWORD, DWORD, DWORD, SIZE_T);
using VirtualAllocFn = LPVOID(WINAPI*)(LPVOID, SIZE_T, DWORD, DWORD);

static HMODULE g_chromeml_module = nullptr;
static CreateFileMappingAFn g_create_file_mapping_a = nullptr;
static MapViewOfFileFn g_map_view_of_file = nullptr;
static VirtualAllocFn g_virtual_alloc = nullptr;
static LONG g_file_mapping_logs = 0;
static LONG g_map_view_logs = 0;
static LONG g_virtual_alloc_logs = 0;
static HMODULE g_dawn_module = nullptr;
static DawnProcTable g_dawn_procs = {};
static WGPUProcCreateInstance g_real_create_instance = nullptr;
static WGPUProcInstanceRequestAdapter g_real_instance_request_adapter = nullptr;
static WGPUProcInstanceWaitAny g_real_instance_wait_any = nullptr;
static WGPUProcAdapterRequestDevice g_real_adapter_request_device = nullptr;
static WGPUProcAdapterCreateDevice g_real_actual_adapter_create_device = nullptr;
static WGPUProcAdapterHasFeature g_real_actual_adapter_has_feature = nullptr;
static WGPUProcAdapterRequestDevice g_real_actual_adapter_request_device =
    nullptr;
static WGPUProcDeviceCreateBindGroup g_real_actual_device_create_bind_group =
    nullptr;
static WGPUProcDeviceCreateErrorTexture
    g_real_actual_device_create_error_texture = nullptr;
static WGPUProcDeviceCreateTexture g_real_actual_device_create_texture =
    nullptr;
static WGPUProcTextureCreateErrorView g_real_actual_texture_create_error_view =
    nullptr;
static WGPUProcTextureCreateView g_real_actual_texture_create_view = nullptr;
static WGPUBackendType g_force_dawn_backend = WGPUBackendType_Undefined;
static bool g_swap_texture_view_slots = true;
static bool g_trace_runtime = false;
static void* g_actual_dawn_proc_table[320] = {};
static size_t g_actual_dawn_proc_table_size = 0;

static constexpr WGPUFeatureName
    kActualFeatureChromiumExperimentalSamplingResourceTable =
        static_cast<WGPUFeatureName>(0x0005003A);
static constexpr WGPUFeatureName kActualFeatureDawnInternalUsages =
    static_cast<WGPUFeatureName>(0x00050000);
static constexpr WGPUFeatureName kActualFeatureFlexibleTextureViews =
    static_cast<WGPUFeatureName>(0x00050033);

#include "generated_dawn_proc_names.inc"
#include "generated_dawn_proc_names_actual.inc"

static size_t FindActualDawnProcIndex(const char* wanted,
                                      size_t actual_name_count) {
  for (size_t i = 0; i < actual_name_count; ++i) {
    if (std::strcmp(kActualDawnProcNames[i], wanted) == 0) {
      return i;
    }
  }
  return static_cast<size_t>(-1);
}

extern "C" uintptr_t StubDawnProc() {
  return 0;
}

extern "C" WGPUInstance StubCreateInstance(WGPUInstanceDescriptor const*) {
  std::printf("StubCreateInstance called\n");
  std::fflush(stdout);
  return nullptr;
}

static void PrintStack(const char* reason, unsigned long code);

static void PrintStringView(const char* label, WGPUStringView view) {
  if (!view.data) {
    std::printf("%s=(null)", label);
    return;
  }
  size_t length = view.length == WGPU_STRLEN ? std::strlen(view.data)
                                             : view.length;
  std::printf("%s=%.*s", label, static_cast<int>(length), view.data);
}

struct AdapterCallbackState {
  WGPURequestAdapterCallback callback = nullptr;
  void* userdata1 = nullptr;
  void* userdata2 = nullptr;
};

struct DeviceCallbackState {
  WGPURequestDeviceCallback callback = nullptr;
  void* userdata1 = nullptr;
  void* userdata2 = nullptr;
};

static void HookRequestAdapterCallback(WGPURequestAdapterStatus status,
                                       WGPUAdapter adapter,
                                       WGPUStringView message,
                                       void* userdata1,
                                       void* userdata2) {
  (void)userdata2;
  auto* state = static_cast<AdapterCallbackState*>(userdata1);
  if (g_trace_runtime) {
    std::printf("dawn_request_adapter_callback status=%d adapter=%p ",
                static_cast<int>(status), adapter);
    PrintStringView("message", message);
    std::printf("\n");
    std::fflush(stdout);
  }
  if (state && state->callback) {
    state->callback(status, adapter, message, state->userdata1,
                    state->userdata2);
  }
  delete state;
}

static void HookRequestDeviceCallback(WGPURequestDeviceStatus status,
                                      WGPUDevice device,
                                      WGPUStringView message,
                                      void* userdata1,
                                      void* userdata2) {
  (void)userdata2;
  auto* state = static_cast<DeviceCallbackState*>(userdata1);
  if (g_trace_runtime) {
    std::printf("dawn_request_device_callback status=%d device=%p ",
                static_cast<int>(status), device);
    PrintStringView("message", message);
    std::printf("\n");
    std::fflush(stdout);
  }
  if (state && state->callback) {
    state->callback(status, device, message, state->userdata1,
                    state->userdata2);
  }
  delete state;
}

static WGPUInstance HookCreateInstance(
    WGPUInstanceDescriptor const* descriptor) {
  if (g_trace_runtime) {
    std::printf("dawn_create_instance descriptor=%p\n", descriptor);
    std::fflush(stdout);
  }
  WGPUInstance ret = g_real_create_instance
                         ? g_real_create_instance(descriptor)
                         : nullptr;
  if (g_trace_runtime || !ret) {
    std::printf("dawn_create_instance_ret=%p\n", ret);
    std::fflush(stdout);
  }
  return ret;
}

static WGPUFuture HookInstanceRequestAdapter(
    WGPUInstance instance,
    WGPURequestAdapterOptions const* options,
    WGPURequestAdapterCallbackInfo callback_info) {
  WGPURequestAdapterOptions modified = {};
  if (options) {
    modified = *options;
  }
  if (g_force_dawn_backend != WGPUBackendType_Undefined) {
    modified.backendType = g_force_dawn_backend;
    options = &modified;
  }
  if (g_trace_runtime) {
    std::printf(
        "dawn_request_adapter instance=%p options=%p feature=%d power=%d fallback=%u backend=%d callback=%p mode=%d\n",
        instance, options,
        options ? static_cast<int>(options->featureLevel) : -1,
        options ? static_cast<int>(options->powerPreference) : -1,
        options ? static_cast<unsigned>(options->forceFallbackAdapter) : 0,
        options ? static_cast<int>(options->backendType) : -1,
        reinterpret_cast<void*>(callback_info.callback),
        static_cast<int>(callback_info.mode));
  }
  if (callback_info.callback) {
    auto* state = new AdapterCallbackState{
        callback_info.callback, callback_info.userdata1,
        callback_info.userdata2};
    callback_info.callback = HookRequestAdapterCallback;
    callback_info.userdata1 = state;
    callback_info.userdata2 = nullptr;
  }
  if (g_trace_runtime) {
    std::fflush(stdout);
  }
  WGPUFuture future = g_real_instance_request_adapter
                          ? g_real_instance_request_adapter(instance, options,
                                                            callback_info)
                          : WGPUFuture{0};
  if (g_trace_runtime || future.id == 0) {
    std::printf("dawn_request_adapter_future=%llu\n",
                static_cast<unsigned long long>(future.id));
    std::fflush(stdout);
  }
  return future;
}

static WGPUWaitStatus HookInstanceWaitAny(WGPUInstance instance,
                                          size_t future_count,
                                          WGPUFutureWaitInfo* futures,
                                          uint64_t timeout_ns) {
  WGPUWaitStatus status =
      g_real_instance_wait_any
          ? g_real_instance_wait_any(instance, future_count, futures,
                                     timeout_ns)
          : WGPUWaitStatus_Error;
  if (g_trace_runtime || status != WGPUWaitStatus_Success) {
    std::printf("dawn_wait_any instance=%p count=%llu timeout=%llu status=%d\n",
                instance, static_cast<unsigned long long>(future_count),
                static_cast<unsigned long long>(timeout_ns),
                static_cast<int>(status));
    std::fflush(stdout);
  }
  return status;
}

static bool AdapterSupportsFeature(WGPUAdapter adapter, WGPUFeatureName feature) {
  if (!g_real_actual_adapter_has_feature) {
    return false;
  }
  return !!g_real_actual_adapter_has_feature(adapter, feature);
}

static bool EnsureFeature(WGPUAdapter adapter,
                          std::vector<WGPUFeatureName>* features,
                          WGPUFeatureName feature) {
  for (WGPUFeatureName existing : *features) {
    if (existing == feature) {
      return false;
    }
  }
  if (g_real_actual_adapter_has_feature &&
      !AdapterSupportsFeature(adapter, feature)) {
    return false;
  }
  features->push_back(feature);
  return true;
}

static WGPUDeviceDescriptor DeviceDescriptorWithExtraFeatures(
    WGPUAdapter adapter,
    WGPUDeviceDescriptor const* descriptor,
    std::vector<WGPUFeatureName>* features,
    bool* injected_sampling,
    bool* injected_internal_usages,
    bool* injected_flexible_views) {
  WGPUDeviceDescriptor modified =
      descriptor ? *descriptor : WGPU_DEVICE_DESCRIPTOR_INIT;
  if (descriptor && descriptor->requiredFeatures &&
      descriptor->requiredFeatureCount) {
    features->assign(descriptor->requiredFeatures,
                     descriptor->requiredFeatures +
                         descriptor->requiredFeatureCount);
  }

  *injected_internal_usages =
      EnsureFeature(adapter, features, kActualFeatureDawnInternalUsages);
  *injected_flexible_views =
      EnsureFeature(adapter, features, kActualFeatureFlexibleTextureViews);
  *injected_sampling = EnsureFeature(
      adapter, features, kActualFeatureChromiumExperimentalSamplingResourceTable);

  modified.requiredFeatureCount = features->size();
  modified.requiredFeatures = features->data();
  return modified;
}

static bool AdapterSupportsSamplingResourceTable(WGPUAdapter adapter) {
  return AdapterSupportsFeature(
      adapter, kActualFeatureChromiumExperimentalSamplingResourceTable);
}

static void HookNoopSetLabel(void* object, WGPUStringView label) {
  static LONG label_logs = 0;
  LONG log_index = InterlockedIncrement(&label_logs);
  if (g_trace_runtime && log_index <= 20) {
    size_t length = label.length;
    if (length == WGPU_STRLEN && label.data) {
      length = std::strlen(label.data);
    }
    std::printf("actual_dawn_noop_set_label object=%p label=%.*s length=%llu\n",
                object, static_cast<int>(length < 96 ? length : 96),
                label.data ? label.data : "",
                static_cast<unsigned long long>(label.length));
    std::fflush(stdout);
  }
}

static WGPUDevice HookActualAdapterCreateDevice(
  WGPUAdapter adapter,
  WGPUDeviceDescriptor const* descriptor) {
  std::vector<WGPUFeatureName> features;
  bool injected_sampling = false;
  bool injected_internal = false;
  bool injected_flexible = false;
  WGPUDeviceDescriptor modified = DeviceDescriptorWithExtraFeatures(
      adapter, descriptor, &features, &injected_sampling, &injected_internal,
      &injected_flexible);
  bool supported = AdapterSupportsSamplingResourceTable(adapter);
  if (g_trace_runtime) {
    std::printf(
        "actual_dawn_create_device adapter=%p descriptor=%p features=%llu "
        "inject_sampling_resource_table=%d inject_internal_usages=%d "
        "inject_flexible_views=%d supported=%d values=",
        adapter, descriptor,
        static_cast<unsigned long long>(modified.requiredFeatureCount),
        injected_sampling ? 1 : 0, injected_internal ? 1 : 0,
        injected_flexible ? 1 : 0, supported ? 1 : 0);
    for (WGPUFeatureName feature : features) {
      std::printf("0x%08x,", static_cast<uint32_t>(feature));
    }
    std::printf("\n");
    std::fflush(stdout);
  }
  return g_real_actual_adapter_create_device
             ? g_real_actual_adapter_create_device(adapter, &modified)
             : nullptr;
}

static WGPUFuture HookAdapterRequestDevice(
    WGPUAdapter adapter,
    WGPUDeviceDescriptor const* descriptor,
  WGPURequestDeviceCallbackInfo callback_info) {
  std::vector<WGPUFeatureName> features;
  bool injected_sampling = false;
  bool injected_internal = false;
  bool injected_flexible = false;
  WGPUDeviceDescriptor modified = DeviceDescriptorWithExtraFeatures(
      adapter, descriptor, &features, &injected_sampling, &injected_internal,
      &injected_flexible);
  if (g_trace_runtime) {
    std::printf(
        "dawn_request_device adapter=%p descriptor=%p callback=%p mode=%d\n",
        adapter, descriptor, reinterpret_cast<void*>(callback_info.callback),
        static_cast<int>(callback_info.mode));
  }
  if (callback_info.callback) {
    auto* state = new DeviceCallbackState{
        callback_info.callback, callback_info.userdata1,
        callback_info.userdata2};
    callback_info.callback = HookRequestDeviceCallback;
    callback_info.userdata1 = state;
    callback_info.userdata2 = nullptr;
  }
  if (g_trace_runtime) {
    std::fflush(stdout);
  }
  WGPUFuture future =
      g_real_adapter_request_device
          ? g_real_adapter_request_device(adapter, &modified, callback_info)
          : WGPUFuture{0};
  if (g_trace_runtime || future.id == 0) {
    std::printf("dawn_request_device_future=%llu\n",
                static_cast<unsigned long long>(future.id));
    std::fflush(stdout);
  }
  return future;
}

static WGPUFuture HookActualAdapterRequestDevice(
    WGPUAdapter adapter,
    WGPUDeviceDescriptor const* descriptor,
  WGPURequestDeviceCallbackInfo callback_info) {
  std::vector<WGPUFeatureName> features;
  bool injected_sampling = false;
  bool injected_internal = false;
  bool injected_flexible = false;
  WGPUDeviceDescriptor modified = DeviceDescriptorWithExtraFeatures(
      adapter, descriptor, &features, &injected_sampling, &injected_internal,
      &injected_flexible);
  bool supported = AdapterSupportsSamplingResourceTable(adapter);
  if (g_trace_runtime) {
    std::printf(
        "actual_dawn_request_device adapter=%p descriptor=%p callback=%p mode=%d "
        "features=%llu inject_sampling_resource_table=%d inject_internal_usages=%d "
        "inject_flexible_views=%d supported=%d\n",
        adapter, descriptor, reinterpret_cast<void*>(callback_info.callback),
        static_cast<int>(callback_info.mode),
        static_cast<unsigned long long>(modified.requiredFeatureCount),
        injected_sampling ? 1 : 0, injected_internal ? 1 : 0,
        injected_flexible ? 1 : 0, supported ? 1 : 0);
  }
  if (callback_info.callback) {
    auto* state = new DeviceCallbackState{
        callback_info.callback, callback_info.userdata1,
        callback_info.userdata2};
    callback_info.callback = HookRequestDeviceCallback;
    callback_info.userdata1 = state;
    callback_info.userdata2 = nullptr;
  }
  if (g_trace_runtime) {
    std::fflush(stdout);
  }
  WGPUFuture future =
      g_real_actual_adapter_request_device
          ? g_real_actual_adapter_request_device(adapter, &modified,
                                                 callback_info)
          : WGPUFuture{0};
  if (g_trace_runtime || future.id == 0) {
    std::printf("actual_dawn_request_device_future=%llu\n",
                static_cast<unsigned long long>(future.id));
    std::fflush(stdout);
  }
  return future;
}

static WGPUTexture HookActualDeviceCreateTexture(
    WGPUDevice device,
    WGPUTextureDescriptor const* descriptor) {
  static LONG texture_logs = 0;
  LONG log_index = InterlockedIncrement(&texture_logs);
  if (g_trace_runtime && log_index <= 80 && descriptor) {
    std::printf(
        "actual_dawn_create_texture #%ld device=%p usage=0x%llx dim=%d "
        "size=%ux%ux%u format=%d mip=%u sample=%u view_formats=%llu next=%p\n",
        log_index, device,
        static_cast<unsigned long long>(descriptor->usage),
        static_cast<int>(descriptor->dimension), descriptor->size.width,
        descriptor->size.height, descriptor->size.depthOrArrayLayers,
        static_cast<int>(descriptor->format), descriptor->mipLevelCount,
        descriptor->sampleCount,
        static_cast<unsigned long long>(descriptor->viewFormatCount),
        descriptor->nextInChain);
    std::fflush(stdout);
  }
  WGPUTexture texture =
      g_real_actual_device_create_texture
          ? g_real_actual_device_create_texture(device, descriptor)
          : nullptr;
  if (((g_trace_runtime && log_index <= 80) || !texture) && descriptor) {
    std::printf("actual_dawn_create_texture_ret #%ld texture=%p\n",
                log_index, texture);
    std::fflush(stdout);
  }
  return texture;
}

static WGPUTexture HookActualDeviceCreateErrorTexture(
    WGPUDevice device,
    WGPUTextureDescriptor const* descriptor) {
  static LONG error_texture_logs = 0;
  LONG log_index = InterlockedIncrement(&error_texture_logs);
  if (g_trace_runtime && log_index <= 80) {
    if (descriptor) {
      std::printf(
          "actual_dawn_create_error_texture #%ld device=%p usage=0x%llx dim=%d "
          "size=%ux%ux%u format=%d mip=%u sample=%u view_formats=%llu next=%p\n",
          log_index, device,
          static_cast<unsigned long long>(descriptor->usage),
          static_cast<int>(descriptor->dimension), descriptor->size.width,
          descriptor->size.height, descriptor->size.depthOrArrayLayers,
          static_cast<int>(descriptor->format), descriptor->mipLevelCount,
          descriptor->sampleCount,
          static_cast<unsigned long long>(descriptor->viewFormatCount),
          descriptor->nextInChain);
    } else {
      std::printf("actual_dawn_create_error_texture #%ld descriptor=null\n",
                  log_index);
    }
    std::fflush(stdout);
  }
  WGPUTexture texture =
      g_real_actual_device_create_error_texture
          ? g_real_actual_device_create_error_texture(device, descriptor)
          : nullptr;
  if ((g_trace_runtime && log_index <= 80) || !texture) {
    std::printf("actual_dawn_create_error_texture_ret #%ld texture=%p\n",
                log_index, texture);
    std::fflush(stdout);
  }
  return texture;
}

static WGPUTextureView HookActualTextureCreateView(
    WGPUTexture texture,
    WGPUTextureViewDescriptor const* descriptor) {
  static LONG view_logs = 0;
  LONG log_index = InterlockedIncrement(&view_logs);
  if (g_trace_runtime && log_index <= 120) {
    if (descriptor) {
      std::printf(
          "actual_dawn_create_view #%ld texture=%p format=%d dim=%d mip=%u/%u "
          "array=%u/%u aspect=%d usage=0x%llx next=%p\n",
          log_index, texture, static_cast<int>(descriptor->format),
          static_cast<int>(descriptor->dimension), descriptor->baseMipLevel,
          descriptor->mipLevelCount, descriptor->baseArrayLayer,
          descriptor->arrayLayerCount, static_cast<int>(descriptor->aspect),
          static_cast<unsigned long long>(descriptor->usage),
          descriptor->nextInChain);
    } else {
      std::printf("actual_dawn_create_view #%ld texture=%p descriptor=null\n",
                  log_index, texture);
    }
    std::fflush(stdout);
  }
  WGPUTextureView view =
      g_real_actual_texture_create_view
          ? g_real_actual_texture_create_view(texture, descriptor)
          : nullptr;
  if ((g_trace_runtime && log_index <= 120) || !view) {
    std::printf("actual_dawn_create_view_ret #%ld view=%p\n", log_index,
                view);
    std::fflush(stdout);
  }
  return view;
}

static WGPUTextureView HookActualTextureCreateErrorView(
    WGPUTexture texture,
    WGPUTextureViewDescriptor const* descriptor) {
  static LONG error_view_logs = 0;
  LONG log_index = InterlockedIncrement(&error_view_logs);
  if (g_trace_runtime && log_index <= 120) {
    if (descriptor) {
      std::printf(
          "actual_dawn_create_error_view #%ld texture=%p format=%d dim=%d "
          "mip=%u/%u array=%u/%u aspect=%d usage=0x%llx next=%p\n",
          log_index, texture, static_cast<int>(descriptor->format),
          static_cast<int>(descriptor->dimension), descriptor->baseMipLevel,
          descriptor->mipLevelCount, descriptor->baseArrayLayer,
          descriptor->arrayLayerCount, static_cast<int>(descriptor->aspect),
          static_cast<unsigned long long>(descriptor->usage),
          descriptor->nextInChain);
    } else {
      std::printf(
          "actual_dawn_create_error_view #%ld texture=%p descriptor=null\n",
          log_index, texture);
    }
    std::fflush(stdout);
    if (log_index <= 3) {
      PrintStack("TextureCreateErrorView",
                 static_cast<unsigned long>(log_index));
    }
  }
  WGPUTextureView view =
      g_real_actual_texture_create_error_view
          ? g_real_actual_texture_create_error_view(texture, descriptor)
          : nullptr;
  if ((g_trace_runtime && log_index <= 120) || !view) {
    std::printf("actual_dawn_create_error_view_ret #%ld view=%p\n", log_index,
                view);
    std::fflush(stdout);
  }
  return view;
}

static WGPUBindGroup HookActualDeviceCreateBindGroup(
    WGPUDevice device,
    WGPUBindGroupDescriptor const* descriptor) {
  static LONG bind_group_logs = 0;
  LONG log_index = InterlockedIncrement(&bind_group_logs);
  if (g_trace_runtime && log_index <= 160) {
    if (descriptor) {
      size_t entry_count = descriptor->entryCount;
      std::printf(
          "actual_dawn_create_bind_group #%ld device=%p layout=%p entries=%llu "
          "next=%p\n",
          log_index, device, descriptor->layout,
          static_cast<unsigned long long>(entry_count),
          descriptor->nextInChain);
      size_t log_entries = entry_count < 12 ? entry_count : 12;
      for (size_t i = 0; i < log_entries; ++i) {
        const WGPUBindGroupEntry& entry = descriptor->entries[i];
        std::printf(
            "  bg_entry #%ld.%llu binding=%u buffer=%p offset=%llu size=%llu "
            "sampler=%p texture_view=%p next=%p\n",
            log_index, static_cast<unsigned long long>(i), entry.binding,
            entry.buffer, static_cast<unsigned long long>(entry.offset),
            static_cast<unsigned long long>(entry.size), entry.sampler,
            entry.textureView, entry.nextInChain);
      }
    } else {
      std::printf("actual_dawn_create_bind_group #%ld descriptor=null\n",
                  log_index);
    }
    std::fflush(stdout);
  }
  WGPUBindGroup group =
      g_real_actual_device_create_bind_group
          ? g_real_actual_device_create_bind_group(device, descriptor)
          : nullptr;
  if ((g_trace_runtime && log_index <= 160) || !group) {
    std::printf("actual_dawn_create_bind_group_ret #%ld bind_group=%p\n",
                log_index, group);
    std::fflush(stdout);
  }
  return group;
}

static void InstallActualDawnWrappers(size_t actual_name_count) {
  const size_t kMissing = static_cast<size_t>(-1);
  const size_t adapter_create_device_index =
      FindActualDawnProcIndex("wgpuAdapterCreateDevice", actual_name_count);
  const size_t adapter_has_feature_index =
      FindActualDawnProcIndex("wgpuAdapterHasFeature", actual_name_count);
  const size_t adapter_request_device_index =
      FindActualDawnProcIndex("wgpuAdapterRequestDevice", actual_name_count);
  if (adapter_create_device_index == kMissing ||
      adapter_has_feature_index == kMissing ||
      adapter_request_device_index == kMissing) {
    std::printf("actual_dawn_wrappers=skipped missing_core_proc\n");
    std::fflush(stdout);
    return;
  }

  g_real_actual_adapter_create_device =
      reinterpret_cast<WGPUProcAdapterCreateDevice>(
          g_actual_dawn_proc_table[adapter_create_device_index]);
  g_real_actual_adapter_has_feature = reinterpret_cast<WGPUProcAdapterHasFeature>(
      g_actual_dawn_proc_table[adapter_has_feature_index]);
  g_real_actual_adapter_request_device =
      reinterpret_cast<WGPUProcAdapterRequestDevice>(
          g_actual_dawn_proc_table[adapter_request_device_index]);

  const size_t device_create_bind_group_index =
      FindActualDawnProcIndex("wgpuDeviceCreateBindGroup", actual_name_count);
  const size_t device_create_error_texture_index =
      FindActualDawnProcIndex("wgpuDeviceCreateErrorTexture",
                              actual_name_count);
  const size_t device_create_texture_index =
      FindActualDawnProcIndex("wgpuDeviceCreateTexture", actual_name_count);
  const size_t texture_create_error_view_index =
      FindActualDawnProcIndex("wgpuTextureCreateErrorView", actual_name_count);
  const size_t texture_create_view_index =
      FindActualDawnProcIndex("wgpuTextureCreateView", actual_name_count);

  if (device_create_bind_group_index != kMissing) {
    g_real_actual_device_create_bind_group =
        reinterpret_cast<WGPUProcDeviceCreateBindGroup>(
            g_actual_dawn_proc_table[device_create_bind_group_index]);
  }
  if (device_create_error_texture_index != kMissing) {
    g_real_actual_device_create_error_texture =
        reinterpret_cast<WGPUProcDeviceCreateErrorTexture>(
            g_actual_dawn_proc_table[device_create_error_texture_index]);
  }
  if (device_create_texture_index != kMissing) {
    g_real_actual_device_create_texture =
        reinterpret_cast<WGPUProcDeviceCreateTexture>(
            g_actual_dawn_proc_table[device_create_texture_index]);
  }
  if (texture_create_error_view_index != kMissing) {
    g_real_actual_texture_create_error_view =
        reinterpret_cast<WGPUProcTextureCreateErrorView>(
            g_actual_dawn_proc_table[texture_create_error_view_index]);
  }
  if (texture_create_view_index != kMissing) {
    g_real_actual_texture_create_view =
        reinterpret_cast<WGPUProcTextureCreateView>(
            g_actual_dawn_proc_table[texture_create_view_index]);
  }
  if (g_real_actual_adapter_create_device) {
    g_actual_dawn_proc_table[adapter_create_device_index] =
        reinterpret_cast<void*>(&HookActualAdapterCreateDevice);
  }
  if (g_real_actual_adapter_request_device) {
    g_actual_dawn_proc_table[adapter_request_device_index] =
        reinterpret_cast<void*>(&HookActualAdapterRequestDevice);
  }
  if (g_real_actual_device_create_bind_group) {
    g_actual_dawn_proc_table[device_create_bind_group_index] =
        reinterpret_cast<void*>(&HookActualDeviceCreateBindGroup);
  }
  if (g_real_actual_device_create_error_texture) {
    g_actual_dawn_proc_table[device_create_error_texture_index] =
        reinterpret_cast<void*>(&HookActualDeviceCreateErrorTexture);
  }
  if (g_real_actual_device_create_texture) {
    g_actual_dawn_proc_table[device_create_texture_index] =
        reinterpret_cast<void*>(&HookActualDeviceCreateTexture);
  }
  if (g_swap_texture_view_slots && g_real_actual_texture_create_error_view &&
      g_real_actual_texture_create_view) {
    g_actual_dawn_proc_table[texture_create_error_view_index] =
        reinterpret_cast<void*>(&HookActualTextureCreateView);
    g_actual_dawn_proc_table[texture_create_view_index] =
        reinterpret_cast<void*>(&HookActualTextureCreateErrorView);
  } else {
    if (g_real_actual_texture_create_error_view) {
      g_actual_dawn_proc_table[texture_create_error_view_index] =
          reinterpret_cast<void*>(&HookActualTextureCreateErrorView);
    }
    if (g_real_actual_texture_create_view) {
      g_actual_dawn_proc_table[texture_create_view_index] =
          reinterpret_cast<void*>(&HookActualTextureCreateView);
    }
  }
  size_t nooped_labels = 0;
  for (size_t i = 0; i < actual_name_count; ++i) {
    const char* name = kActualDawnProcNames[i];
    size_t name_len = std::strlen(name);
    constexpr const char kSetLabelSuffix[] = "SetLabel";
    constexpr size_t kSetLabelSuffixLen = sizeof(kSetLabelSuffix) - 1;
    if (name_len >= kSetLabelSuffixLen &&
        std::strcmp(name + name_len - kSetLabelSuffixLen,
                    kSetLabelSuffix) == 0) {
      g_actual_dawn_proc_table[i] =
          reinterpret_cast<void*>(&HookNoopSetLabel);
      ++nooped_labels;
    }
  }
  if (g_trace_runtime) {
    std::printf(
        "actual_dawn_wrappers=create_device:%d has_feature:%d request_device:%d "
        "create_bind_group:%d create_texture:%d create_error_texture:%d "
        "create_view:%d create_error_view:%d swap_texture_view_slots:%d "
        "noop_set_label:%llu\n",
        g_real_actual_adapter_create_device ? 1 : 0,
        g_real_actual_adapter_has_feature ? 1 : 0,
        g_real_actual_adapter_request_device ? 1 : 0,
        g_real_actual_device_create_bind_group ? 1 : 0,
        g_real_actual_device_create_texture ? 1 : 0,
        g_real_actual_device_create_error_texture ? 1 : 0,
        g_real_actual_texture_create_view ? 1 : 0,
        g_real_actual_texture_create_error_view ? 1 : 0,
        g_swap_texture_view_slots ? 1 : 0,
        static_cast<unsigned long long>(nooped_labels));
    std::fflush(stdout);
  }
}

static void InstallDawnWrappers() {
  g_real_create_instance = g_dawn_procs.createInstance;
  g_real_instance_request_adapter = g_dawn_procs.instanceRequestAdapter;
  g_real_instance_wait_any = g_dawn_procs.instanceWaitAny;
  g_real_adapter_request_device = g_dawn_procs.adapterRequestDevice;
  g_dawn_procs.createInstance = HookCreateInstance;
  g_dawn_procs.instanceRequestAdapter = HookInstanceRequestAdapter;
  g_dawn_procs.instanceWaitAny = HookInstanceWaitAny;
  g_dawn_procs.adapterRequestDevice = HookAdapterRequestDevice;
}

static void InitDummyDawnProcTable() {
  void** slots = reinterpret_cast<void**>(&g_dawn_procs);
  size_t count = sizeof(g_dawn_procs) / sizeof(void*);
  for (size_t i = 0; i < count; ++i) {
    slots[i] = reinterpret_cast<void*>(&StubDawnProc);
  }
  g_dawn_procs.createInstance = &StubCreateInstance;
}

static bool LoadDawnProcTableFromDll(const wchar_t* path) {
  HMODULE dawn = LoadLibraryW(path);
  if (!dawn) {
    std::printf("LoadLibraryW(webgpu_dawn) failed gle=%lu\n", GetLastError());
    std::fflush(stdout);
    return false;
  }
  g_dawn_module = dawn;
  size_t actual_name_count =
      sizeof(kActualDawnProcNames) / sizeof(kActualDawnProcNames[0]);
  size_t actual_loaded = 0;
  for (size_t i = 0; i < actual_name_count; ++i) {
    FARPROC proc = GetProcAddress(dawn, kActualDawnProcNames[i]);
    g_actual_dawn_proc_table[i] = reinterpret_cast<void*>(proc);
    if (proc) {
      ++actual_loaded;
    } else if (g_trace_runtime && i < 16) {
      std::printf("missing_actual_dawn_proc[%llu]=%s\n",
                  static_cast<unsigned long long>(i),
                  kActualDawnProcNames[i]);
    }
  }
  g_actual_dawn_proc_table_size = actual_name_count;
  InstallActualDawnWrappers(actual_name_count);
  if (g_trace_runtime) {
    std::printf("actual_dawn_table=%p actual_slots=%llu actual_loaded=%llu\n",
                g_actual_dawn_proc_table,
                static_cast<unsigned long long>(actual_name_count),
                static_cast<unsigned long long>(actual_loaded));
  }
  void** slots = reinterpret_cast<void**>(&g_dawn_procs);
  size_t slot_count = sizeof(g_dawn_procs) / sizeof(void*);
  size_t name_count = sizeof(kDawnProcNames) / sizeof(kDawnProcNames[0]);
  size_t loaded = 0;
  size_t missing = 0;
  if (g_trace_runtime) {
    std::printf("webgpu_dawn=%p slots=%llu names=%llu\n", dawn,
                static_cast<unsigned long long>(slot_count),
                static_cast<unsigned long long>(name_count));
  }
  for (size_t i = 0; i < slot_count && i < name_count; ++i) {
    FARPROC proc = GetProcAddress(dawn, kDawnProcNames[i]);
    if (proc) {
      slots[i] = reinterpret_cast<void*>(proc);
      ++loaded;
    } else {
      ++missing;
      if (g_trace_runtime && missing <= 12) {
        std::printf("missing_dawn_proc[%llu]=%s\n",
                    static_cast<unsigned long long>(i), kDawnProcNames[i]);
      }
    }
  }
  if (g_trace_runtime) {
    std::printf("loaded_dawn_procs=%llu missing=%llu\n",
                static_cast<unsigned long long>(loaded),
                static_cast<unsigned long long>(missing));
  }
  InstallDawnWrappers();
  std::fflush(stdout);
  return actual_loaded == actual_name_count ||
         (loaded > 0 && g_dawn_procs.createInstance != &StubCreateInstance);
}

static void PrintStack(const char* reason, unsigned long code) {
  void* frames[48] = {};
  USHORT count = CaptureStackBackTrace(0, 48, frames, nullptr);
  auto base = reinterpret_cast<uintptr_t>(g_chromeml_module);
  MODULEINFO mi = {};
  HMODULE psapi = LoadLibraryW(L"psapi.dll");
  using GetModuleInformationFn = BOOL(WINAPI*)(HANDLE, HMODULE, LPMODULEINFO,
                                               DWORD);
  auto get_module_information =
      psapi ? reinterpret_cast<GetModuleInformationFn>(
                  GetProcAddress(psapi, "GetModuleInformation"))
            : nullptr;
  if (get_module_information && g_chromeml_module) {
    get_module_information(GetCurrentProcess(), g_chromeml_module, &mi,
                           sizeof(mi));
  }
  uintptr_t end = base + static_cast<uintptr_t>(mi.SizeOfImage);
  std::printf("process_termination_hook reason=%s code=%lu stack_count=%hu\n",
              reason, code, count);
  for (USHORT i = 0; i < count; ++i) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(frames[i]);
    if (base && addr >= base && addr < end) {
      std::printf("  #%02hu %p optimization_guide_internal.dll+0x%llx\n", i,
                  frames[i],
                  static_cast<unsigned long long>(addr - base));
    } else {
      std::printf("  #%02hu %p\n", i, frames[i]);
    }
  }
  std::fflush(stdout);
}

static void PrintAddressModule(const char* label, void* address) {
  MEMORY_BASIC_INFORMATION mbi = {};
  if (!address || !VirtualQuery(address, &mbi, sizeof(mbi))) {
    std::printf("%s=%p module=(unknown)\n", label, address);
    return;
  }
  HMODULE module = reinterpret_cast<HMODULE>(mbi.AllocationBase);
  wchar_t wide_path[MAX_PATH] = {};
  char path[MAX_PATH * 4] = {};
  if (GetModuleFileNameW(module, wide_path, MAX_PATH)) {
    WideCharToMultiByte(CP_UTF8, 0, wide_path, -1, path, sizeof(path), nullptr,
                        nullptr);
  }
  uintptr_t base = reinterpret_cast<uintptr_t>(module);
  uintptr_t addr = reinterpret_cast<uintptr_t>(address);
  std::printf("%s=%p module_base=%p module_rva=0x%llx module=%s\n", label,
              address, module, static_cast<unsigned long long>(addr - base),
              path[0] ? path : "(unknown)");
}

static BOOL WINAPI HookTerminateProcess(HANDLE, UINT exit_code) {
  PrintStack("TerminateProcess", exit_code);
  std::_Exit(210);
}

static VOID WINAPI HookExitProcess(UINT exit_code) {
  PrintStack("ExitProcess", exit_code);
  std::_Exit(211);
}

static VOID WINAPI HookRaiseFailFastException(PEXCEPTION_RECORD,
                                              PCONTEXT,
                                              DWORD flags) {
  PrintStack("RaiseFailFastException", flags);
  std::_Exit(212);
}

static VOID WINAPI HookRaiseException(DWORD code,
                                      DWORD flags,
                                      DWORD arg_count,
                                      const ULONG_PTR* args) {
  (void)flags;
  (void)arg_count;
  (void)args;
  PrintStack("RaiseException", code);
  std::_Exit(213);
}

static bool PatchIatByName(HMODULE module,
                           const char* imported_name,
                           void* replacement) {
  auto base = reinterpret_cast<uint8_t*>(module);
  auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
  if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) {
    return false;
  }
  auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
  if (nt->Signature != IMAGE_NT_SIGNATURE) {
    return false;
  }
  auto& dir =
      nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
  if (!dir.VirtualAddress || !dir.Size) {
    return false;
  }
  bool patched = false;
  auto desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
      base + dir.VirtualAddress);
  for (; desc->Name; ++desc) {
    auto thunk = reinterpret_cast<IMAGE_THUNK_DATA*>(
        base + desc->OriginalFirstThunk);
    auto iat = reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->FirstThunk);
    if (!desc->OriginalFirstThunk) {
      thunk = iat;
    }
    for (; thunk->u1.AddressOfData; ++thunk, ++iat) {
      if (IMAGE_SNAP_BY_ORDINAL(thunk->u1.Ordinal)) {
        continue;
      }
      auto by_name =
          reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + thunk->u1.AddressOfData);
      if (std::strcmp(reinterpret_cast<const char*>(by_name->Name),
                      imported_name) != 0) {
        continue;
      }
      DWORD old_protect = 0;
      if (!VirtualProtect(&iat->u1.Function, sizeof(void*), PAGE_READWRITE,
                          &old_protect)) {
        continue;
      }
      iat->u1.Function = reinterpret_cast<ULONG_PTR>(replacement);
      DWORD ignored = 0;
      VirtualProtect(&iat->u1.Function, sizeof(void*), old_protect, &ignored);
      patched = true;
    }
  }
  return patched;
}

static HANDLE WINAPI HookCreateFileMappingA(HANDLE file,
                                            LPSECURITY_ATTRIBUTES attrs,
                                            DWORD protect,
                                            DWORD max_high,
                                            DWORD max_low,
                                            LPCSTR name) {
  HANDLE ret = g_create_file_mapping_a(file, attrs, protect, max_high, max_low,
                                       name);
  unsigned long long size =
      (static_cast<unsigned long long>(max_high) << 32) | max_low;
  LONG log_index = InterlockedIncrement(&g_file_mapping_logs);
  if ((g_trace_runtime && (log_index <= 40 || size > (128ull << 20))) ||
      !ret) {
    std::printf(
        "CreateFileMappingA file=%p protect=0x%lx size=%llu name=%s ret=%p gle=%lu\n",
        file, protect, size, name ? name : "", ret, GetLastError());
    std::fflush(stdout);
  }
  return ret;
}

static LPVOID WINAPI HookMapViewOfFile(HANDLE mapping,
                                       DWORD access,
                                       DWORD off_high,
                                       DWORD off_low,
                                       SIZE_T bytes) {
  LPVOID ret =
      g_map_view_of_file(mapping, access, off_high, off_low, bytes);
  unsigned long long offset =
      (static_cast<unsigned long long>(off_high) << 32) | off_low;
  LONG log_index = InterlockedIncrement(&g_map_view_logs);
  if ((g_trace_runtime && (log_index <= 80 || bytes > (128ull << 20))) ||
      !ret) {
    std::printf(
        "MapViewOfFile mapping=%p access=0x%lx offset=%llu bytes=%llu ret=%p gle=%lu\n",
        mapping, access, offset, static_cast<unsigned long long>(bytes), ret,
        GetLastError());
    std::fflush(stdout);
  }
  return ret;
}

static LPVOID WINAPI HookVirtualAlloc(LPVOID addr,
                                      SIZE_T size,
                                      DWORD allocation_type,
                                      DWORD protect) {
  LPVOID ret = g_virtual_alloc(addr, size, allocation_type, protect);
  LONG log_index = InterlockedIncrement(&g_virtual_alloc_logs);
  if ((g_trace_runtime && (log_index <= 40 || size > (128ull << 20))) ||
      !ret) {
    std::printf(
        "VirtualAlloc addr=%p size=%llu type=0x%lx protect=0x%lx ret=%p gle=%lu\n",
        addr, static_cast<unsigned long long>(size), allocation_type, protect,
        ret, GetLastError());
    std::fflush(stdout);
  }
  return ret;
}

static void Fatal(const char* msg) {
  static LONG fatal_logs = 0;
  LONG log_index = InterlockedIncrement(&fatal_logs);
  if (log_index <= 20) {
    std::printf("fatal_callback=%s\n", msg ? msg : "(null)");
    std::fflush(stdout);
  } else if (log_index == 21) {
    std::printf("fatal_callback=(further messages suppressed)\n");
    std::fflush(stdout);
  }
}

static LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS* info) {
  DWORD code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode
                                             : 0;
  void* address =
      info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress
                                    : nullptr;
  std::printf("unhandled_exception code=0x%08lx address=%p\n", code, address);
  std::fflush(stdout);
  return EXCEPTION_EXECUTE_HANDLER;
}

static LONG CALLBACK VectoredExceptionHandler(EXCEPTION_POINTERS* info) {
  DWORD code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode
                                             : 0;
  if (code != EXCEPTION_ACCESS_VIOLATION &&
      code != EXCEPTION_IN_PAGE_ERROR &&
      code != EXCEPTION_ILLEGAL_INSTRUCTION &&
      code != EXCEPTION_STACK_OVERFLOW &&
      code != EXCEPTION_ARRAY_BOUNDS_EXCEEDED &&
      code != EXCEPTION_INT_DIVIDE_BY_ZERO) {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  void* address =
      info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress
                                    : nullptr;
  std::printf("vectored_exception code=0x%08lx address=%p\n", code, address);
  PrintAddressModule("exception_address", address);
  if (info && info->ExceptionRecord &&
      info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
      info->ExceptionRecord->NumberParameters >= 2) {
    std::printf("access_violation op=%llu target=%p\n",
                static_cast<unsigned long long>(
                    info->ExceptionRecord->ExceptionInformation[0]),
                reinterpret_cast<void*>(
                    info->ExceptionRecord->ExceptionInformation[1]));
    PrintAddressModule(
        "access_violation_target",
        reinterpret_cast<void*>(
            info->ExceptionRecord->ExceptionInformation[1]));
  }
#if defined(_M_X64) || defined(__x86_64__)
  if (info && info->ContextRecord) {
    std::printf(
        "context rip=%p rsp=%p rbp=%p rax=%p rbx=%p rcx=%p rdx=%p r8=%p r9=%p\n",
        reinterpret_cast<void*>(info->ContextRecord->Rip),
        reinterpret_cast<void*>(info->ContextRecord->Rsp),
        reinterpret_cast<void*>(info->ContextRecord->Rbp),
        reinterpret_cast<void*>(info->ContextRecord->Rax),
        reinterpret_cast<void*>(info->ContextRecord->Rbx),
        reinterpret_cast<void*>(info->ContextRecord->Rcx),
        reinterpret_cast<void*>(info->ContextRecord->Rdx),
        reinterpret_cast<void*>(info->ContextRecord->R8),
        reinterpret_cast<void*>(info->ContextRecord->R9));
  }
#endif
  PrintStack("VectoredException", code);
  std::fflush(stdout);
  std::_Exit(230);
}

static void SignalHandler(int sig) {
  std::printf("signal=%d\n", sig);
  std::fflush(stdout);
  std::_Exit(100 + sig);
}

static void TerminateHandler() {
  std::printf("std_terminate\n");
  std::fflush(stdout);
  std::_Exit(101);
}

static void NoMetric1(const char*, int, int) {}
static void NoMetric2(const char*, int, int, int, size_t) {}
static void NoMetric3(const char*, int64_t) {}
static void ConstraintDelete(ChromeMLConstraint) {}
static bool ConstraintComputeMask(ChromeMLConstraint, ChromeMLConstraintMask*) {
  return false;
}
static bool ConstraintCommitToken(ChromeMLConstraint, uint32_t) {
  return false;
}
static bool ConstraintIsStopped(ChromeMLConstraint) {
  return false;
}
static const char* ConstraintGetError(ChromeMLConstraint) {
  return nullptr;
}
static ChromeMLConstraint ConstraintClone(ChromeMLConstraint) {
  return 0;
}

static ChromeFunctionRaw MakeChromeFunction(void* invoker,
                                            void* storage0 = nullptr,
                                            void* storage1 = nullptr) {
  ChromeFunctionRaw fn = {};
  fn.policy = &kChromeFunctionPolicy;
  fn.storage[0] = storage0;
  fn.storage[1] = storage1;
  fn.invoker = invoker;
  return fn;
}

static void CallChromeFunctionVoid(void* task) {
  if (!task) {
    return;
  }
  auto* fn = static_cast<ChromeFunctionRaw*>(task);
  if (!fn->invoker) {
    return;
  }
  using Invoker = void (*)(const void* storage);
  reinterpret_cast<Invoker>(fn->invoker)(&fn->storage[0]);
}

static void ChromeSizeInTokensInvoker(const void*, int tokens) {
  std::printf("size_in_tokens=%d\n", tokens);
  std::fflush(stdout);
}

static void ChromeScoreInvoker(const void*, float score) {
  std::printf("score=%f\n", score);
  std::fflush(stdout);
}

struct ChromeInputPiece {
  alignas(8) uint8_t bytes[0x68] = {};

  void InitToken(uint32_t token) {
    std::memset(bytes, 0, sizeof(bytes));
    std::memcpy(bytes, &token, sizeof(token));
  }

  void InitString(const ChromeString& text) {
    std::memset(bytes, 0, sizeof(bytes));
    std::memcpy(bytes, &text, sizeof(text));
    uint32_t index = 1;
    std::memcpy(bytes + 0x60, &index, sizeof(index));
  }

  void InitImage(const ChromeSkBitmapView& image) {
    std::memset(bytes, 0, sizeof(bytes));
    std::memcpy(bytes, &image, sizeof(image));
    uint32_t index = 2;
    std::memcpy(bytes + 0x60, &index, sizeof(index));
  }

  void InitAudio(const ChromeAudioBufferView& audio) {
    std::memset(bytes, 0, sizeof(bytes));
    std::memcpy(bytes, &audio, sizeof(audio));
    uint32_t index = 3;
    std::memcpy(bytes + 0x60, &index, sizeof(index));
  }
};
static_assert(sizeof(ChromeInputPiece) == 0x68);

struct InputPiecePrompt {
  ChromeStringHolder prompt;
  std::vector<ChromeInputPiece> pieces;

  void Init(std::string text,
            int mode,
            const AudioInputData* audio,
            const std::vector<const ImageInputData*>& images) {
    prompt.Init(std::move(text));
    const size_t extra = (audio ? 1 : 0) + images.size();
    if (mode == 0) {
      pieces.resize(1 + extra);
      size_t n = 0;
      pieces[n++].InitString(prompt.chrome);
      for (const ImageInputData* image : images) {
        pieces[n++].InitImage(image->View());
      }
      if (audio) {
        pieces[n++].InitAudio(audio->View());
      }
      return;
    }
    pieces.resize((mode == 1 ? 3 : 4) + extra);
    size_t n = 0;
    pieces[n++].InitToken(2);  // ml::Token::kUser
    pieces[n++].InitString(prompt.chrome);
    for (const ImageInputData* image : images) {
      pieces[n++].InitImage(image->View());
    }
    if (audio) {
      pieces[n++].InitAudio(audio->View());
    }
    pieces[n++].InitToken(3);  // ml::Token::kEnd
    if (mode != 1) {
      pieces[n++].InitToken(1);  // ml::Token::kModel
    }
  }
};

static void ChromeContextSavedInvoker(const void*, int tokens) {
  std::printf("context_saved_tokens=%d\n", tokens);
  std::fflush(stdout);
}

struct GenerateState {
  HANDLE done = nullptr;
  std::string text;
  int last_status = -1;
};

static GenerateState* g_current_generate_state = nullptr;

static void ChromeGenerateOutputInvoker(const void* storage,
                                        const ChromeMLGenerateOutput* output) {
  (void)storage;
  auto* state = g_current_generate_state;
  if (!output) {
    std::printf("generate_output=null\n");
    std::fflush(stdout);
    return;
  }
  int status = static_cast<int>(output->status);
  if (state) {
    state->last_status = status;
    if (output->text) {
      state->text.append(output->text);
    }
  }
  std::printf("generate_output status=%d text=%s tool_calls=%llu\n", status,
              output->text ? output->text : "",
              static_cast<unsigned long long>(output->tool_calls_size));
  std::fflush(stdout);
  if (status == static_cast<int>(ChromeMLGenerateStatus::kComplete) && state &&
      state->done) {
    SetEvent(state->done);
  }
}

static void ScheduleInline(uintptr_t context, void* task) {
  std::printf("schedule_inline context=0x%llx task=%p\n",
              static_cast<unsigned long long>(context), task);
  std::fflush(stdout);
  CallChromeFunctionVoid(task);
}

static void ScheduleThread(uintptr_t context, void* task) {
  std::printf("schedule_thread context=0x%llx task=%p\n",
              static_cast<unsigned long long>(context), task);
  std::fflush(stdout);
  CallChromeFunctionVoid(task);
}

static void QueryAdapterCallback(WGPUAdapter adapter, void* userdata) {
  (void)userdata;
  std::printf("query_adapter_callback adapter=%p\n", adapter);
  std::fflush(stdout);
}

struct DawnSmokeState {
  bool called = false;
  WGPURequestAdapterStatus status = WGPURequestAdapterStatus_Error;
  WGPUAdapter adapter = nullptr;
};

static void DawnSmokeAdapterCallback(WGPURequestAdapterStatus status,
                                     WGPUAdapter adapter,
                                     WGPUStringView message,
                                     void* userdata1,
                                     void*) {
  auto* state = static_cast<DawnSmokeState*>(userdata1);
  if (state) {
    state->called = true;
    state->status = status;
    state->adapter = adapter;
  }
  std::printf("dawn_smoke_callback status=%d adapter=%p ",
              static_cast<int>(status), adapter);
  PrintStringView("message", message);
  std::printf("\n");
  std::fflush(stdout);
}

static const char* BackendName(WGPUBackendType backend) {
  switch (backend) {
    case WGPUBackendType_Undefined:
      return "undefined";
    case WGPUBackendType_Null:
      return "null";
    case WGPUBackendType_WebGPU:
      return "webgpu";
    case WGPUBackendType_D3D11:
      return "d3d11";
    case WGPUBackendType_D3D12:
      return "d3d12";
    case WGPUBackendType_Vulkan:
      return "vulkan";
    default:
      return "other";
  }
}

static bool DawnSmokeOnce(WGPUBackendType backend) {
  std::printf("dawn_smoke_begin backend=%s(%d)\n", BackendName(backend),
              static_cast<int>(backend));
  WGPUInstanceDescriptor instance_desc = WGPU_INSTANCE_DESCRIPTOR_INIT;
  WGPUInstance instance = g_dawn_procs.createInstance(&instance_desc);
  if (!instance) {
    std::printf("dawn_smoke_instance=null\n");
    return false;
  }
  WGPURequestAdapterOptions options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
  options.backendType = backend;
  options.powerPreference = WGPUPowerPreference_HighPerformance;
  options.featureLevel = WGPUFeatureLevel_Core;
  DawnSmokeState state;
  WGPURequestAdapterCallbackInfo callback_info =
      WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
  callback_info.mode = WGPUCallbackMode_AllowProcessEvents;
  callback_info.callback = DawnSmokeAdapterCallback;
  callback_info.userdata1 = &state;
  WGPUFuture future =
      g_dawn_procs.instanceRequestAdapter(instance, &options, callback_info);
  if (g_dawn_procs.instanceWaitAny && future.id != 0) {
    WGPUFutureWaitInfo wait_info = WGPU_FUTURE_WAIT_INFO_INIT;
    wait_info.future = future;
    g_dawn_procs.instanceWaitAny(instance, 1, &wait_info, 5000000000ull);
  }
  for (int i = 0; i < 20 && !state.called; ++i) {
    if (g_dawn_procs.instanceProcessEvents) {
      g_dawn_procs.instanceProcessEvents(instance);
    }
    Sleep(50);
  }
  if (state.adapter && g_dawn_procs.adapterGetInfo) {
    WGPUAdapterInfo info = WGPU_ADAPTER_INFO_INIT;
    WGPUStatus info_status = g_dawn_procs.adapterGetInfo(state.adapter, &info);
    std::printf("dawn_smoke_adapter_info status=%d vendor_id=0x%x device_id=0x%x backend=%d type=%d ",
                static_cast<int>(info_status), info.vendorID, info.deviceID,
                static_cast<int>(info.backendType),
                static_cast<int>(info.adapterType));
    PrintStringView("vendor", info.vendor);
    std::printf(" ");
    PrintStringView("device", info.device);
    std::printf(" ");
    PrintStringView("description", info.description);
    std::printf("\n");
    if (g_dawn_procs.adapterInfoFreeMembers) {
      g_dawn_procs.adapterInfoFreeMembers(info);
    }
  }
  if (state.adapter && g_dawn_procs.adapterRelease) {
    g_dawn_procs.adapterRelease(state.adapter);
  }
  if (g_dawn_procs.instanceRelease) {
    g_dawn_procs.instanceRelease(instance);
  }
  std::printf("dawn_smoke_end backend=%s called=%d status=%d adapter=%p\n",
              BackendName(backend), state.called ? 1 : 0,
              static_cast<int>(state.status), state.adapter);
  std::fflush(stdout);
  return state.called && state.status == WGPURequestAdapterStatus_Success &&
         state.adapter;
}

static HANDLE OpenExistingRead(const wchar_t* path) {
  return CreateFileW(path, GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

static HANDLE OpenExistingReadWrite(const wchar_t* path) {
  return CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

static HANDLE OpenAlwaysReadWrite(const wchar_t* path) {
  return CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

static std::string WideToUtf8(const wchar_t* value) {
  if (!value) {
    return {};
  }
  int needed = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr,
                                   nullptr);
  if (needed <= 0) {
    return {};
  }
  std::string out(static_cast<size_t>(needed - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, value, -1, out.data(), needed, nullptr,
                      nullptr);
  return out;
}

static uint16_t ReadLE16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

static uint32_t ReadLE32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

static bool ReadWholeFile(const wchar_t* path, std::vector<uint8_t>* bytes) {
  bytes->clear();
  HANDLE file = OpenExistingRead(path);
  if (file == INVALID_HANDLE_VALUE) {
    std::printf("read_file_open_failed gle=%lu\n", GetLastError());
    return false;
  }
  LARGE_INTEGER size = {};
  if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 ||
      size.QuadPart > static_cast<LONGLONG>(std::numeric_limits<DWORD>::max())) {
    std::printf("read_file_size_failed gle=%lu size=%lld\n", GetLastError(),
                static_cast<long long>(size.QuadPart));
    CloseHandle(file);
    return false;
  }
  bytes->resize(static_cast<size_t>(size.QuadPart));
  DWORD read = 0;
  bool ok = bytes->empty() ||
            ReadFile(file, bytes->data(), static_cast<DWORD>(bytes->size()),
                     &read, nullptr);
  CloseHandle(file);
  if (!ok || read != bytes->size()) {
    std::printf("read_file_failed gle=%lu read=%lu expected=%llu\n",
                GetLastError(), read,
                static_cast<unsigned long long>(bytes->size()));
    bytes->clear();
    return false;
  }
  return true;
}

static bool ResampleMono(int input_rate,
                         int target_rate,
                         std::vector<float>* samples) {
  if (target_rate <= 0 || input_rate <= 0 || target_rate == input_rate ||
      samples->empty()) {
    return true;
  }
  double ratio = static_cast<double>(target_rate) / input_rate;
  size_t out_size =
      static_cast<size_t>(std::llround(samples->size() * ratio));
  if (out_size == 0) {
    return false;
  }
  std::vector<float> out(out_size);
  for (size_t i = 0; i < out_size; ++i) {
    double src = static_cast<double>(i) * input_rate / target_rate;
    size_t lo = static_cast<size_t>(src);
    size_t hi = (lo + 1 < samples->size()) ? lo + 1 : lo;
    float frac = static_cast<float>(src - lo);
    out[i] = (*samples)[lo] * (1.0f - frac) + (*samples)[hi] * frac;
  }
  *samples = std::move(out);
  return true;
}

static bool LoadWavAsMonoFloat(const wchar_t* path,
                               int target_rate,
                               AudioInputData* audio) {
  std::vector<uint8_t> bytes;
  if (!ReadWholeFile(path, &bytes)) {
    return false;
  }
  if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
      std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
    std::printf("wav_parse_failed=not_riff_wave\n");
    return false;
  }

  uint16_t format = 0;
  uint16_t channels = 0;
  uint32_t sample_rate = 0;
  uint16_t block_align = 0;
  uint16_t bits_per_sample = 0;
  const uint8_t* data = nullptr;
  size_t data_size = 0;

  size_t pos = 12;
  while (pos + 8 <= bytes.size()) {
    const uint8_t* chunk = bytes.data() + pos;
    uint32_t chunk_size = ReadLE32(chunk + 4);
    size_t payload = pos + 8;
    if (payload + chunk_size > bytes.size()) {
      std::printf("wav_parse_failed=truncated_chunk pos=%llu size=%u\n",
                  static_cast<unsigned long long>(pos), chunk_size);
      return false;
    }
    if (std::memcmp(chunk, "fmt ", 4) == 0 && chunk_size >= 16) {
      format = ReadLE16(bytes.data() + payload);
      channels = ReadLE16(bytes.data() + payload + 2);
      sample_rate = ReadLE32(bytes.data() + payload + 4);
      block_align = ReadLE16(bytes.data() + payload + 12);
      bits_per_sample = ReadLE16(bytes.data() + payload + 14);
    } else if (std::memcmp(chunk, "data", 4) == 0) {
      data = bytes.data() + payload;
      data_size = chunk_size;
    }
    pos = payload + chunk_size + (chunk_size & 1u);
  }

  if (!data || !channels || !sample_rate || !block_align || !bits_per_sample) {
    std::printf("wav_parse_failed=missing_fields format=%u channels=%u rate=%u bits=%u block=%u data=%p\n",
                format, channels, sample_rate, bits_per_sample, block_align,
                data);
    return false;
  }
  if (channels > 8) {
    std::printf("wav_parse_failed=too_many_channels channels=%u\n", channels);
    return false;
  }
  size_t bytes_per_sample = bits_per_sample / 8;
  if (bytes_per_sample == 0 || block_align < channels * bytes_per_sample) {
    std::printf("wav_parse_failed=bad_alignment bits=%u channels=%u block=%u\n",
                bits_per_sample, channels, block_align);
    return false;
  }
  size_t frames = data_size / block_align;
  std::vector<float> samples(frames);
  for (size_t frame = 0; frame < frames; ++frame) {
    double sum = 0.0;
    const uint8_t* frame_data = data + frame * block_align;
    for (uint16_t ch = 0; ch < channels; ++ch) {
      const uint8_t* sample = frame_data + ch * bytes_per_sample;
      float value = 0.0f;
      if (format == 1 && bits_per_sample == 8) {
        value = (static_cast<int>(sample[0]) - 128) / 128.0f;
      } else if (format == 1 && bits_per_sample == 16) {
        int16_t v = static_cast<int16_t>(ReadLE16(sample));
        value = v / 32768.0f;
      } else if (format == 1 && bits_per_sample == 24) {
        int32_t v = static_cast<int32_t>(sample[0]) |
                    (static_cast<int32_t>(sample[1]) << 8) |
                    (static_cast<int32_t>(sample[2]) << 16);
        if (v & 0x00800000) {
          v |= static_cast<int32_t>(0xff000000);
        }
        value = v / 8388608.0f;
      } else if (format == 1 && bits_per_sample == 32) {
        int32_t v = static_cast<int32_t>(ReadLE32(sample));
        value = v / 2147483648.0f;
      } else if (format == 3 && bits_per_sample == 32) {
        std::memcpy(&value, sample, sizeof(value));
      } else {
        std::printf("wav_parse_failed=unsupported_format format=%u bits=%u\n",
                    format, bits_per_sample);
        return false;
      }
      sum += value;
    }
    samples[frame] = static_cast<float>(sum / channels);
  }

  int final_rate = static_cast<int>(sample_rate);
  if (target_rate > 0 && target_rate != final_rate) {
    if (!ResampleMono(final_rate, target_rate, &samples)) {
      std::printf("wav_resample_failed input_rate=%d target_rate=%d\n",
                  final_rate, target_rate);
      return false;
    }
    final_rate = target_rate;
  }

  audio->sample_rate_hz = final_rate;
  audio->num_channels = 1;
  audio->num_frames = static_cast<int32_t>(samples.size());
  audio->samples = std::move(samples);
  std::printf("audio_loaded path=%s source_rate=%u sample_rate=%d channels=1 frames=%d samples=%llu\n",
              WideToUtf8(path).c_str(), sample_rate, audio->sample_rate_hz,
              audio->num_frames,
              static_cast<unsigned long long>(audio->samples.size()));
  return audio->num_frames > 0;
}

template <typename T>
static void ReleaseCom(T** value) {
  if (*value) {
    (*value)->Release();
    *value = nullptr;
  }
}

static bool LoadImageAsRgba(const wchar_t* path, ImageInputData* image) {
  image->width = 0;
  image->height = 0;
  image->rgba.clear();

  HRESULT coinit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  bool should_uninit = SUCCEEDED(coinit);
  if (FAILED(coinit) && coinit != RPC_E_CHANGED_MODE) {
    std::printf("image_coinitialize_failed hr=0x%08lx\n",
                static_cast<unsigned long>(coinit));
    return false;
  }

  IWICImagingFactory* factory = nullptr;
  IWICBitmapDecoder* decoder = nullptr;
  IWICBitmapFrameDecode* frame = nullptr;
  IWICFormatConverter* converter = nullptr;
  UINT width = 0;
  UINT height = 0;
  bool ok = false;

  HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory));
  if (FAILED(hr)) {
    std::printf("image_factory_failed hr=0x%08lx\n",
                static_cast<unsigned long>(hr));
    goto cleanup;
  }
  hr = factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                                          WICDecodeMetadataCacheOnLoad,
                                          &decoder);
  if (FAILED(hr)) {
    std::printf("image_decoder_failed hr=0x%08lx path=%s\n",
                static_cast<unsigned long>(hr), WideToUtf8(path).c_str());
    goto cleanup;
  }
  hr = decoder->GetFrame(0, &frame);
  if (FAILED(hr)) {
    std::printf("image_frame_failed hr=0x%08lx\n",
                static_cast<unsigned long>(hr));
    goto cleanup;
  }
  hr = factory->CreateFormatConverter(&converter);
  if (FAILED(hr)) {
    std::printf("image_converter_create_failed hr=0x%08lx\n",
                static_cast<unsigned long>(hr));
    goto cleanup;
  }
  hr = converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                             WICBitmapDitherTypeNone, nullptr, 0.0,
                             WICBitmapPaletteTypeCustom);
  if (FAILED(hr)) {
    std::printf("image_converter_init_failed hr=0x%08lx\n",
                static_cast<unsigned long>(hr));
    goto cleanup;
  }
  hr = converter->GetSize(&width, &height);
  if (FAILED(hr) || width == 0 || height == 0 ||
      width > 16384 || height > 16384) {
    std::printf("image_size_failed hr=0x%08lx width=%u height=%u\n",
                static_cast<unsigned long>(hr), width, height);
    goto cleanup;
  }
  {
    size_t stride = static_cast<size_t>(width) * 4;
    size_t total = stride * height;
    if (total > static_cast<size_t>(std::numeric_limits<UINT>::max())) {
      std::printf("image_too_large bytes=%llu\n",
                  static_cast<unsigned long long>(total));
      goto cleanup;
    }
    image->rgba.resize(total);
    hr = converter->CopyPixels(nullptr, static_cast<UINT>(stride),
                               static_cast<UINT>(total), image->rgba.data());
    if (FAILED(hr)) {
      std::printf("image_copy_pixels_failed hr=0x%08lx\n",
                  static_cast<unsigned long>(hr));
      goto cleanup;
    }
    image->width = static_cast<int32_t>(width);
    image->height = static_cast<int32_t>(height);
  }
  std::printf("image_loaded path=%s width=%d height=%d bytes=%llu\n",
              WideToUtf8(path).c_str(), image->width, image->height,
              static_cast<unsigned long long>(image->rgba.size()));
  ok = true;

cleanup:
  ReleaseCom(&converter);
  ReleaseCom(&frame);
  ReleaseCom(&decoder);
  ReleaseCom(&factory);
  if (should_uninit) {
    CoUninitialize();
  }
  if (!ok) {
    image->rgba.clear();
  }
  return ok;
}

int wmain(int argc, wchar_t** argv) {
  SetUnhandledExceptionFilter(UnhandledFilter);
  AddVectoredExceptionHandler(1, VectoredExceptionHandler);
  std::signal(SIGABRT, SignalHandler);
  std::signal(SIGSEGV, SignalHandler);
  std::signal(SIGILL, SignalHandler);
  std::signal(SIGFPE, SignalHandler);
  std::signal(SIGTERM, SignalHandler);
  std::set_terminate(TerminateHandler);
  const wchar_t* dll_path =
      L".\\runtime\\optimization_guide_internal.dll";
  const wchar_t* weights_path =
      L".\\model\\OptGuideOnDeviceModel\\2025.8.8.1141\\weights.bin";
  const wchar_t* cache_path = L"C:\\tmp\\chromeml_bare_cpu_cache.bin";
  const wchar_t* dawn_dll_path =
      L".\\runtime\\webgpu_dawn.dll";
  const wchar_t* audio_path = nullptr;
  std::vector<const wchar_t*> image_paths;
  bool enable_litert_lm = true;
  bool use_inline_schedule = true;
  bool use_path = false;
  bool probe_caps = false;
  bool probe_perf = false;
  bool query_adapter = false;
  bool dawn_smoke = false;
  bool probe_score = false;
  bool use_cache = true;
  bool set_callbacks = true;
  bool use_real_dawn = true;
  bool rw_weights = false;
  bool probe_input_piece_size = true;
  int backend = static_cast<int>(ml::ModelBackendType::kCpuBackend);
  int top_k = 3;
  float temperature = 0.0f;
  int input_mode = 2;
  int audio_target_rate = 0;
  uint32_t max_tokens = 4096;
  uint32_t max_output_tokens = 64;
  bool reserve_safety_tokens = true;
  std::string prompt_text = "Write one short sentence about local AI.";
  auto performance_hint = ml::ModelPerformanceHint::kHighestQuality;

  for (int i = 1; i < argc; ++i) {
    if (std::wcscmp(argv[i], L"--no-litertlm") == 0) {
      enable_litert_lm = false;
    } else if (std::wcscmp(argv[i], L"--inline-schedule") == 0) {
      use_inline_schedule = true;
    } else if (std::wcscmp(argv[i], L"--thread-schedule") == 0) {
      use_inline_schedule = false;
    } else if (std::wcscmp(argv[i], L"--path") == 0) {
      use_path = true;
    } else if (std::wcscmp(argv[i], L"--probe-caps") == 0) {
      probe_caps = true;
    } else if (std::wcscmp(argv[i], L"--probe-perf") == 0) {
      probe_perf = true;
    } else if (std::wcscmp(argv[i], L"--query-adapter") == 0) {
      query_adapter = true;
    } else if (std::wcscmp(argv[i], L"--dawn-smoke") == 0) {
      dawn_smoke = true;
    } else if (std::wcscmp(argv[i], L"--probe-score") == 0) {
      probe_score = true;
    } else if (std::wcscmp(argv[i], L"--no-cache") == 0) {
      use_cache = false;
    } else if (std::wcscmp(argv[i], L"--no-callbacks") == 0) {
      set_callbacks = false;
    } else if (std::wcscmp(argv[i], L"--no-size-input-piece") == 0) {
      probe_input_piece_size = false;
    } else if (std::wcscmp(argv[i], L"--trace-runtime") == 0 ||
               std::wcscmp(argv[i], L"--verbose-runtime") == 0) {
      g_trace_runtime = true;
    } else if (std::wcscmp(argv[i], L"--dll") == 0 && i + 1 < argc) {
      dll_path = argv[++i];
    } else if (std::wcscmp(argv[i], L"--dummy-dawn") == 0) {
      use_real_dawn = false;
    } else if (std::wcscmp(argv[i], L"--swap-texture-view-slots") == 0) {
      g_swap_texture_view_slots = true;
    } else if (std::wcscmp(argv[i], L"--no-swap-texture-view-slots") == 0) {
      g_swap_texture_view_slots = false;
    } else if (std::wcscmp(argv[i], L"--dawn-dll") == 0 && i + 1 < argc) {
      dawn_dll_path = argv[++i];
    } else if (std::wcscmp(argv[i], L"--audio") == 0 && i + 1 < argc) {
      audio_path = argv[++i];
    } else if (std::wcscmp(argv[i], L"--image") == 0 && i + 1 < argc) {
      image_paths.push_back(argv[++i]);
    } else if (std::wcscmp(argv[i], L"--audio-target-rate") == 0 &&
               i + 1 < argc) {
      audio_target_rate = std::wcstol(argv[++i], nullptr, 10);
    } else if (std::wcscmp(argv[i], L"--force-dawn-backend") == 0 &&
               i + 1 < argc) {
      const wchar_t* value = argv[++i];
      if (std::wcscmp(value, L"d3d12") == 0) {
        g_force_dawn_backend = WGPUBackendType_D3D12;
      } else if (std::wcscmp(value, L"d3d11") == 0) {
        g_force_dawn_backend = WGPUBackendType_D3D11;
      } else if (std::wcscmp(value, L"vulkan") == 0) {
        g_force_dawn_backend = WGPUBackendType_Vulkan;
      } else if (std::wcscmp(value, L"null") == 0) {
        g_force_dawn_backend = WGPUBackendType_Null;
      }
    } else if (std::wcscmp(argv[i], L"--gpu") == 0) {
      backend = static_cast<int>(ml::ModelBackendType::kGpuBackend);
    } else if (std::wcscmp(argv[i], L"--apu") == 0) {
      backend = static_cast<int>(ml::ModelBackendType::kApuBackend);
    } else if (std::wcscmp(argv[i], L"--tmp-weights") == 0) {
      weights_path = L"C:\\tmp\\chrome_weights.tfl3";
    } else if (std::wcscmp(argv[i], L"--weights") == 0 && i + 1 < argc) {
      weights_path = argv[++i];
    } else if (std::wcscmp(argv[i], L"--prompt") == 0 && i + 1 < argc) {
      prompt_text = WideToUtf8(argv[++i]);
    } else if (std::wcscmp(argv[i], L"--top1") == 0) {
      top_k = 1;
    } else if ((std::wcscmp(argv[i], L"--top-k") == 0 ||
                std::wcscmp(argv[i], L"--topk") == 0) &&
               i + 1 < argc) {
      top_k = static_cast<int>(std::wcstol(argv[++i], nullptr, 10));
      if (top_k < 1) {
        top_k = 1;
      }
    } else if (std::wcscmp(argv[i], L"--temperature") == 0 &&
               i + 1 < argc) {
      temperature = std::wcstof(argv[++i], nullptr);
      if (temperature < 0.0f) {
        temperature = 0.0f;
      }
    } else if (std::wcscmp(argv[i], L"--short") == 0) {
      max_tokens = 1024;
    } else if ((std::wcscmp(argv[i], L"--max-tokens") == 0 ||
                std::wcscmp(argv[i], L"--context-tokens") == 0) &&
               i + 1 < argc) {
      max_tokens =
          static_cast<uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
    } else if (std::wcscmp(argv[i], L"--no-safety-reserve") == 0) {
      reserve_safety_tokens = false;
    } else if (std::wcscmp(argv[i], L"--max-output") == 0 && i + 1 < argc) {
      max_output_tokens =
          static_cast<uint32_t>(std::wcstoul(argv[++i], nullptr, 10));
    } else if (std::wcscmp(argv[i], L"--fast") == 0) {
      performance_hint = ml::ModelPerformanceHint::kFastestInference;
    } else if (std::wcscmp(argv[i], L"--rw-weights") == 0) {
      rw_weights = true;
    } else if (std::wcscmp(argv[i], L"--raw-input") == 0) {
      input_mode = 0;
    } else if (std::wcscmp(argv[i], L"--user-input") == 0) {
      input_mode = 1;
    } else if (std::wcscmp(argv[i], L"--chat-input") == 0) {
      input_mode = 2;
    }
  }

  std::printf("sizeof_optional_u32=%zu\n", sizeof(std::optional<uint32_t>));
  std::printf("sizeof_model_data=%zu\n", sizeof(ChromeMLModelData));
  std::printf("sizeof_descriptor=%zu\n", sizeof(ChromeMLModelDescriptor));
  std::printf("backend=%d enable_litert_lm=%d use_path=%d schedule=%s input_mode=%d\n",
              backend, enable_litert_lm ? 1 : 0, use_path ? 1 : 0,
              use_inline_schedule ? "inline" : "thread", input_mode);
  uint32_t append_max_tokens =
      reserve_safety_tokens && max_tokens > kReserveTokensForSafety
          ? max_tokens - kReserveTokensForSafety
          : max_tokens;
  std::printf("chrome_compatible max_tokens=%u append_max_tokens=%u reserve=%u top_k=%d temperature=%.3f max_output=%u\n",
              max_tokens, append_max_tokens,
              reserve_safety_tokens ? kReserveTokensForSafety : 0, top_k,
              temperature, max_output_tokens);
  std::fflush(stdout);

  AudioInputData audio_input;
  AudioInputData* audio_ptr = nullptr;
  std::vector<ImageInputData> image_inputs;
  std::vector<const ImageInputData*> image_ptrs;
  if (audio_path) {
    if (!LoadWavAsMonoFloat(audio_path, audio_target_rate, &audio_input)) {
      return 8;
    }
    audio_ptr = &audio_input;
  }
  if (!image_paths.empty()) {
    image_inputs.resize(image_paths.size());
    image_ptrs.reserve(image_paths.size());
    for (size_t i = 0; i < image_paths.size(); ++i) {
      if (!LoadImageAsRgba(image_paths[i], &image_inputs[i])) {
        return 10;
      }
      image_ptrs.push_back(&image_inputs[i]);
    }
  }

  wchar_t dll_dir[MAX_PATH] = {};
  std::wcscpy(dll_dir, dll_path);
  wchar_t* slash = std::wcsrchr(dll_dir, L'\\');
  if (slash) {
    *slash = 0;
  }
  SetDllDirectoryW(dll_dir);
  HMODULE dll = LoadLibraryW(dll_path);
  if (!dll) {
    std::printf("LoadLibraryW failed gle=%lu\n", GetLastError());
    return 2;
  }
  g_chromeml_module = dll;
  std::printf("dll_base=%p\n", dll);
  HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
  g_create_file_mapping_a = reinterpret_cast<CreateFileMappingAFn>(
      GetProcAddress(kernel32, "CreateFileMappingA"));
  g_map_view_of_file = reinterpret_cast<MapViewOfFileFn>(
      GetProcAddress(kernel32, "MapViewOfFile"));
  g_virtual_alloc =
      reinterpret_cast<VirtualAllocFn>(GetProcAddress(kernel32, "VirtualAlloc"));
  std::printf("hook ExitProcess=%d TerminateProcess=%d RaiseFailFastException=%d RaiseException=%d CreateFileMappingA=%d MapViewOfFile=%d VirtualAlloc=%d\n",
              PatchIatByName(dll, "ExitProcess",
                             reinterpret_cast<void*>(HookExitProcess))
                  ? 1
                  : 0,
              PatchIatByName(dll, "TerminateProcess",
                             reinterpret_cast<void*>(HookTerminateProcess))
                  ? 1
                  : 0,
              PatchIatByName(
                  dll, "RaiseFailFastException",
                  reinterpret_cast<void*>(HookRaiseFailFastException))
                  ? 1
                  : 0,
              PatchIatByName(dll, "RaiseException",
                             reinterpret_cast<void*>(HookRaiseException))
                  ? 1
                  : 0,
              PatchIatByName(dll, "CreateFileMappingA",
                             reinterpret_cast<void*>(HookCreateFileMappingA))
                  ? 1
                  : 0,
              PatchIatByName(dll, "MapViewOfFile",
                             reinterpret_cast<void*>(HookMapViewOfFile))
                  ? 1
                  : 0,
              PatchIatByName(dll, "VirtualAlloc",
                             reinterpret_cast<void*>(HookVirtualAlloc))
                  ? 1
                  : 0);
  auto getter = reinterpret_cast<GetChromeMLAPI>(
      GetProcAddress(dll, "GetChromeMLAPI"));
  if (!getter) {
    std::printf("GetProcAddress(GetChromeMLAPI) failed gle=%lu\n",
                GetLastError());
    return 3;
  }
  void* const* api = getter(enable_litert_lm);
  std::printf("api=%p\n", api);
  if (!api) {
    return 4;
  }

  auto destroy_model = reinterpret_cast<DestroyModelFn>(api[4]);
  auto init_dawn = reinterpret_cast<InitDawnProcsFn>(api[0]);
  auto get_perf = reinterpret_cast<GetEstimatedPerformanceFn>(api[5]);
  auto query_gpu_adapter = reinterpret_cast<QueryGPUAdapterFn>(api[6]);
  auto get_caps = reinterpret_cast<GetCapabilitiesFn>(api[7]);
  auto set_metrics = reinterpret_cast<SetMetricsFns>(api[1]);
  auto set_fatal_gpu = reinterpret_cast<SetFatalErrorFn>(api[2]);
  auto set_fatal_nongpu = reinterpret_cast<SetFatalErrorFn>(api[8]);
  auto create_model = reinterpret_cast<SessionCreateModelFn>(api[9]);
  auto session_append = reinterpret_cast<SessionAppendFn>(api[10]);
  auto session_generate = reinterpret_cast<SessionGenerateFn>(api[11]);
  auto session_size_in_tokens =
      reinterpret_cast<SessionSizeInTokensFn>(api[13]);
  auto session_size_in_tokens_input =
      reinterpret_cast<SessionSizeInTokensInputPieceFn>(api[14]);
  auto session_score = reinterpret_cast<SessionScoreFn>(api[15]);
  auto create_session = reinterpret_cast<CreateSessionFn>(api[17]);
  auto destroy_session = reinterpret_cast<DestroySessionFn>(api[19]);
  auto create_cancel = reinterpret_cast<CreateCancelFn>(api[20]);
  auto destroy_cancel = reinterpret_cast<DestroyCancelFn>(api[21]);
  auto cancel_execute_model = reinterpret_cast<CancelExecuteModelFn>(api[22]);
  auto set_constraints = reinterpret_cast<SetConstraintFns>(api[23]);
  (void)cancel_execute_model;
  std::printf("SessionCreateModel=%p CreateSession=%p DestroyModel=%p\n",
              reinterpret_cast<void*>(create_model),
              reinterpret_cast<void*>(create_session),
              reinterpret_cast<void*>(destroy_model));
  auto dll_base = reinterpret_cast<uintptr_t>(dll);
  std::printf("SessionCreateModel_rva=0x%llx\n",
              static_cast<unsigned long long>(
                  reinterpret_cast<uintptr_t>(
                      reinterpret_cast<void*>(create_model)) -
                  dll_base));
  if (init_dawn) {
    InitDummyDawnProcTable();
    bool real_dawn = false;
    if (use_real_dawn) {
      real_dawn = LoadDawnProcTableFromDll(dawn_dll_path);
    }
    const DawnProcTable* init_table =
        g_actual_dawn_proc_table_size
            ? reinterpret_cast<const DawnProcTable*>(g_actual_dawn_proc_table)
            : &g_dawn_procs;
    init_dawn(*init_table);
    std::printf("InitDawnProcs=%s table=%p compiled_slots=%llu actual_slots=%llu\n",
                g_actual_dawn_proc_table_size
                    ? "webgpu_dawn_actual_277_table"
                    : (real_dawn ? "webgpu_dawn_manual_table" : "dummy"),
                init_table,
                static_cast<unsigned long long>(sizeof(g_dawn_procs) /
                                                sizeof(void*)),
                static_cast<unsigned long long>(g_actual_dawn_proc_table_size));
  }

  if (dawn_smoke) {
    WGPUBackendType backends[] = {
        g_force_dawn_backend != WGPUBackendType_Undefined
            ? g_force_dawn_backend
            : WGPUBackendType_Undefined,
        WGPUBackendType_D3D12,
        WGPUBackendType_Vulkan,
        WGPUBackendType_D3D11,
        WGPUBackendType_Null,
    };
    bool ok = false;
    for (WGPUBackendType candidate : backends) {
      ok = DawnSmokeOnce(candidate) || ok;
      if (g_force_dawn_backend != WGPUBackendType_Undefined) {
        break;
      }
    }
    return ok ? 0 : 7;
  }

  if (set_callbacks && set_fatal_gpu) {
    set_fatal_gpu(Fatal);
  }
  if (set_callbacks && set_fatal_nongpu) {
    set_fatal_nongpu(Fatal);
  }
  ChromeMLMetricsFns metrics = {NoMetric1, NoMetric2, NoMetric3};
  if (set_callbacks && set_metrics) {
    set_metrics(&metrics);
  }
  ChromeMLConstraintFns constraints = {ConstraintDelete, ConstraintComputeMask,
                                      ConstraintCommitToken,
                                      ConstraintIsStopped, ConstraintGetError,
                                      ConstraintClone};
  if (set_callbacks && set_constraints) {
    set_constraints(&constraints);
  }

  ChromeMLPerformanceInfo perf = {};
  if (probe_perf && get_perf) {
    std::printf(
        "GetEstimatedPerformance=%d input=%f output=%f integrated=%d heap=%llu max_buffer=%llu\n",
        get_perf(&perf) ? 1 : 0, perf.input_speed, perf.output_speed,
        perf.is_integrated_gpu ? 1 : 0,
        static_cast<unsigned long long>(perf.device_heap_size),
        static_cast<unsigned long long>(perf.max_buffer_size));
  }
  if (query_adapter && query_gpu_adapter) {
    std::printf("QueryGPUAdapter=%d\n",
                query_gpu_adapter(QueryAdapterCallback, nullptr) ? 1 : 0);
  }

  HANDLE weights = INVALID_HANDLE_VALUE;
  HANDLE cache = INVALID_HANDLE_VALUE;
  std::string path_utf8;
  if (use_path) {
    int needed = WideCharToMultiByte(CP_UTF8, 0, weights_path, -1, nullptr, 0,
                                     nullptr, nullptr);
    path_utf8.resize(static_cast<size_t>(needed));
    WideCharToMultiByte(CP_UTF8, 0, weights_path, -1, path_utf8.data(), needed,
                        nullptr, nullptr);
  } else {
    weights = rw_weights ? OpenExistingReadWrite(weights_path)
                         : OpenExistingRead(weights_path);
    if (weights == INVALID_HANDLE_VALUE) {
      std::printf("Open weights failed gle=%lu\n", GetLastError());
      return 5;
    }
    std::printf("weights_handle=%p\n", weights);
    if (probe_caps && get_caps) {
      ChromeMLCapabilities capabilities;
      bool ok = get_caps(weights, capabilities);
      std::printf("GetCapabilities=%d image=%d audio=%d\n", ok ? 1 : 0,
                  capabilities.image_input ? 1 : 0,
                  capabilities.audio_input ? 1 : 0);
      weights = INVALID_HANDLE_VALUE;
      weights = rw_weights ? OpenExistingReadWrite(weights_path)
                           : OpenExistingRead(weights_path);
      if (weights == INVALID_HANDLE_VALUE) {
        std::printf("Reopen weights after GetCapabilities failed gle=%lu\n",
                    GetLastError());
        return 5;
      }
    }
    if (use_cache &&
        backend == static_cast<int>(ml::ModelBackendType::kCpuBackend)) {
      cache = OpenAlwaysReadWrite(cache_path);
      std::printf("cache_handle=%p gle=%lu\n", cache, GetLastError());
    }
  }

  ChromeMLModelData data;
  if (use_path) {
    data.model_path = path_utf8.c_str();
    data.sentencepiece_model_path = "";
  } else {
    data.weights_file = weights;
    data.cache_file = cache == INVALID_HANDLE_VALUE
                          ? InvalidPlatformFile()
                          : static_cast<PlatformFile>(cache);
  }
  ChromeMLModelDescriptor descriptor = {};
  descriptor.backend_type = static_cast<ml::ModelBackendType>(backend);
  descriptor.model_data = &data;
  descriptor.max_tokens = max_tokens;
  descriptor.temperature = temperature;
  descriptor.top_k = top_k;
  descriptor.num_draft_tokens = 0;
  descriptor.prefer_texture_weights = true;
  descriptor.enable_host_mapped_pointer = true;
  descriptor.use_low_power = false;
  descriptor.allow_fp16 = true;
  descriptor.performance_hint = performance_hint;

  std::printf("calling SessionCreateModel\n");
  std::fflush(stdout);
  ChromeMLModel model = create_model(
      &descriptor, 0xC0DEC0DE,
      use_inline_schedule ? ScheduleInline : ScheduleThread);
  std::printf("model=0x%llx\n", static_cast<unsigned long long>(model));
  std::fflush(stdout);

  if (!model) {
    if (weights != INVALID_HANDLE_VALUE) {
      CloseHandle(weights);
    }
    if (cache != INVALID_HANDLE_VALUE) {
      CloseHandle(cache);
    }
    return 6;
  }

  ChromeMLSession session = 0;
  if (create_session) {
    ChromeMLAdaptationDescriptor adaptation = {};
    adaptation.max_tokens = max_tokens;
    adaptation.top_k = static_cast<uint32_t>(top_k);
    adaptation.temperature = temperature;
    adaptation.enable_speculative_decoding = false;
    adaptation.enable_image_input = !image_ptrs.empty();
    adaptation.enable_audio_input = audio_ptr != nullptr;
    const ChromeMLAdaptationDescriptor* adaptation_ptr =
        (audio_ptr || !image_ptrs.empty()) ? &adaptation : nullptr;
    std::printf("create_session enable_image=%d image_count=%llu enable_audio=%d\n",
                adaptation.enable_image_input ? 1 : 0,
                static_cast<unsigned long long>(image_ptrs.size()),
                adaptation.enable_audio_input ? 1 : 0);
    session = create_session(model, adaptation_ptr);
    std::printf("session=0x%llx\n", static_cast<unsigned long long>(session));
  }
  if (session && session_size_in_tokens) {
    ChromeFunctionRaw size_fn = MakeChromeFunction(
        reinterpret_cast<void*>(&ChromeSizeInTokensInvoker));
    ChromeStringHolder text;
    text.Init("Hello from bare runner.");
    std::printf("calling SessionSizeInTokens\n");
    std::fflush(stdout);
    session_size_in_tokens(session, text.chrome, &size_fn);
  }
  if (probe_score && session && session_score) {
    ChromeFunctionRaw score_fn =
        MakeChromeFunction(reinterpret_cast<void*>(&ChromeScoreInvoker));
    ChromeStringHolder text;
    text.Init("Hello");
    std::printf("calling SessionScore\n");
    std::fflush(stdout);
    session_score(session, text.chrome, &score_fn);
  }
  if (session && session_append && session_generate) {
    InputPiecePrompt input;
    input.Init(prompt_text, input_mode, audio_ptr, image_ptrs);
    if (probe_input_piece_size && session_size_in_tokens_input) {
      ChromeFunctionRaw size_piece_fn = MakeChromeFunction(
          reinterpret_cast<void*>(&ChromeSizeInTokensInvoker));
      std::printf("calling SessionSizeInTokensInputPiece pieces=%llu\n",
                  static_cast<unsigned long long>(input.pieces.size()));
      std::fflush(stdout);
      session_size_in_tokens_input(session, model, input.pieces.data(),
                                   input.pieces.size(), &size_piece_fn);
    }
    ChromeMLCancel append_cancel = create_cancel ? create_cancel() : 0;
    std::printf("append_cancel=0x%llx\n",
                static_cast<unsigned long long>(append_cancel));
    std::fflush(stdout);
    ChromeFunctionRaw context_fn = MakeChromeFunction(
        reinterpret_cast<void*>(&ChromeContextSavedInvoker));
    ChromeMLAppendOptions append_options = {};
    append_options.input = input.pieces.data();
    append_options.input_size = input.pieces.size();
    append_options.max_tokens = append_max_tokens;
    append_options.context_saved_fn = &context_fn;
    append_options.input_source = 1;
    std::printf("calling SessionAppend\n");
    std::fflush(stdout);
    bool append_ok = session_append(session, &append_options, append_cancel);
    std::printf("append_ok=%d\n", append_ok ? 1 : 0);
    std::fflush(stdout);
    if (append_cancel && destroy_cancel) {
      destroy_cancel(append_cancel);
    }

    GenerateState generate_state;
    generate_state.done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_current_generate_state = &generate_state;
    ChromeFunctionRaw output_fn = MakeChromeFunction(
        reinterpret_cast<void*>(&ChromeGenerateOutputInvoker),
        &generate_state);
    ChromeMLGenerateOptions generate_options = {};
    generate_options.max_output_tokens = max_output_tokens;
    generate_options.constraint = 0;
    generate_options.output_fn = &output_fn;
    if (append_ok && generate_state.done) {
      ChromeMLCancel generate_cancel = create_cancel ? create_cancel() : 0;
      std::printf("generate_cancel=0x%llx\n",
                  static_cast<unsigned long long>(generate_cancel));
      std::fflush(stdout);
      std::printf("calling SessionGenerate\n");
      std::fflush(stdout);
      bool generate_ok =
          session_generate(session, &generate_options, generate_cancel);
      std::printf("generate_ok=%d\n", generate_ok ? 1 : 0);
      std::fflush(stdout);
      if (generate_ok) {
        DWORD wait = WaitForSingleObject(generate_state.done, 60000);
        std::printf("generate_wait=%lu complete_status=%d full_text=%s\n",
                    wait, generate_state.last_status,
                    generate_state.text.c_str());
        std::fflush(stdout);
      }
      if (generate_cancel && destroy_cancel) {
        destroy_cancel(generate_cancel);
      }
    }
    g_current_generate_state = nullptr;
    if (generate_state.done) {
      CloseHandle(generate_state.done);
    }
  }
  if (session && destroy_session) {
    destroy_session(session);
    std::printf("destroyed_session=1\n");
  }
  if (destroy_model) {
    destroy_model(model);
    std::printf("destroyed_model=1\n");
  }
  return 0;
}
