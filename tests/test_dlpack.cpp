/// Validates the zero-copy DLPack exporter (cyberfluids::interop::to_dlpack):
///   - a C-contiguous float64 {9,12} array exports CPU/float64/correct-shape with
///     NULL strides and data aliasing the source buffer;
///   - writing through the DLTensor data pointer mutates the source (zero-copy);
///   - after the tensor's deleter runs the source buffer is still alive (the
///     export holds its own reference — borrow semantics);
///   - a non-contiguous (transposed) float32 view exports non-NULL element strides.

#include <cstdint>

#include <numpp/core/creation.hpp>
#include <numpp/core/ndarray.hpp>

#include "cyberfluids/interop/dlpack_export.hpp"
#include "testing.hpp"

int main() {
    // ---- C-contiguous float64 {9,12}. -------------------------------------
    {
        numpp::ndarray a = numpp::zeros({9, 12}, numpp::kFloat64);
        DLManagedTensor* mt = cyberfluids::interop::to_dlpack(a);
        const DLTensor& t = mt->dl_tensor;

        CF_CHECK(t.device.device_type == kDLCPU);
        CF_CHECK(t.device.device_id == 0);
        CF_CHECK(t.ndim == 2);
        CF_CHECK(t.dtype.code == kDLFloat);
        CF_CHECK(t.dtype.bits == 64);
        CF_CHECK(t.dtype.lanes == 1);
        CF_CHECK(t.shape[0] == 9 && t.shape[1] == 12);
        CF_CHECK(t.strides == nullptr);  // C-contiguous -> NULL strides
        CF_CHECK(t.byte_offset == 0);
        CF_CHECK(t.data == static_cast<const void*>(a.bytes()));  // aliases source

        // Zero-copy write-through: mutate via the DLTensor, observe on the source.
        static_cast<double*>(t.data)[7] = 99.0;
        CF_CHECK_CLOSE(a.typed_data<double>()[7], 99.0, 0.0);

        // Deleter releases the export's buffer reference; the source array still
        // owns its buffer, so the data survives.
        mt->deleter(mt);
        CF_CHECK_CLOSE(a.typed_data<double>()[7], 99.0, 0.0);
    }

    // ---- Non-contiguous transposed float32 view -> element strides. --------
    {
        numpp::ndarray a = numpp::zeros({4, 6}, numpp::kFloat32);
        numpp::ndarray tr = a.transpose();  // {6,4}, non-C-contiguous
        if (!tr.c_contiguous()) {           // guard: only meaningful if truly strided
            DLManagedTensor* mt = cyberfluids::interop::to_dlpack(tr);
            const DLTensor& t = mt->dl_tensor;
            CF_CHECK(t.dtype.code == kDLFloat && t.dtype.bits == 32);
            CF_CHECK(t.shape[0] == 6 && t.shape[1] == 4);
            CF_CHECK(t.strides != nullptr);  // strided -> explicit element strides
            const int64_t is = tr.itemsize();
            CF_CHECK(t.strides[0] == tr.strides()[0] / is);
            CF_CHECK(t.strides[1] == tr.strides()[1] / is);
            mt->deleter(mt);
        }
    }

    // ---- Unexportable dtype is rejected cleanly. --------------------------
    // Regression for the exception-safety leak: to_dlpack resolves the dtype
    // (which throws here) BEFORE allocating the ctx/tensor, so an unexportable
    // dtype leaks nothing. We exercise the throwing path via dl_dtype directly.
    {
        bool threw = false;
        try {
            cyberfluids::interop::dl_dtype(numpp::DType(numpp::DTypeId::String));
        } catch (const numpp::type_error&) {
            threw = true;
        }
        CF_CHECK(threw);
    }

    if (cftest::failures == 0) std::printf("dlpack: all checks passed\n");
    return cftest::failures;
}
