#pragma once

/// @file
/// @brief ID types and enums shared across the RenderGraph public surface.

#include <cstdint>

namespace goleta::rg
{

/// @brief Stable per-graph identifier for a pass. Assigned during RenderGraph::addPass.
enum class PassId : uint32_t
{
    Invalid = 0xFFFFFFFFu
};

/// @brief Per-pass index of a reflected field (input/output/inout slot).
enum class FieldId : uint32_t
{
    Invalid = 0xFFFFFFFFu
};

/// @brief Index of a merged logical resource after graph compilation. Passes' fields connected
///        by edges share the same LogicalResourceId.
enum class LogicalResourceId : uint32_t
{
    Invalid = 0xFFFFFFFFu
};

/// @brief What kind of physical resource backs a field.
enum class RgResourceType : uint8_t
{
    Texture,
    Buffer,
};

/// @brief Direction and modality of a field's access.
enum class RgAccessMode : uint8_t
{
    Input,       ///< Read-only.
    Output,      ///< Write-only; prior contents discarded.
    InputOutput, ///< Read-modify-write on the same physical resource.
};

/// @brief How a pass intends to bind a resource during execute. Drives layout + stage lookup.
enum class RgBindAs : uint8_t
{
    Auto,             ///< Infer from usage bits (Sampled if no storage flag, etc.).
    Sampled,          ///< Texture: ShaderResource layout, ShaderResourceRead access.
    Storage,          ///< Texture/Buffer: UnorderedAccess layout, UAV access.
    ColorAttachment,  ///< Texture: ColorAttachment layout.
    DepthAttachment,  ///< Texture: DepthStencil layout.
    CopySrc,
    CopyDst,
};

/// @brief Lifetime class of a logical resource. Controls pool behavior.
enum class RgLifetime : uint8_t
{
    Transient,  ///< Recycled across frames; no cross-frame state.
    Persistent, ///< Kept alive across frames (e.g. HiZ, indirect args, VT page tables).
    Imported,   ///< Owned by the caller, bound via RenderGraph::setInput.
};

/// @brief Failure reasons surfaced by RenderGraph::compile().
enum class RgError : uint32_t
{
    DuplicateField = 1,
    UnresolvedInput,
    TypeMismatch,
    DirectionMismatch,
    Cycle,
    MissingMetadata,
    ResourceCreationFailed,
    CrossGraphHandle,
    InvalidHandle,
};

} // namespace goleta::rg
