#pragma once

#include <cstdint>
#include <memory>

#include <numpp/core/dtype.hpp>
#include <numpp/core/error.hpp>
#include <numpp/core/ndarray.hpp>

#include "cyberfluids/interop/dlpack.h"

/// Zero-copy DLPack export of a NumPP ndarray. The returned DLManagedTensor
/// aliases the array's buffer (no copy) and holds its own reference to the
/// buffer, so the data outlives the source field for as long as the tensor (or a
/// consumer built from it) is alive. The caller owns the returned pointer and
/// MUST invoke its `deleter` exactly once — DLPack consumers (numpy/torch
/// from_dlpack) do this automatically when the imported view is released.
/// See openspec/specs/language-bindings/spec.md.
namespace cyberfluids::interop {

namespace detail {
/// Manager context kept alive by the DLManagedTensor: a refcount bump on the
/// buffer (NOT a data copy) plus the heap shape/stride arrays the DLTensor
/// points at.
struct DlpackCtx {
    std::shared_ptr<numpp::Buffer> buffer;
    numpp::Shape shape;
    numpp::Strides strides;  // element strides; empty when C-contiguous
};
}  // namespace detail

inline DLDataType dl_dtype(const numpp::DType& dt) {
    using Id = numpp::DTypeId;
    const auto bits = static_cast<uint8_t>(dt.itemsize() * 8);
    switch (dt.id()) {
        case Id::Bool:
            return DLDataType{kDLBool, 8, 1};
        case Id::Int8:
        case Id::Int16:
        case Id::Int32:
        case Id::Int64:
            return DLDataType{kDLInt, bits, 1};
        case Id::UInt8:
        case Id::UInt16:
        case Id::UInt32:
        case Id::UInt64:
            return DLDataType{kDLUInt, bits, 1};
        case Id::Float16:
        case Id::Float32:
        case Id::Float64:
            return DLDataType{kDLFloat, bits, 1};
        case Id::Complex64:
        case Id::Complex128:
            return DLDataType{kDLComplex, bits, 1};
        default:
            throw numpp::type_error("to_dlpack: dtype not DLPack-exportable");
    }
}

inline DLManagedTensor* to_dlpack(const numpp::ndarray& a) {
    // Resolve the dtype BEFORE allocating: dl_dtype throws for an unexportable
    // dtype, and throwing after `new` would leak ctx/mt (and a buffer ref).
    const DLDataType dtype = dl_dtype(a.dtype());
    auto* ctx = new detail::DlpackCtx{a.buffer(), a.shape(), {}};
    auto* mt = new DLManagedTensor{};
    DLTensor& t = mt->dl_tensor;
    t.data = const_cast<char*>(a.bytes());  // buffer base + offset (offset folded in)
    t.device = DLDevice{kDLCPU, 0};
    t.ndim = static_cast<int32_t>(a.ndim());
    t.dtype = dtype;
    t.shape = ctx->shape.data();  // outlives the tensor; freed by the deleter
    if (a.c_contiguous()) {
        t.strides = nullptr;  // NULL == compact row-major
    } else {
        ctx->strides.resize(static_cast<std::size_t>(a.ndim()));
        const auto& byteStrides = a.strides();
        const int64_t itemsize = a.itemsize();
        for (int64_t i = 0; i < a.ndim(); ++i)
            ctx->strides[static_cast<std::size_t>(i)] = byteStrides[static_cast<std::size_t>(i)] / itemsize;
        t.strides = ctx->strides.data();
    }
    t.byte_offset = 0;  // offset already folded into t.data for maximal consumer compat
    mt->manager_ctx = ctx;
    mt->deleter = [](DLManagedTensor* self) {
        delete static_cast<detail::DlpackCtx*>(self->manager_ctx);
        delete self;
    };
    return mt;
}

}  // namespace cyberfluids::interop
