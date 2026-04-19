#pragma once

/// @file
/// @brief Forward declarations for the RenderGraph module.

namespace goleta::rg
{

class RenderGraph;
class RenderPassReflection;
class IRenderPass;
class RenderContext;
class RenderData;

struct Texture;
struct Buffer;

template <class T> struct PassInput;
template <class T> struct PassOutput;
template <class T> struct PassInputOutput;

} // namespace goleta::rg
