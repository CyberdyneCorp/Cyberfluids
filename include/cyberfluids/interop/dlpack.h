/* Vendored DLPack ABI (https://github.com/dmlc/dlpack) — the header-only,
 * cross-framework zero-copy tensor-exchange protocol. This is an ABI SPEC, not a
 * library: the struct/enum layout below is fixed and shared by NumPy, PyTorch,
 * JAX, etc., so vendoring it (rather than taking a dependency) is the standard,
 * intended use. Layout is byte-identical to the upstream/NumPP copy so tensors
 * exported here import correctly into any DLPack consumer.
 *
 * Guarded by a named macro so a translation unit that also (indirectly) pulls in
 * another vendored copy does not get a duplicate definition. */
#ifndef CYBERFLUIDS_INTEROP_DLPACK_H
#define CYBERFLUIDS_INTEROP_DLPACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { kDLCPU = 1, kDLCUDA = 2, kDLCUDAHost = 3, kDLOpenCL = 4 } DLDeviceType;

typedef struct {
    DLDeviceType device_type;
    int32_t device_id;
} DLDevice;

typedef enum {
    kDLInt = 0,
    kDLUInt = 1,
    kDLFloat = 2,
    kDLBfloat = 4,
    kDLComplex = 5,
    kDLBool = 6,
} DLDataTypeCode;

typedef struct {
    uint8_t code;
    uint8_t bits;
    uint16_t lanes;
} DLDataType;

typedef struct {
    void* data;
    DLDevice device;
    int32_t ndim;
    DLDataType dtype;
    int64_t* shape;
    int64_t* strides; /* in elements (not bytes); NULL means C-contiguous */
    uint64_t byte_offset;
} DLTensor;

typedef struct DLManagedTensor {
    DLTensor dl_tensor;
    void* manager_ctx;
    void (*deleter)(struct DLManagedTensor* self);
} DLManagedTensor;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // CYBERFLUIDS_INTEROP_DLPACK_H
