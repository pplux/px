/* -----------------------------------------------------------------------------
Copyright (c) 2018 Jose L. Hidalgo (PpluX)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
----------------------------------------------------------------------------- */

// USAGE
//
// In *ONE* C++ file you need to declare
// #define PX_RENDER_IMPLEMENTATION
// before including the file that contains px_render.h

#ifndef PX_RENDER
#define PX_RENDER

#include <stdint.h>

// -- Uniforms ---------------------------------------------------------------
// 
// Automatic uniforms:
//   mat4 u_model
//   mat3 u_normal --> computed as the 3x3 Inverse Transpose of u_model
//   mat4 u_view
//   mat4 u_projection
//   mat4 u_modelView
//   mat4 u_modelViewProjection
//   mat4 u_viewProjection
// Inverses:
//   mat4 u_invModel
//   mat4 u_invView
//   mat4 u_invProjection
//   mat4 u_invModelView
//   mat4 u_invModelViewProjection
//   mat4 u_invViewProjection
//
// For simplicity user uniform data will be an array of vec4
// defined as:
//   vec4 u_data[num]
//
// Samplers:
//   u_tex0
//   u_tex1
//   ...
//   u_texN    (N = kMaxTextureUnits)


namespace px_render {

  struct RenderContext;

  union Vec2 {
    struct {
      float x,y;
    } v;
    struct {
      float r,g;
    } c;
    float f[2];
  };

  union Vec3 {
    struct {
      float x,y,z;
    } v;
    struct {
      float r,g,b;
    } c;
    float f[3];
  };

  union Vec4 {
    struct {
      float x,y,z,w;
    } v;
    struct {
      float r,g,b,a;
    } c;
    float f[4];
  };

  union Mat4 {
    struct {
      float m11,m21,m31,m41;
      float m12,m22,m32,m42;
      float m13,m23,m33,m43;
      float m14,m24,m34,m44;
    } m;
    Vec4 column[4];
    float f[16];
    static Mat4 Identity() {
      return Mat4 {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        };
    }
    static Mat4 Mult(const Mat4 &a, const Mat4 &b);
    static Vec4 Mult(const Mat4 &a, const Vec4 &b);
    static Mat4 Inverse(const Mat4&);
    static Mat4 Transpose(const Mat4&);
    // Retunrs a Transformation Matrix computed as Scale, then a Rotation, finally a Translation
    // note, scale x,y,z = axis, w = angle (in radians)
    static Mat4 SRT(const Vec3 &scale, const Vec4 &rotate_axis_angle, const Vec3 &translate);
  };

  static const size_t kMaxVertexAttribs = 16;
  static const size_t kMaxTextureUnits  = 16;

  struct Usage {
    enum Enum {
      Static,
      Dynamic,
      Stream,
    };
  };

  struct BufferType {
    enum Enum {
      Invalid = 0,
      Vertex,
      Index,
    };
  };

  struct TextureType {
    enum Enum {
      Invalid = 0,
      T1D,
      T2D,
      T3D,
      CubeMap,
    };
  };

  struct TexelsFormat {
    enum Enum {
      None = 0,
      R_U8,
      RG_U8,
      RGB_U8,
      RGBA_U8,
      Depth_U16,
      DepthStencil_U16,
      Depth_U24,
      DepthStencil_U24,
    };
  };

  struct SamplerWrapping {
    enum Enum {
      Repeat,
      MirroredRepeat,
      Clamp,
    };
  };

  struct SamplerFiltering {
    enum Enum {
      Nearest,
      Linear,
      NearestMipmapNearest,
      NearestMipmapLinear,
      LinearMipmapNearest,
      LinearMipmapLinear,
    };
  };

  struct Primitive {
    enum Enum {
      Lines,
      Triangles,
      Points,
    };
  };

  struct Cull {
    enum Enum {
      Disabled,
      Front,
      Back,
    };
  };

  struct BlendFactor {
    enum Enum {
      Zero,
      One,
      SrcColor,
      OneMinusSrcColor,
      SrcAlpha,
      OneMinusSrcAlpha,
      DstColor,
      OneMinusDstColor,
      DstAlpha,
      OneMinusDstAlpha,
      SrcAlphaSaturated,
      BlendColor,
      OneMinusBlendColor,
      BlendAlpha,
      OneMinusBlendAlpha,
    };
  };

  struct BlendOp {
    enum Enum {
      Add,
      Substract,
      ReverseSubstract,
      Min,
      Max
    };
  };

  struct CompareFunc {
    enum Enum {
      Disabled,
      Never,
      Less,
      LessEqual,
      Equal,
      NotEqual,
      GreaterEqual,
      Greater,
      Always,
    };
  };

  struct VertexFormat {
    enum Enum {
      //-------------------------------
      Undefined = 0,
      Float  = 0x1,
      Int8   = 0x2,
      UInt8  = 0x3,
      Int16  = 0x4,
      UInt16 = 0x5,
      Int32  = 0x6,
      UInt32 = 0x7,
      //-------------------------------
      NumComponents1 = 0x10,
      NumComponents2 = 0x20,
      NumComponents3 = 0x30,
      NumComponents4 = 0x40,
      //-------------------------------
      Normalized = 0x100,
      //-------------------------------
      Float1 = Float | NumComponents1,
      Float2 = Float | NumComponents2,
      Float3 = Float | NumComponents3,
      Float4 = Float | NumComponents4,
      //-------------------------------
      TypeMask           = 0xF,
      TypeShift          = 0,
      NumComponentsMask  = 0xF0,
      NumComponentsShift = 4,
      FlagsMask          = 0xF00,
      FlagsShift         = 8,
    };
  };

  struct VertexStep {
    enum Enum {
      PerVertex,
      PerInstance,
    };
  };

  struct VertexDeclaration {
    // name is not needed if you are using attribute layout qualifiers
    const char *name = nullptr;
    uint32_t format = 0;
    uint32_t buffer_index = 0;
    VertexStep::Enum vertex_step = VertexStep::PerVertex;

    // offset & stride = 0 will be computed once the whole pipeline is set
    // normally you should not try to manipulate them.
    uint32_t offset = 0;
    uint32_t stride = 0;
  };

  struct IndexFormat {
    enum Enum {
      UInt8,
      UInt16,
      UInt32,
    };
  };

  struct GPUResource {
    struct Type {
      enum Enum {
        Invalid,
        Texture,
        Buffer,
        Pipeline,
        Framebuffer,
      };
    };
    RenderContext* ctx;
    uint32_t id;
    Type::Enum type;
  };

  struct Texture : public GPUResource {
    Texture(RenderContext* ctx = nullptr, uint32_t id = 0) : GPUResource{ctx, id, Type::Texture} {}
    struct Info {
      uint16_t width  = 1;
      uint16_t height = 1;
      uint16_t depth  = 1;
      SamplerFiltering::Enum minification_filter = SamplerFiltering::Linear;
      SamplerFiltering::Enum magnification_filter = SamplerFiltering::Linear;
      SamplerWrapping::Enum wrapping[3] = { SamplerWrapping::Repeat, SamplerWrapping::Repeat, SamplerWrapping::Repeat };
      TexelsFormat::Enum format = TexelsFormat::None;
      Usage::Enum usage = Usage::Static;
      TextureType::Enum type = TextureType::T2D;
    };
  };

  struct Buffer : public GPUResource {
    Buffer(RenderContext *ctx = nullptr, uint32_t id = 0) : GPUResource{ctx, id, Type::Buffer} {}
    struct Info {
      BufferType::Enum type = BufferType::Invalid;
      uint32_t size = 0; // size in bytes
      Usage::Enum usage = Usage::Static;
    };
  };

  struct Framebuffer : public GPUResource {
    Framebuffer(RenderContext *ctx = nullptr, uint32_t id = 0) : GPUResource{ctx, id, Type::Framebuffer} {}
    struct Info {
      Texture::Info color_texture_info;
      Texture::Info depth_stencil_texture_info;
      uint16_t num_color_textures = 1;
    };

    Texture color_texture(uint16_t index = 0) const;
    Texture depth_stencil_texture() const;
  };

  struct Pipeline : public GPUResource {
    Pipeline(RenderContext *ctx = nullptr, uint32_t id = 0) : GPUResource{ctx, id, Type::Pipeline} {}
    struct Info {
      struct {
        const char* vertex;
        const char* fragment;
      } shader = {};
      uint32_t uniform_size = 0; // this must be multiple of sizeof(vec4)
      VertexDeclaration attribs[kMaxVertexAttribs] = {};
      TextureType::Enum textures[kMaxTextureUnits] = {};
      Primitive::Enum primitive = Primitive::Triangles;
      Cull::Enum cull = Cull::Back;
      struct {
        BlendFactor::Enum src_rgb = BlendFactor::SrcAlpha;
        BlendFactor::Enum dst_rgb = BlendFactor::OneMinusSrcAlpha;
        BlendOp::Enum     op_rgb  = BlendOp::Add;
        BlendFactor::Enum src_alpha = BlendFactor::SrcAlpha;
        BlendFactor::Enum dst_alpha = BlendFactor::OneMinusSrcAlpha;
        BlendOp::Enum     op_alpha = BlendOp::Add;
        Vec4 color = {0.0f, 0.0f, 0.0f, 0.0f};
        bool enabled = false;
      } blend;
      CompareFunc::Enum depth_func = CompareFunc::Less;
      bool rgba_write = true;
      bool depth_write = true;
    };
  };


  struct DisplayList {
    DisplayList();
    ~DisplayList();

    DisplayList(const DisplayList &d) = delete;
    DisplayList(DisplayList &&d) {
      data_ = d.data_;
      d.data_ = nullptr;
    }
    DisplayList& operator=(const DisplayList&) = delete;
    DisplayList& operator=(DisplayList &&d) {
      data_ = d.data_;
      d.data_ = nullptr;
    }

    // Helper macro to define properties... yeah, ugly but 
    // convenient.
    #define PROP(type, name, ...) \
      type name = __VA_ARGS__;\
      Self& set_##name(const type &c) { name = c; return *this; }

    #define PROP_PTR(type, name) \
      const type *name = nullptr;\
      Self& set_##name(const type *c) { name = c; return *this; }

    #define PROP_ARRAY(type, count, name) \
      type name[count] = {};\
      Self& set_##name(size_t i, const type &c) { name[i] = c; return *this; }

    struct ClearData {
      typedef ClearData Self;
      PROP(Vec4,    color,  {0.0f, 0.0f, 0.0f, 1.0f});
      PROP(float,   depth,  1.0f);
      PROP(int32_t, stencil, 0);
      PROP(bool, clear_color, true);
      PROP(bool, clear_depth, true);
      PROP(bool, clear_stencil, false);
    };

    struct SetupViewData {
      typedef SetupViewData Self;
      struct Viewport {
        uint16_t x,y,width, height;
      };
      PROP(Viewport, viewport, {});
      PROP(Mat4, view_matrix, Mat4::Identity());
      PROP(Mat4, projection_matrix, Mat4::Identity());
      PROP(Framebuffer, framebuffer, {})
    };

    struct SetupPipelineData {
      typedef SetupPipelineData Self;
      PROP(Pipeline, pipeline, {});
      PROP_ARRAY(Texture, kMaxTextureUnits, texture);
      PROP_ARRAY(Buffer, kMaxVertexAttribs, buffer);
      PROP(Vec4, scissor, {});
      PROP(Mat4, model_matrix, Mat4::Identity());
      PROP_PTR(uint8_t, uniforms);
    };
    
    struct RenderData {
      typedef RenderData Self;
      PROP(Buffer, index_buffer, {});
      PROP(uint32_t, offset, 0);
      PROP(uint32_t, count, 0);
      PROP(uint32_t, instances, 1);
      PROP(IndexFormat::Enum, type, IndexFormat::UInt16);
    };

    struct FillBufferData {
      typedef FillBufferData Self;
      PROP(Buffer, buffer, {});
      PROP(uint32_t, offset, 0);
      PROP(uint32_t, size, 0);
      PROP_PTR(void,data); // data could be null if you're for example building mipmaps only...
    };

    struct FillTextureData {
      typedef FillTextureData Self;
      PROP(Texture, texture, {});
      PROP(uint16_t, offset_x, 0);
      PROP(uint16_t, offset_y, 0);
      PROP(uint16_t, offset_z, 0);
      PROP(uint16_t, width,  0); // if 0 --> texture.width
      PROP(uint16_t, height, 0); // if 0 --> texture.height
      PROP(uint16_t, depth,  0); // if 0 --> texture.depth
      PROP(bool, build_mipmap, false);
      PROP_PTR(void,data);
    };

    #undef PROP
    #undef PROP_PTR
    #undef PROP_ARRAY

    // clears previous contents, and prepares the display list to
    // be filled from the start.
    void reset();

    // finishes last command added to the display list, copying everything it needs
    // internally. This is optional as long as you submit displaylist immediately after
    // they are filled. If you plan to store a display list you must call finish to ensure
    // all data is copied.
    void commitLastCommand();

    // -- Add then modify API --------------------------------
    // *WARNING* the result reference of these methods is only
    // valid up until any other command is added to the DisplayList
    // you should always add, modify, and forget about the variable.
    // Do not keep the reference!! in any form.
    ClearData& clearCommand();
    SetupViewData &setupViewCommand();
    SetupPipelineData& setupPipelineCommand();
    RenderData& renderCommand();
    FillBufferData& fillBufferCommand();
    FillTextureData& fillTextureCommand();
    //--------------------------------------------------------

    void add(const ClearData&d) { clearCommand() = d;}
    void add(const SetupViewData &d) { setupViewCommand() = d; }
    void add(const SetupPipelineData &d) { setupPipelineCommand() = d; }
    void add(const RenderData &d) { renderCommand() = d; }
    void add(const FillBufferData &d) { fillBufferCommand() = d; }
    void add(const FillTextureData &d) { fillTextureCommand() = d; }

    DisplayList& destroy(const Texture &t);
    DisplayList& destroy(const Buffer &b);
    DisplayList& destroy(const Pipeline&p);
    DisplayList& destroy(const Framebuffer &fb);

    DisplayList clone() const;

    struct Data;
    struct Command;
    Data *data_ = nullptr;
  };

  struct RenderContextParams {
    // Max number of any element is 2^20... the other 12 bits are
    // reserved to detect dangling pointers.

    uint32_t max_textures = 128;
    uint32_t max_buffers = 128;
    uint32_t max_framebuffers = 128;
    uint32_t max_pipelines = 64;

    // if no error callback is provider, stderr will be used to spit
    // out possible problems. Once an error is produced, there is no 
    // guarantee that any given command will not end in crashing the app.
    void (*on_error_callback)(const char *error) = nullptr;
  };

  struct RenderContext {
    RenderContext();
    ~RenderContext();

    // allocate resources to handle all objects that can be created later
    void init(const RenderContextParams &params = RenderContextParams());

    // marks the render context for destruction on the next executeOnGPU
    void finish();

    Texture createTexture(const Texture::Info &info);
    Buffer createBuffer(const Buffer::Info &info);
    Pipeline createPipeline(const Pipeline::Info &info);
    Framebuffer createFramebuffer(const Framebuffer::Info &info);

    // submits a display list, stealing all of its content, and resetting the
    // object to a default initial state.
    void submitDisplayList(DisplayList *dl);

    // submits a display list, stealing all of its content, and resetting the
    // object to a default initial state. It also issues a FrameSwap.
    void submitDisplayListAndSwap(DisplayList *dl);

    void submitDisplayList(DisplayList &&dl);
    void submitDisplayListAndSwap(DisplayList &&dl);
    void submitDisplayListCopy(const DisplayList &dl) { submitDisplayList(dl.clone()); }
    void submitDisplayListCopyAndSwap(const DisplayList &dl) { submitDisplayListAndSwap(dl.clone()); }

    struct Result {
      enum Enum {
        Finished = 0, //> stop calling executeOnGPU
        OK,           //> Last executeOnGPU ended OK
        OK_Swap,      //> Last executeOnGPU ended OK and swap is needed
                      //  to present results to the user.
      };
    };

    // This method should be called in a loop from the thread that 
    // created the render context until it returns Result::Finished.
    // If returns OK_Swap you should call the proper swap command of 
    // the window.
    Result::Enum executeOnGPU();

    RenderContext(const RenderContext&) = delete;
    RenderContext& operator=(const RenderContext&) = delete;

    struct Data;
    Data *data_ = nullptr;
  };
}

#endif // PX_RENDER

#if defined(PX_RENDER_IMPLEMENTATION) && !defined(PX_RENDER_IMPLEMENTATION_DONE)

#define PX_RENDER_IMPLEMENTATION_DONE

#if !defined(PX_RENDER_BACKEND_GL) \
 && !defined(PX_RENDER_BACKEND_GLES)
#  define PX_REDER_BACKEND_GL 
#endif

#ifndef PX_RENDER_DEBUG_LEVEL
#define PX_RENDER_DEBUG_LEVEL 100
#endif

#ifndef PX_RENDER_DEBUG_FUNC
#  ifdef PX_RENDER_DEBUG
#    define PX_RENDER_DEBUG_FUNC(LEVEL,...) {if(LEVEL< PX_RENDER_DEBUG_LEVEL) {fprintf(stdout, __VA_ARGS__);fflush(stdout);}}
#  else
#    define PX_RENDER_DEBUG_FUNC(...) /*nothing*/
#  endif
#endif


#include <vector>
#include <algorithm>
#include <memory>
#include <thread>
#include <condition_variable>
#include <type_traits>
#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include <cstring>
#include <cmath>
#include <cassert>
#include <cstdarg>

namespace px_render {


  template<class T>
  struct Mem {
    std::unique_ptr<T[]> data;
    size_t count = 0;

    T& operator[](size_t pos) {
      assert(pos < count && "Invalid position...");
      return data[pos];
    }

    const T& operator[](size_t pos) const {
      assert(pos < count && "Invalid position...");
      return data[pos];
    }

    Mem() {}

    Mem(Mem &&m) {
      data = std::move(m.data);
      count = m.count;
      m.count = 0;
    }

    Mem(const Mem &m) = delete;
    Mem& operator=(Mem &&) = delete;
    Mem& operator=(const Mem &) = delete;

    void copy(const T* ptr, size_t c) {
      data = std::make_unique<T[]>(c);
      for (size_t i = 0; i < c; ++i) {
        data[i] = ptr[i];
      }
      count = c;
    }

    void copy(const Mem &m) { copy(m.data.get(), m.count); }

    void alloc(size_t c) {
      data = std::make_unique<T[]>(c);
      count = c;
    }
  };


  // -- Implementation -------------------------------------------------------

  struct InstanceBase {
    uint32_t version = 0;
    std::atomic<uint32_t> state = {0};

    bool acquire() {
      uint32_t v = (version+1);
      if (!v) v = 1;
      uint32_t e = 0;
      if (state.compare_exchange_weak(e, v)) {
        version = v;
        return true;
      }
      return false;
    }

    void release() {
      state = 0;
    }
  };

  struct PipelineInstance : public InstanceBase {
    Pipeline::Info info;
    // Internal copy of strings
    Mem<char> vertex_shader;
    Mem<char> fragment_shader;
    Mem<char> attributes[kMaxVertexAttribs];
  };

  struct BufferInstance : public InstanceBase{
    Buffer::Info info;
  };

  struct TextureInstance : public InstanceBase{
    Texture::Info info;
    size_t bytes_per_pixel = 0;
  };

  struct FramebufferInstance : public InstanceBase{
    Framebuffer::Info info;
    Mem<Texture> color_texture;
    Texture depth_texture;
  };

  struct BackEnd;

  static size_t ComputeVertexSize(uint32_t format) {
    uint32_t type  = (format & VertexFormat::TypeMask)>> VertexFormat::TypeShift;
    uint32_t count = (format & VertexFormat::NumComponentsMask) >> VertexFormat::NumComponentsShift;
    uint32_t result = count;
    switch (type) {
      // 1
      case VertexFormat::Int8:  break;
      case VertexFormat::UInt8: break;
      // 2
      case VertexFormat::Int16:  
      case VertexFormat::UInt16: result *=2; break;
      // 4
      case VertexFormat::Int32:
      case VertexFormat::UInt32:
      case VertexFormat::Float: result *= 4; break;
      default:
      // ERROR
      return 0;
    }
    return result;
  }


  struct DisplayList::Command {
    struct Type {
      enum Enum {
        Invalid = 0,
        Clear,
        TextureData,
        BufferData,
        SetupView,
        SetupPipeline,
        FillBuffer,
        FillTexture,
        Render,
        DestroyResource,

        Scissor,

      };
    };
    static const size_t kPayloadSize = std::max<size_t>({
      sizeof(ClearData),
      sizeof(SetupViewData),
      sizeof(SetupPipelineData),
      sizeof(RenderData),
      sizeof(FillBufferData),
      sizeof(FillTextureData),
    });
    //static const size_t kPayloadSizeAligned64 =(kPayloadSize+7)>>3;
    Type::Enum type = Type::Invalid;
    uint8_t data[kPayloadSize];
    uint32_t payload_index = (uint32_t)-1;
  };

  struct DisplayList::Data {
    std::vector<Command> commands;
    std::vector<Mem<uint8_t>> payloads;
    bool last_command_patched = true;

    void reset() {
      commands.clear();
      payloads.clear();
      last_command_patched = true;
    }

    void copy(const Data &d) {
      commands = d.commands; // relay on copy from std::vector
      // since Mem<T> doesn't allow copy, we explicitly copy each
      // element. 
      payloads.resize(d.payloads.size());
      for (size_t i = 0; i < d.payloads.size(); ++i) {
        payloads[i].copy(d.payloads[i]);
      }
      last_command_patched = d.last_command_patched;
    } 
  };

  struct RenderContext::Data {
    static const uint32_t kNumStoredFrames = 4;

    struct RenderElement {
      DisplayList::Data display_list;
      std::condition_variable cv;
      std::mutex mutex;
      bool empty = true;
      bool swap = false;
    };

    bool marked_for_finish = false;
    uint32_t r_list_pos = 0;
    RenderElement list[kNumStoredFrames];
    std::atomic<uint32_t> w_list_pos = {};
    // instances ---------------------------------
    Mem<PipelineInstance>    pipelines;
    Mem<BufferInstance>      buffers;
    Mem<TextureInstance>     textures;
    Mem<FramebufferInstance> framebuffers;
    // Render State ------------------------------
    Mat4 view_matrix;
    Mat4 inv_view_matrix;
    bool inv_view_matrix_computed = false;
    Mat4 projection_matrix;
    Mat4 inv_projection_matrix;
    bool inv_projection_matrix_computed = false;
    Mat4 model_matrix;
    RenderContextParams params;
    DisplayList::SetupPipelineData last_pipeline = {};
    // Back end -----------------------------------
    BackEnd *back_end = nullptr;
  };

  static void OnError(const RenderContext::Data *ctx, const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[2048];
    std::vsnprintf(buffer, 2048, format, args);
    va_end(args);
    PX_RENDER_DEBUG_FUNC(0, "px_render ERROR --> %s\n", buffer);
    if (ctx->params.on_error_callback) {
      ctx->params.on_error_callback(buffer);
    } else {
      assert(!"FATAL ERROR");
    }
  }

  template<class T>
  static uint32_t AcquireResource(RenderContext *ctx, Mem<T> *pool) {
    uint32_t try_count = 10; //<... should never need this but...
    while (try_count--) {
      for (uint32_t i = 0; i < pool->count; ++i) {
        if ((*pool)[i].acquire()) {
          uint32_t version = (*pool)[i].version;
          uint32_t result = i | (version << 20);
          PX_RENDER_DEBUG_FUNC(100, "AcquireResource %s [%u (%u,%u)]\n", typeid(T).name(), result, i, version);
          return result;
        }
      }
    }
    // ... no luck
    OnError(ctx->data_, "Could not allocate instance of type[T]");
    return 0;
  }

  static uint32_t IDToIndex(uint32_t id) {
    return id & 0x000FFFFF;
  }

  static std::pair<uint32_t, uint32_t> IDToIndexAndVersion(uint32_t id) {
    uint32_t pos     = id & 0x000FFFFF;
    uint32_t version = (id & 0xFFF00000)>>20;
    return {pos,version};
  }

  template<class T>
  static bool CheckValidResource(const RenderContext::Data *ctx, uint32_t id, const Mem<T> *pool) {
    auto pv = IDToIndexAndVersion(id);
    uint32_t pos     = pv.first;
    uint32_t version = pv.second;
    const T* result = &(*pool)[pos];
    uint32_t real_version = (result->state.load() & 0xFFF);
    if (real_version == version) {
      return true;
    }
    PX_RENDER_DEBUG_FUNC(10,"Ivalid Resource %s [%u (%u,%u != %u)]\n", typeid(T).name(), id, pos, version, real_version);
    return false;
  }

  template<class T>
  static void CheckValidResourceOrError(const RenderContext::Data *ctx, uint32_t id, const Mem<T> *pool) {
    if (!CheckValidResource(ctx, id, pool)) {
      auto pv = IDToIndexAndVersion(id);
      OnError(ctx, "Invalid Resource Pointer (Dangling reference) ref %u --> pos %u, version %u", id, pv.first, pv.second);
    }
  }

  template<class T, class B>
  static std::pair<T*,B*> GetResource(RenderContext::Data *ctx, uint32_t id, Mem<T> *instance_array, Mem<B> *backend_array) {
    CheckValidResourceOrError(ctx, id, instance_array);
    uint32_t index = IDToIndex(id);
    return {&(*instance_array)[index], &(*backend_array)[index]};
  }

  template<class T>
  static T* GetResource(RenderContext::Data *ctx, uint32_t id, Mem<T> *instance_array) {
    CheckValidResourceOrError(ctx, id, instance_array);
    uint32_t index = IDToIndex(id);
    return &(*instance_array)[index];
  }

  static void InitBackEnd(BackEnd **b, const RenderContextParams &params);
  static void DestroyBackEnd(BackEnd **b);
  static void ExecuteDisplayList(RenderContext::Data *data, const DisplayList::Data *dl);
  static void PatchLastDisplayListCommand(DisplayList::Data *dl);

  static void SubmitDisplayList(RenderContext::Data *d, DisplayList::Data &&dl, bool swap) {
    PatchLastDisplayListCommand(&dl);
    uint32_t p = (d->w_list_pos.fetch_add(1))%RenderContext::Data::kNumStoredFrames;
    auto &re = d->list[p];
    std::unique_lock<std::mutex> lk(re.mutex);
    re.cv.wait(lk, [&re, &d]{return re.empty || d->marked_for_finish;});
    if (!d->marked_for_finish) {
      re.display_list = std::move(dl);
      re.empty = false;
      re.swap = swap;
    }
    lk.unlock();
  }

  RenderContext::RenderContext() {
    data_ = new Data();
  }

  RenderContext::~RenderContext() {
    delete data_;
    data_ = nullptr;
  }

  void RenderContext::init(const RenderContextParams &params) {
    data_->params = params;
    data_->pipelines.alloc(params.max_pipelines);
    data_->framebuffers.alloc(params.max_framebuffers);
    data_->buffers.alloc(params.max_buffers);
    data_->textures.alloc(params.max_textures);
    InitBackEnd(&data_->back_end, params);
  }

  DisplayList::DisplayList() {
    data_ = new Data();
  }

  DisplayList::~DisplayList() {
    delete data_;
    data_ = nullptr;
  }

  DisplayList DisplayList::clone() const {
    DisplayList copy;
    copy.data_->copy(*data_);
    return copy;
  }

  Texture RenderContext::createTexture(const Texture::Info &info) {
    if (info.format == TexelsFormat::None) return Texture();

    uint32_t id = AcquireResource(this, &data_->textures);
    uint32_t pos = IDToIndex(id);
    TextureInstance &i_obj = data_->textures.data[pos];
    i_obj.info = info;

    switch(info.format) {
      case TexelsFormat::R_U8:
        i_obj.bytes_per_pixel = 1;
        break;
      case TexelsFormat::RG_U8:
        i_obj.bytes_per_pixel = 2;
        break;
      case TexelsFormat::RGB_U8:
        i_obj.bytes_per_pixel = 3;
        break;
      case TexelsFormat::RGBA_U8:
        i_obj.bytes_per_pixel = 4;
        break;
      case TexelsFormat::Depth_U16:
        i_obj.bytes_per_pixel = 2;
        break;
      case TexelsFormat::DepthStencil_U16:
        i_obj.bytes_per_pixel = 4;
        break;
    }

    return Texture{this, id};
  }

  Buffer RenderContext::createBuffer(const Buffer::Info &info) {
    uint32_t id = AcquireResource(this, &data_->buffers);
    uint32_t pos = IDToIndex(id);
    BufferInstance &i_obj = data_->buffers.data[pos];
    i_obj.info = info;
    return Buffer{this,id};
  }

  Pipeline RenderContext::createPipeline(const Pipeline::Info &info) {
    uint32_t id = AcquireResource(this, &data_->pipelines);
    uint32_t pos =  IDToIndex(id);
    PipelineInstance &i_obj = data_->pipelines.data[pos];
    i_obj.info = info;
    i_obj.fragment_shader.copy(info.shader.fragment, std::strlen(info.shader.fragment)+1);
    i_obj.vertex_shader.copy(info.shader.vertex, std::strlen(info.shader.vertex)+1);
    // make a copy of all strings involved
    i_obj.info.shader.fragment = i_obj.fragment_shader.data.get();
    i_obj.info.shader.vertex = i_obj.vertex_shader.data.get();
    // also compute strides/offsets
    size_t strides_by_buffer_index[kMaxVertexAttribs] = {};
    for (auto i = 0; i < kMaxVertexAttribs; ++i) {
      const char *name = info.attribs[i].name;
      if (name) {
        i_obj.attributes[i].copy(name, strlen(name)+1);
        i_obj.info.attribs[i].name = i_obj.attributes[i].data.get();
      }
      if (!i_obj.info.attribs[i].offset) {
        i_obj.info.attribs[i].offset = strides_by_buffer_index[i_obj.info.attribs[i].buffer_index];
      }
      strides_by_buffer_index[i_obj.info.attribs[i].buffer_index] += ComputeVertexSize(i_obj.info.attribs[i].format);
    }

    // set stride to all
    for (auto i = 0; i < kMaxVertexAttribs; ++i) {
      auto &a = i_obj.info.attribs[i];
      if (!a.stride) a.stride = strides_by_buffer_index[a.buffer_index];
    }
    
    return Pipeline{this, id};
  }

  Framebuffer RenderContext::createFramebuffer(const Framebuffer::Info &info) {
    uint32_t id = AcquireResource(this, &data_->framebuffers);
    uint32_t pos = IDToIndex(id);
    FramebufferInstance &i_obj = data_->framebuffers.data[pos];
    i_obj.info = info;
    i_obj.color_texture.alloc(info.num_color_textures);
    for (uint32_t i = 0; i < info.num_color_textures; ++i) {
      i_obj.color_texture[i] = createTexture(info.color_texture_info);
    }
    i_obj.depth_texture = createTexture(info.depth_stencil_texture_info);
    
    return Framebuffer{this,id};
  }

  Texture Framebuffer::color_texture(uint16_t p) const {
    if (!ctx) {
      return Texture();
    }
    CheckValidResourceOrError(ctx->data_, id, &ctx->data_->framebuffers);
    uint32_t pos = IDToIndex(id);
    return ctx->data_->framebuffers[pos].color_texture[p];
  }

  RenderContext::Result::Enum RenderContext::executeOnGPU() {
    if (data_->marked_for_finish) {
      return Result::Finished;
    }
    uint32_t p = data_->r_list_pos%RenderContext::Data::kNumStoredFrames;
    auto &re = data_->list[p];
    std::unique_lock<std::mutex> lk(re.mutex);
    bool with_swap = false;
    if (!re.empty) {
      ExecuteDisplayList(data_, &re.display_list);
      re.empty = true;
      data_->r_list_pos++;
      with_swap = re.swap;
    }
    lk.unlock();
    re.cv.notify_one();
    return with_swap?Result::OK_Swap:Result::OK;
  }

  void RenderContext::submitDisplayList(DisplayList &&dl) {
    SubmitDisplayList(data_, std::move(*dl.data_), false);
  }
  void RenderContext::submitDisplayListAndSwap(DisplayList &&dl) {
    SubmitDisplayList(data_, std::move(*dl.data_), true);
  }

  void RenderContext::finish() {
    data_->marked_for_finish = true;
    for (size_t i = 0; i < Data::kNumStoredFrames; ++i) {
      data_->list[i].cv.notify_all();
    }
  }

  // Here we copy last commands.raw pointer to internal payload
  static void PatchLastDisplayListCommand(DisplayList::Data *dl) {
    if (!dl->commands.empty() && !dl->last_command_patched) {
      dl->last_command_patched = true;
      auto &last = dl->commands.back();
      switch (last.type) {
        case DisplayList::Command::Type::SetupPipeline: {
          auto c = reinterpret_cast<DisplayList::SetupPipelineData*>(last.data);
          if (c->uniforms) {
            RenderContext::Data *ctx = c->pipeline.ctx->data_;
            auto pi = GetResource(ctx, c->pipeline.id, &ctx->pipelines);
            Mem<uint8_t> mem;
            mem.copy(c->uniforms, pi->info.uniform_size);
            c->uniforms = nullptr;
            last.payload_index = dl->payloads.size();
            dl->payloads.push_back(std::move(mem));
          }
        }break;
        case DisplayList::Command::Type::FillBuffer: {
          auto c = reinterpret_cast<DisplayList::FillBufferData*>(last.data);
          if (c->data) {
            Mem<uint8_t> mem;
            mem.copy((const uint8_t*)c->data, c->size);
            c->data = nullptr;
            last.payload_index = dl->payloads.size();
            dl->payloads.push_back(std::move(mem));
          }
        } break;
        case DisplayList::Command::Type::FillTexture: {
          auto c = reinterpret_cast<DisplayList::FillTextureData*>(last.data);
          RenderContext::Data *ctx = c->texture.ctx->data_;
          auto ti = GetResource(ctx, c->texture.id, &ctx->textures);
          c->width  = c->width ?c->width :ti->info.width;
          c->height = c->height?c->height:ti->info.height;
          c->depth  = c->depth ?c->depth :ti->info.depth;
          if (c->data) {
            Mem<uint8_t> mem;
            mem.copy((const uint8_t*)c->data, ti->bytes_per_pixel*c->width*c->height*c->depth);
            c->data = nullptr;
            last.payload_index = dl->payloads.size();
            dl->payloads.push_back(std::move(mem));
          }
        } break;
        default: break;/* nothing to do*/
      }
    }
  }

  void DisplayList::reset() {
    data_->reset();
  }

  void DisplayList::commitLastCommand() {
    PatchLastDisplayListCommand(data_);
  }

  template<class T>
  static T* EmplaceCommand(DisplayList::Data *dl, DisplayList::Command::Type::Enum t) {
    PatchLastDisplayListCommand(dl);
    DisplayList::Command c = {};
    c.type = t;
    *reinterpret_cast<T*>(c.data) = T(); // initialization
    dl->commands.push_back(std::move(c));
    dl->last_command_patched = false;
    return reinterpret_cast<T*>(dl->commands.back().data);
  }

  #define COMMAND(name, NAME) \
    DisplayList::NAME##Data& DisplayList::name ## Command() { \
      return *EmplaceCommand<NAME##Data>(data_, Command::Type::NAME); \
    } \
    static void Execute(RenderContext::Data *, const DisplayList::NAME ## Data &, const Mem<uint8_t> *);

  COMMAND(clear, Clear)
  COMMAND(setupView, SetupView)
  COMMAND(setupPipeline, SetupPipeline)
  COMMAND(render, Render)
  COMMAND(fillBuffer, FillBuffer)
  COMMAND(fillTexture, FillTexture)
  #undef COMMAND

  struct DestroyCommand {
    GPUResource resource;
  };

  static void DestroyBackEndResource(RenderContext::Data *ctx, const GPUResource::Type::Enum type, uint32_t pos);

  static void Execute(RenderContext::Data *ctx, const DestroyCommand &dc) {
    if (dc.resource.ctx == nullptr && dc.resource.id == 0) return; // invalid resource nothing needs to be done
    if (ctx != dc.resource.ctx->data_ ) OnError(ctx, "RenderContext mismatch");

    uint32_t pos = IDToIndex(dc.resource.id);
    PX_RENDER_DEBUG_FUNC(100,"Destroy Resource %u [%u -> %u)]\n", dc.resource.type, dc.resource.id, pos);
    DestroyBackEndResource(ctx, dc.resource.type, pos);
    switch (dc.resource.type) {
      case GPUResource::Type::Invalid:
        OnError(ctx, "Trying to destroy an invalid resource");
        break;
      case GPUResource::Type::Buffer:
        ctx->buffers[pos].state = 0;
        break;
      case GPUResource::Type::Pipeline:
        ctx->pipelines[pos].state = 0;
        break;
      case GPUResource::Type::Texture:
        ctx->textures[pos].state = 0;
        break;
      case GPUResource::Type::Framebuffer:
        {
          FramebufferInstance *fb = &ctx->framebuffers[pos];

          for (uint32_t i = 0; i < fb->color_texture.count; ++i) {
            uint32_t t_pos = IDToIndex(fb->color_texture[i].id);
            DestroyBackEndResource(ctx, GPUResource::Type::Texture, t_pos);
            ctx->textures[t_pos].state = 0;
          }
          if (CheckValidResource(ctx, fb->depth_texture.id, &ctx->textures)) {
            uint32_t t_pos = IDToIndex(fb->depth_texture.id);
            DestroyBackEndResource(ctx, GPUResource::Type::Texture, t_pos);
            ctx->textures[t_pos].state = 0;
          }
		  
          fb->state = 0;
		  
        } break;
      default:
        OnError(ctx, "Invalid Resource to destroy");
    }
  }

  #define DESTROY(NAME) \
  DisplayList& DisplayList::destroy(const NAME &b) { \
    EmplaceCommand<DestroyCommand>(data_, Command::Type::DestroyResource)->resource = b; \
    return *this; \
  }

  DESTROY(Buffer)
  DESTROY(Pipeline)
  DESTROY(Texture)
  DESTROY(Framebuffer)
  #undef DESTROY

  static void StartDisplayList(RenderContext::Data *ctx);
  static void EndDisplayList(RenderContext::Data *ctx);

  void ExecuteDisplayList(RenderContext::Data *ctx, const DisplayList::Data *dl) {
    StartDisplayList(ctx);
    size_t payloads_count = dl->payloads.size();
    PX_RENDER_DEBUG_FUNC(1000,"Execute DL (Commands %u) (Payloads %u)\n", dl->commands.size(), payloads_count);
    for (auto &c : dl->commands) {
      const Mem<uint8_t> *payloads= nullptr;
      if (c.payload_index < payloads_count) {
        payloads= &dl->payloads[c.payload_index];
      }
      switch (c.type) {
      #define C(Name) \
        case DisplayList::Command::Type::Name: \
          PX_RENDER_DEBUG_FUNC(1500, "  Command " #Name "\n");\
          Execute(ctx, *reinterpret_cast<const DisplayList::Name##Data*>(c.data), payloads);\
          break;
        C(Clear);
        C(SetupView);
        C(SetupPipeline);
        C(Render);
        C(FillBuffer);
        C(FillTexture);
      #undef C
        case DisplayList::Command::Type::DestroyResource:
          PX_RENDER_DEBUG_FUNC(1500, "  Command Destroy\n");\
          Execute(ctx, *reinterpret_cast<const DestroyCommand*>(c.data));
          break;
      }
    }
    EndDisplayList(ctx);
  }

// Some Helper functions, thanks MESA! ---------------------------------------
// https://cgit.freedesktop.org/mesa/mesa/tree/src/mesa/math/m_matrix.c


  static void mat4_mult(const float *a, const float *b, float *product) {
    #define A(row,col)  a[(col<<2)+row]
    #define B(row,col)  b[(col<<2)+row]
    #define P(row,col)  product[(col<<2)+row]
    for (int i = 0; i < 4; i++) {
      const float ai0 = A(i, 0), ai1 = A(i, 1), ai2 = A(i, 2), ai3 = A(i, 3);
      P(i, 0) = ai0 * B(0, 0) + ai1 * B(1, 0) + ai2 * B(2, 0) + ai3 * B(3, 0);
      P(i, 1) = ai0 * B(0, 1) + ai1 * B(1, 1) + ai2 * B(2, 1) + ai3 * B(3, 1);
      P(i, 2) = ai0 * B(0, 2) + ai1 * B(1, 2) + ai2 * B(2, 2) + ai3 * B(3, 2);
      P(i, 3) = ai0 * B(0, 3) + ai1 * B(1, 3) + ai2 * B(2, 3) + ai3 * B(3, 3);
    }
    #undef A
    #undef B
    #undef P
  }

  static bool mat4_inverse(const float *m, float *out) {
    #define MAT(m,r,c) (m)[(c)*4+(r)]
    #define SWAP_ROWS(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }

    float wtmp[4][8];
    float m0, m1, m2, m3, s;
    float *r0, *r1, *r2, *r3;

    r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

    r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
    r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
    r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

    r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
    r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
    r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

    r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
    r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
    r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

    r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
    r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
    r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

    /* choose pivot - or die */
    if (fabs(r3[0])>fabs(r2[0])) SWAP_ROWS(r3, r2);
    if (fabs(r2[0])>fabs(r1[0])) SWAP_ROWS(r2, r1);
    if (fabs(r1[0])>fabs(r0[0])) SWAP_ROWS(r1, r0);
    if (0.0F == r0[0]) return false;

    /* eliminate first variable     */
    m1 = r1[0] / r0[0]; m2 = r2[0] / r0[0]; m3 = r3[0] / r0[0];
    s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
    s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
    s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0.0F) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
    s = r0[5];
    if (s != 0.0F) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
    s = r0[6];
    if (s != 0.0F) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
    s = r0[7];
    if (s != 0.0F) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

    /* choose pivot - or die */
    if (fabs(r3[1])>fabs(r2[1])) SWAP_ROWS(r3, r2);
    if (fabs(r2[1])>fabs(r1[1])) SWAP_ROWS(r2, r1);
    if (0.0F == r1[1]) return false;

    /* eliminate second variable */
    m2 = r2[1] / r1[1]; m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
    s = r1[4]; if (0.0F != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
    s = r1[5]; if (0.0F != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
    s = r1[6]; if (0.0F != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
    s = r1[7]; if (0.0F != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

    /* choose pivot - or die */
    if (fabs(r3[2])>fabs(r2[2])) SWAP_ROWS(r3, r2);
    if (0.0F == r2[2]) return false;

    /* eliminate third variable */
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
    r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
    r3[7] -= m3 * r2[7];

    /* last check */
    if (0.0F == r3[3]) return false;

    s = 1.0F / r3[3];             /* now back substitute row 3 */
    r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

    m2 = r2[3];                 /* now back substitute row 2 */
    s = 1.0F / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
    r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
    r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
    r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

    m1 = r1[2];                 /* now back substitute row 1 */
    s = 1.0F / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
    r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
    r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

    m0 = r0[1];                 /* now back substitute row 0 */
    s = 1.0F / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
    r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

    MAT(out, 0, 0) = r0[4]; MAT(out, 0, 1) = r0[5],
    MAT(out, 0, 2) = r0[6]; MAT(out, 0, 3) = r0[7],
    MAT(out, 1, 0) = r1[4]; MAT(out, 1, 1) = r1[5],
    MAT(out, 1, 2) = r1[6]; MAT(out, 1, 3) = r1[7],
    MAT(out, 2, 0) = r2[4]; MAT(out, 2, 1) = r2[5],
    MAT(out, 2, 2) = r2[6]; MAT(out, 2, 3) = r2[7],
    MAT(out, 3, 0) = r3[4]; MAT(out, 3, 1) = r3[5],
    MAT(out, 3, 2) = r3[6]; MAT(out, 3, 3) = r3[7];

    #undef MAT
    #undef SWAP_ROWS
    return true;
  }

  void mat4_transpose(const float from[16], float to[16]) {
    to[0]  = from[0]; to[1]  = from[4]; to[2]  = from[8];  to[3]  = from[12];
    to[4]  = from[1]; to[5]  = from[5]; to[6]  = from[9];  to[7]  = from[13];
    to[8]  = from[2]; to[9]  = from[6]; to[10] = from[10]; to[11] = from[14];
    to[12] = from[3]; to[13] = from[7]; to[14] = from[11]; to[15] = from[15];
  }


  Mat4 Mat4::Mult(const Mat4 &a, const Mat4 &b) {
    Mat4 result;
    mat4_mult(a.f, b.f, result.f);
    return result;
  }

  Vec4 Mat4::Mult(const Mat4 &a, const Vec4 &b) {
    Vec4 result;
    result.f[0] = a.column[0].f[0]*b.f[0] + a.column[1].f[0]*b.f[1] + a.column[2].f[0]*b.f[2] + a.column[3].f[0]*b.f[3];
    result.f[1] = a.column[0].f[1]*b.f[0] + a.column[1].f[1]*b.f[1] + a.column[2].f[1]*b.f[2] + a.column[3].f[1]*b.f[3];
    result.f[2] = a.column[0].f[2]*b.f[0] + a.column[1].f[2]*b.f[1] + a.column[2].f[2]*b.f[2] + a.column[3].f[2]*b.f[3];
    result.f[3] = a.column[0].f[3]*b.f[0] + a.column[1].f[3]*b.f[1] + a.column[2].f[3]*b.f[2] + a.column[3].f[3]*b.f[3];
    return result;
  }

  Mat4 Mat4::Inverse(const Mat4 &m) {
    Mat4 result;
    mat4_inverse(m.f,result.f);
    return result;
  }

  Mat4 Mat4::Transpose(const Mat4 &m) {
    Mat4 result;
    mat4_transpose(m.f,result.f);
    return result;
  }

  Mat4 Mat4::SRT(const Vec3 &scale, const Vec4 &rotate_axis_angle, const Vec3 &translate) {
    float c = cosf(rotate_axis_angle.v.w);
    float s = sinf(rotate_axis_angle.v.w);
    float ci = 1.0f -c;
    float inv_axis_d = 1.0f/sqrtf(
        rotate_axis_angle.v.x*rotate_axis_angle.v.x +
        rotate_axis_angle.v.y*rotate_axis_angle.v.y +
        rotate_axis_angle.v.z*rotate_axis_angle.v.z);
    float x = rotate_axis_angle.v.x*inv_axis_d;
    float y = rotate_axis_angle.v.y*inv_axis_d;
    float z = rotate_axis_angle.v.z*inv_axis_d;
    return Mat4 {
      scale.v.x*(x*x*ci+c),
      scale.v.x*(y*x*ci+z*s),
      scale.v.x*(x*z*ci-y*s),
      0,

      scale.v.y*(x*y*ci -z*s),
      scale.v.y*(y*y*ci + c),
      scale.v.y*(y*z*ci + x*s),
      0,

      scale.v.z*(x*z*ci+y*s),
      scale.v.z*(y*z*ci-x*s),
      scale.v.z*(z*z*ci + c),
      0, 

      translate.v.x,
      translate.v.y,
      translate.v.z,
      1.0f
    };

  }

  static void ComputeModel(RenderContext::Data *ctx, float *output, uint8_t *range) {
    memcpy(output, ctx->model_matrix.f, sizeof(float)*16);
    *range = 4;
  }

  static void ComputeInvModel(RenderContext::Data *ctx, float *output, uint8_t *range) {
    mat4_inverse(ctx->model_matrix.f, output);
    *range = 4;
  }


  static void ComputeNormal(RenderContext::Data *ctx, float *output, uint8_t *range) {
    float inv_model[16];
    uint8_t tmp;
    ComputeInvModel(ctx, inv_model, &tmp);
    float trans[16];
    mat4_transpose(inv_model, trans);
    output[0] = trans[0];
    output[1] = trans[1];
    output[2] = trans[2];
    output[3] = trans[4];
    output[4] = trans[5];
    output[5] = trans[8];
    output[6] = trans[9];
    output[7] = trans[10];
    output[8] = trans[11];
    *range = 3;
  }

  static void ComputeView(RenderContext::Data *ctx, float *output, uint8_t *range) {
    memcpy(output, ctx->view_matrix.f, sizeof(float)*16);
    *range = 4;
  }

  static void ComputeProjection(RenderContext::Data *ctx, float *output, uint8_t *range) {
    memcpy(output, ctx->projection_matrix.f, sizeof(float)*16);
    *range = 4;
  }

  static void ComputeModelView(RenderContext::Data *ctx, float *output, uint8_t *range) {
    mat4_mult(ctx->view_matrix.f, ctx->model_matrix.f, output);
    *range = 4;
  }

  static void ComputeModelViewProjection(RenderContext::Data *ctx, float *output, uint8_t *range) {
    float tmp[16];
    mat4_mult(ctx->view_matrix.f, ctx->model_matrix.f, tmp);
    mat4_mult(ctx->projection_matrix.f, tmp, output);
    *range = 4;
  }

  static void ComputeViewProjection(RenderContext::Data *ctx, float *output, uint8_t *range) {
    mat4_mult(ctx->projection_matrix.f, ctx->view_matrix.f, output);
    *range = 4;
  }

  static void ComputeInvView(RenderContext::Data *ctx, float *output, uint8_t *range) {
    mat4_inverse(ctx->view_matrix.f, output);
  }

  static void ComputeInvProjection(RenderContext::Data *ctx, float *output, uint8_t *range) {
    mat4_inverse(ctx->projection_matrix.f, output);
  }

  static void ComputeInvModelView(RenderContext::Data *ctx, float *output, uint8_t *range) {
    float tmp[16];
    ComputeModelView(ctx, tmp, range);
    mat4_inverse(tmp, output);
  }

  static void ComputeInvModelViewProjection(RenderContext::Data *ctx, float *output, uint8_t *range) {
    float tmp[16];
    ComputeModelViewProjection(ctx, tmp, range);
    mat4_inverse(tmp, output);
  }

  static void ComputeInvViewProjection(RenderContext::Data *ctx, float *output, uint8_t *range) {
    float tmp[16];
    ComputeViewProjection(ctx, tmp, range);
    mat4_inverse(tmp, output);
  }

  struct UniformData {
    const char *name;
    void (*compute)(RenderContext::Data *ctx, float *output, uint8_t *range);
  };

  // Automatic handled uniforms....
  static const size_t kAutomaticUniformCount = 14;

  static UniformData Uniforms[kAutomaticUniformCount] = {
    // -- depends on the instance/model ------------
    {"u_data", nullptr}, //> Reserved for the USER (array of vec4)
    {"u_normal",ComputeNormal},
    {"u_model",ComputeModel},
    {"u_modelView",ComputeModelView},
    {"u_modelViewProjection",ComputeModelViewProjection},
    {"u_invModel",ComputeInvModel},
    {"u_invModelView",ComputeInvModelView},
    {"u_invModelViewProjection",ComputeInvModelViewProjection},
    //-- independent from the instance/model -------
    {"u_view",ComputeView},
    {"u_projection",ComputeProjection},
    {"u_viewProjection",ComputeViewProjection},
    {"u_invView",ComputeInvView},
    {"u_invProjection",ComputeInvProjection},
    {"u_invViewProjection",ComputeInvViewProjection},
  };


// BackEnd Implementation ----------------------------------------------------

#ifdef PX_RENDER_BACKEND_GLES
  #define glClearDepth glClearDepthf
#endif
  
  struct BackEnd {
    struct Pipeline {
      GLuint program = 0;
      GLint uniforms_location[kAutomaticUniformCount] = {};
      GLint texture_uniforms_location[kMaxTextureUnits] = {};
      GLint attrib_location[kMaxVertexAttribs] = {};

      bool needs_vertex_buffer_bind = false;
    };

    struct Buffer {
      GLuint buffer = 0;
    };
    
    struct Texture {
      GLuint texture = 0;
      GLenum format = 0;
      GLenum internal_format = 0;
      GLenum type = 0;
      GLenum target = 0;
    };

    struct Framebuffer {
      GLuint framebuffer = 0;
    };

    GLuint vao = 0;

    Mem<Pipeline>    pipelines;
    Mem<Buffer>      buffers;
    Mem<Texture>     textures;
    Mem<Framebuffer> framebuffers;
  };


  static void InitBackEnd(BackEnd **b, const RenderContextParams &params) {
    *b = new BackEnd();
    (*b)->pipelines.alloc(params.max_pipelines);
    (*b)->buffers.alloc(params.max_buffers);
    (*b)->textures.alloc(params.max_textures);
    (*b)->framebuffers.alloc(params.max_framebuffers);
  }

  static void DestroyBackEnd(BackEnd **b) {
    delete *b;
    *b = nullptr;
  }

  static void CheckGLError(RenderContext::Data *ctx, const char* operation) {
    int32_t error = glGetError();
    if (error) {
      OnError(ctx, "OpenGL Error: 0x%x (%s)", error, operation);
      return;
    }
  }
  #define GLCHECK_STR_STR(A) #A
  #define GLCHECK_STR(A) GLCHECK_STR_STR(A)
  #define GLCHECK(...) {__VA_ARGS__; CheckGLError(ctx, __FILE__  ":" GLCHECK_STR(__LINE__) "-->" #__VA_ARGS__);}

  static void Execute(RenderContext::Data *ctx,const DisplayList::ClearData &d, const Mem<uint8_t>*) {
    
    uint32_t mask = 0;
    if (d.clear_color) {
      GLCHECK(glClearColor(
        d.color.c.r,
        d.color.c.g,
        d.color.c.b,
        d.color.c.a));
      mask |= GL_COLOR_BUFFER_BIT;
      GLCHECK(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    }
    if (d.clear_depth) {
      GLCHECK(glClearDepth(d.depth));
      mask |= GL_DEPTH_BUFFER_BIT;
      GLCHECK(glDepthMask(GL_TRUE));
    }
    if (d.clear_stencil) {
      GLCHECK(glClearStencil(d.stencil));
      mask |= GL_STENCIL_BUFFER_BIT;
    }
    GLCHECK(glClear(mask));
  }
  
  static uint32_t CompileShader(RenderContext::Data *ctx, GLenum type, const char *src) {
    uint32_t shader = glCreateShader(type);
    if (!shader) {
      OnError(ctx, "OpenGL: Could not create Shader");
      return 0;
    }
    if (shader) {
      int32_t compiled = 0;

      GLCHECK(glShaderSource(shader, 1, &src, NULL));
      GLCHECK(glCompileShader(shader));
      GLCHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));

      if (!compiled) {
        static const size_t max_length = 2048;
        char buffer[max_length];
        GLCHECK(glGetShaderInfoLog(shader, max_length, NULL, buffer));
        OnError(ctx, "Error Compiling Shader(%d):%s\nCODE:\n%.256s\n", type, buffer, src);
        GLCHECK(glDeleteShader(shader));
        return 0;
      }
    }
    return shader;
  }

  static GLenum Translate(BlendFactor::Enum e) {
    switch (e) {
      case BlendFactor::Zero: return GL_ZERO;
      case BlendFactor::One: return GL_ONE;
      case BlendFactor::SrcColor: return GL_SRC_COLOR;
      case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
      case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
      case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
      case BlendFactor::DstColor: return GL_DST_COLOR;
      case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
      case BlendFactor::DstAlpha: return GL_DST_ALPHA;
      case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
      case BlendFactor::SrcAlphaSaturated: return GL_SRC_ALPHA_SATURATE;
      case BlendFactor::BlendColor: return GL_BLEND_COLOR;
      case BlendFactor::OneMinusBlendColor: return GL_ONE_MINUS_CONSTANT_COLOR;
      case BlendFactor::BlendAlpha: return GL_CONSTANT_ALPHA;
      case BlendFactor::OneMinusBlendAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
    // ERROR!
    assert(!"Invalid BlendFactor");
    return 0;
  }

  static GLenum Translate(BlendOp::Enum e) {
    switch (e) {
      case BlendOp::Add: return GL_FUNC_ADD;
      case BlendOp::Substract: return GL_FUNC_SUBTRACT;
      case BlendOp::ReverseSubstract: return GL_FUNC_REVERSE_SUBTRACT;
      case BlendOp::Min: return GL_MIN;
      case BlendOp::Max: return GL_MAX;
    }
    // ERROR!
    assert(!"Invalid BlendOp");
    return 0;
  }

  static GLenum Translate(IndexFormat::Enum e) {
    switch (e) {
      case IndexFormat::UInt8:  return GL_UNSIGNED_BYTE;
      case IndexFormat::UInt16: return GL_UNSIGNED_SHORT;
      case IndexFormat::UInt32: return GL_UNSIGNED_INT;
    }
    // ERROR!
    assert(!"Invalid IndexFormat");
    return 0;
  }

  static GLenum Translate(Primitive::Enum e) {
    switch (e) {
      case Primitive::Lines:     return GL_LINES;
      case Primitive::Triangles: return GL_TRIANGLES;
      case Primitive::Points:    return GL_POINTS;
    }
    // ERROR!
    assert(!"Invalid Primitive");
    return 0;
  }

  static GLenum Translate(Usage::Enum e) {
    switch (e) {
      case Usage::Static:  return GL_STATIC_DRAW;
      case Usage::Dynamic: return GL_DYNAMIC_DRAW;
      case Usage::Stream:  return GL_STREAM_DRAW;
    }
    // ERROR!
    assert(!"Invalid Usage");
    return 0;
  }

  static GLenum Translate(CompareFunc::Enum e) {
    switch (e) {
      case CompareFunc::Never:        return GL_NEVER;
      case CompareFunc::Less:         return GL_LESS;
      case CompareFunc::LessEqual:    return GL_LEQUAL;
      case CompareFunc::Equal:        return GL_EQUAL;
      case CompareFunc::NotEqual:     return GL_NOTEQUAL;
      case CompareFunc::GreaterEqual: return GL_GEQUAL;
      case CompareFunc::Greater:      return GL_GREATER;
      case CompareFunc::Always:       return GL_ALWAYS;
      }
    // ERROR!
    assert(!"Invalid CompareFunc");
    return 0;
  }

  static GLenum Translate(SamplerFiltering::Enum e) {
    switch (e) {
      case SamplerFiltering::Nearest:              return GL_NEAREST;
      case SamplerFiltering::Linear:               return GL_LINEAR;
      case SamplerFiltering::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
      case SamplerFiltering::NearestMipmapLinear:  return GL_NEAREST_MIPMAP_LINEAR;
      case SamplerFiltering::LinearMipmapNearest:  return GL_LINEAR_MIPMAP_NEAREST;
      case SamplerFiltering::LinearMipmapLinear:   return GL_LINEAR_MIPMAP_LINEAR;
    }
    // ERROR !
    assert(!"Invalid Sampler Filtering");
    return 0;
  }

  static GLenum Translate(SamplerWrapping::Enum e) {
    switch (e) {
      case SamplerWrapping::Repeat:         return GL_REPEAT;
      case SamplerWrapping::MirroredRepeat: return GL_MIRRORED_REPEAT;
      case SamplerWrapping::Clamp:          return GL_CLAMP_TO_EDGE;
    }
    // ERROR !
    assert(!"Invalid Sampler Wrapping");
    return 0;
  }

  static GLenum TranslateVertexType(uint32_t format) {
    format = format & VertexFormat::TypeMask;
    switch (format) {
      case VertexFormat::Float:  return GL_FLOAT; break;
      case VertexFormat::Int8:   return GL_BYTE; break;
      case VertexFormat::UInt8:  return GL_UNSIGNED_BYTE; break;
      case VertexFormat::Int16:  return GL_SHORT; break;
      case VertexFormat::UInt16: return GL_UNSIGNED_SHORT; break;
      case VertexFormat::Int32:  return GL_INT; break;
      case VertexFormat::UInt32: return GL_UNSIGNED_INT; break;
      default:
      // ERROR
    assert(!"Invalid Vertex Type");
      return 0;
    }
  }

  static void StartDisplayList(RenderContext::Data *ctx) {
    if (!ctx->back_end->vao) {
     GLCHECK(glGenVertexArrays(1, &ctx->back_end->vao));
    }
    GLCHECK(glBindVertexArray(ctx->back_end->vao));
  }

  static void EndDisplayList(RenderContext::Data *ctx) {
    ctx->last_pipeline = {};
  }

  static void Execute(RenderContext::Data *ctx, const DisplayList::FillBufferData &d, const Mem<uint8_t> *payloads) {
    auto b = GetResource(ctx, d.buffer.id, &ctx->buffers, &ctx->back_end->buffers);
    GLuint id = b.second->buffer;
    GLenum target = 0;
    if (d.offset + payloads->count > b.first->info.size) {
      OnError(ctx, "Invalid Fillbuffer, override. Size %u, offset %u, data_size %u", b.first->info.size, d.offset, payloads->count);
      return;
     }
    switch(b.first->info.type) {
      case BufferType::Vertex: target = GL_ARRAY_BUFFER; break;
      case BufferType::Index:  target = GL_ELEMENT_ARRAY_BUFFER; break;
      default: OnError(ctx, "Invalid buffer type"); break;
    }
    if (!id) {
      GLCHECK(glGenBuffers(1, &id));
      GLCHECK(glBindBuffer(target, id));
      GLCHECK(glBufferData(target, b.first->info.size, nullptr, Translate(b.first->info.usage)));
      b.second->buffer = id;
    }
    if (payloads) {
      GLCHECK(glBindBuffer(target, id));
      GLCHECK(glBufferSubData(target, d.offset, payloads->count, payloads->data.get()));
    }
  }

  static void TextureInitialization(RenderContext::Data *ctx, GLenum target, const Texture::Info &info) {
    GLCHECK(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, Translate(info.magnification_filter)));
    GLCHECK(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, Translate(info.minification_filter)));
    GLCHECK(glTexParameteri(target, GL_TEXTURE_WRAP_S, Translate(info.wrapping[0])));
    if (target > TextureType::T1D) GLCHECK(glTexParameteri(target, GL_TEXTURE_WRAP_T, Translate(info.wrapping[1])));
    if (target > TextureType::T2D) GLCHECK(glTexParameteri(target, GL_TEXTURE_WRAP_R, Translate(info.wrapping[2])));
  }

  static std::pair<TextureInstance*, BackEnd::Texture*> InitTexture(RenderContext::Data *ctx, const Texture tex) {
    auto t = GetResource(ctx, tex.id, &ctx->textures, &ctx->back_end->textures);
    GLuint id = t.second->texture;
    auto& back_end = t.second;
    bool initialize = false;
    if (!id) {
      GLCHECK(glGenTextures(1, &id));
      initialize = true;
      back_end->texture = id;
      switch (t.first->info.format) {
        case TexelsFormat::R_U8:
          back_end->format = GL_RED;
          back_end->internal_format = GL_R8;
          back_end->type = GL_UNSIGNED_BYTE;
          break;
        case TexelsFormat::RG_U8:
          back_end->format = GL_RG;
          back_end->internal_format = GL_RG8;
          back_end->type = GL_UNSIGNED_BYTE;
          break;
        case TexelsFormat::RGB_U8:
          back_end->format = GL_RGB;
          back_end->internal_format = GL_RGB8;
          back_end->type = GL_UNSIGNED_BYTE;
          break;
        case TexelsFormat::RGBA_U8:
          back_end->format = GL_RGBA;
          back_end->internal_format = GL_RGBA8;
          back_end->type = GL_UNSIGNED_BYTE;
          break;
        case TexelsFormat::Depth_U16:
          back_end->format = GL_DEPTH_COMPONENT;
          back_end->internal_format = GL_DEPTH_COMPONENT16;
          back_end->type = GL_UNSIGNED_SHORT;
          break;
        case TexelsFormat::DepthStencil_U16:
          back_end->format = GL_DEPTH_STENCIL;
          back_end->internal_format = GL_DEPTH24_STENCIL8;
          back_end->type = GL_UNSIGNED_SHORT;
          break;
        case TexelsFormat::Depth_U24:
          back_end->format = GL_DEPTH_COMPONENT;
          back_end->internal_format = GL_DEPTH_COMPONENT24;
          back_end->type = GL_UNSIGNED_INT;
          break;
        case TexelsFormat::DepthStencil_U24:
          back_end->format = GL_DEPTH_STENCIL;
          back_end->internal_format = GL_DEPTH24_STENCIL8;
          back_end->type = GL_UNSIGNED_INT;
          break;
      }
      switch (t.first->info.type) {
        #ifdef PX_RENDER_BACKEND_GL
        case TextureType::T1D:
          t.second->target = GL_TEXTURE_1D;
          GLCHECK(glBindTexture(GL_TEXTURE_1D, id));
          GLCHECK(glTexImage1D(GL_TEXTURE_1D, 0, back_end->internal_format, t.first->info.width, 0, back_end->format, back_end->type, nullptr));
          TextureInitialization(ctx, GL_TEXTURE_1D, t.first->info);
        #else
          OnError(ctx, "Texture1D not supported");
        #endif
          break;
        case TextureType::T2D:
          t.second->target = GL_TEXTURE_2D;
          GLCHECK(glBindTexture(GL_TEXTURE_2D, id));
          GLCHECK(glTexImage2D(GL_TEXTURE_2D, 0, back_end->internal_format, t.first->info.width, t.first->info.height, 0, back_end->format, back_end->type, nullptr));
          TextureInitialization(ctx, GL_TEXTURE_2D, t.first->info);
          break;
        case TextureType::T3D:
          t.second->target = GL_TEXTURE_3D;
          GLCHECK(glBindTexture(GL_TEXTURE_3D, id));
          GLCHECK(glTexImage3D(GL_TEXTURE_3D, 0, back_end->internal_format, t.first->info.width, t.first->info.height, t.first->info.depth, 0, back_end->format, back_end->type, nullptr));
          TextureInitialization(ctx, GL_TEXTURE_3D, t.first->info);
          break;
        case TextureType::CubeMap:
          OnError(ctx,"CubeMap, not implemented yet");
          break;
      }
    }
    return t;
  }

  static void Execute(RenderContext::Data *ctx, const DisplayList::FillTextureData &d, const Mem<uint8_t> *payloads) {
    auto t = InitTexture(ctx, d.texture);
    auto& back_end = *t.second;
    if (payloads) {
      GLCHECK(glBindTexture(back_end.target, back_end.texture));
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      switch (t.first->info.type) {
        #ifdef PX_RENDER_BACKEND_GL
        case TextureType::T1D:
          GLCHECK(glTexSubImage1D(GL_TEXTURE_1D,0, d.offset_x, d.width, back_end.format, back_end.type, payloads->data.get()))
        #else
          OnError(ctx, "Texture1D not supported");
        #endif
          break;
        case TextureType::T2D:
          GLCHECK(glTexSubImage2D(GL_TEXTURE_2D,0, d.offset_x, d.offset_y, d.width, d.height, back_end.format, back_end.type, payloads->data.get()))
          break;
        case TextureType::T3D:
          GLCHECK(glTexSubImage3D(GL_TEXTURE_3D,0, d.offset_x, d.offset_y, d.offset_z, d.width, d.height, d.depth, back_end.format, back_end.type, payloads->data.get()))
          break;
        case TextureType::CubeMap:
          OnError(ctx,"CubeMap, not implemented yet");
          break;
      }
      if (d.build_mipmap) GLCHECK(glGenerateMipmap(back_end.target));
    }
  }
  static void Execute(RenderContext::Data *ctx, const DisplayList::SetupViewData &d, const Mem<uint8_t>*) {
    if (d.framebuffer.id != 0) {
      auto fb = GetResource(ctx, d.framebuffer.id, &ctx->framebuffers, &ctx->back_end->framebuffers);
      GLuint fb_id = fb.second->framebuffer;
      if (!fb_id) {
        GLCHECK(glGenFramebuffers(1, &fb_id));
        GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, fb_id));
        for (uint16_t i = 0; i < fb.first->info.num_color_textures; ++i) {
          auto tex = InitTexture(ctx, fb.first->color_texture[i]);
          if (tex.second->target != GL_TEXTURE_2D) OnError(ctx, "Invalid texture type, expected texture 2D (color %u)", i);
          GLCHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, tex.second->texture, 0));
        }
        if (CheckValidResource(ctx, fb.first->depth_texture.id, &ctx->textures)) {
          auto tex = InitTexture(ctx, fb.first->depth_texture);
          if (tex.second->target != GL_TEXTURE_2D) OnError(ctx, "Invalid texture type, expected texture 2D (depth/stencil)");
          switch (tex.first->info.format) {
            case TexelsFormat::Depth_U16:
            case TexelsFormat::Depth_U24:
              GLCHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex.second->texture, 0));
              break;
            case TexelsFormat::DepthStencil_U16:
            case TexelsFormat::DepthStencil_U24:
              GLCHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, tex.second->texture, 0));
              break;
            default:
              OnError(ctx, "Invalid texels-format for a depth/stencil texture ... was %u", tex.first->info.format);
          }
        }

        fb.second->framebuffer = fb_id;
      } else {
        GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, fb_id));
      }
    } else {
      GLCHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }
    if (d.viewport.width != 0 && d.viewport.height != 0) {
      GLCHECK(glViewport(d.viewport.x, d.viewport.y, d.viewport.width, d.viewport.height));
    }
    GLCHECK(glDisable(GL_SCISSOR_TEST));
    ctx->view_matrix = d.view_matrix;
    ctx->projection_matrix = d.projection_matrix;
    ctx->inv_view_matrix_computed = false;
    ctx->inv_projection_matrix_computed = false;
  }


  static void ChangePipeline(RenderContext::Data *ctx, const DisplayList::SetupPipelineData &d) {
    bool main_pipeline_change = !(d.pipeline.id == ctx->last_pipeline.pipeline.id);
    ctx->last_pipeline = d;
    if (main_pipeline_change) {
      auto pi = GetResource(ctx, ctx->last_pipeline.pipeline.id, &ctx->pipelines, &ctx->back_end->pipelines);
      if (pi.second->program == 0) {
        // initialization
        for(auto &i:pi.second->uniforms_location) { i = -1; }
        for(auto &i:pi.second->texture_uniforms_location) { i = -1; }

        uint32_t shader_v = CompileShader(ctx, GL_VERTEX_SHADER,   pi.first->vertex_shader.data.get());
        uint32_t shader_f = CompileShader(ctx, GL_FRAGMENT_SHADER, pi.first->fragment_shader.data.get());
        if (!shader_v || !shader_f) {
          return;
        }
        uint32_t program_id = glCreateProgram();
        if (!program_id) {
          OnError(ctx, "Could not create program object");
          return;
        }
        GLCHECK(glAttachShader(program_id, shader_v));
        GLCHECK(glAttachShader(program_id, shader_f));

        // set attribute positions
        for (auto i = 0; i < kMaxVertexAttribs; ++i) {
          const auto &attrib = pi.first->info.attribs[i];
          if (attrib.format) {
            GLCHECK(glBindAttribLocation(program_id, i, attrib.name));
          } else {
            break;
          }
        }

        GLCHECK(glLinkProgram(program_id));
        int32_t linkStatus = GL_FALSE;
        GLCHECK(glGetProgramiv(program_id, GL_LINK_STATUS, &linkStatus));
        if (linkStatus != GL_TRUE) {
          char log[2048];
          GLCHECK(glGetProgramInfoLog(program_id, 2048, NULL, log));
          GLCHECK(glDeleteProgram(program_id));
          OnError(ctx, "Could not link program --> %s", log);
          return;
        }
        pi.second->program = program_id;
        GLCHECK(glDeleteShader(shader_v));
        GLCHECK(glDeleteShader(shader_f));

        // Gather Uniform Info
        for (auto i = 0; i < kAutomaticUniformCount; ++i) {
          pi.second->uniforms_location[i] = glGetUniformLocation(program_id, Uniforms[i].name);
        }

        for (auto i = 0; i < kMaxTextureUnits; ++i) {
          char name[32];
          snprintf(name, 32, "u_tex%d", i);
          if (pi.first->info.textures[i]) {
            pi.second->texture_uniforms_location[i] = glGetUniformLocation(program_id, name);
          }
        }
      }

      GLCHECK(glUseProgram(pi.second->program));

      if (pi.first->info.blend.enabled) {
        GLCHECK(glEnable(GL_BLEND));
        auto c = pi.first->info.blend.color.c;
        GLCHECK(glBlendColor(c.r, c.g, c.b, c.a));
        GLCHECK(glBlendEquationSeparate(Translate(pi.first->info.blend.op_rgb), Translate(pi.first->info.blend.op_alpha)));
        GLCHECK(glBlendFuncSeparate(
          Translate(pi.first->info.blend.src_rgb),   Translate(pi.first->info.blend.dst_rgb),
          Translate(pi.first->info.blend.src_alpha), Translate(pi.first->info.blend.dst_alpha)
        ));
      } else {
        GLCHECK(glDisable(GL_BLEND));
      }

      switch(pi.first->info.cull) {
        case Cull::Front: 
          GLCHECK(glEnable(GL_CULL_FACE));
          GLCHECK(glCullFace(GL_FRONT));
          break;
        case Cull::Back:
          GLCHECK(glEnable(GL_CULL_FACE));
          GLCHECK(glCullFace(GL_BACK));
          break;
        case Cull::Disabled:
          GLCHECK(glDisable(GL_CULL_FACE));
          break;
      }
      
      GLboolean rgbw = (pi.first->info.rgba_write)?GL_TRUE:GL_FALSE;
      GLboolean dephtw = (pi.first->info.depth_write)?GL_TRUE:GL_FALSE;
      GLCHECK(glColorMask(rgbw, rgbw, rgbw, rgbw));
      GLCHECK(glDepthMask(dephtw));

      if (pi.first->info.depth_func != CompareFunc::Disabled) {
        GLCHECK(glDepthFunc(Translate(pi.first->info.depth_func)));
        GLCHECK(glEnable(GL_DEPTH_TEST));
      } else {
        GLCHECK(glDisable(GL_DEPTH_TEST));
      }

      for (auto i = 0; i < kMaxVertexAttribs; ++i) {
        const auto &attrib = pi.first->info.attribs[i];
        if (attrib.format) {
          GLCHECK(glEnableVertexAttribArray(i));
          switch (attrib.vertex_step) {
            case VertexStep::PerVertex:
              GLCHECK(glVertexAttribDivisor(i,0));
              break;
            case VertexStep::PerInstance:
              GLCHECK(glVertexAttribDivisor(i,1));
              break;
          }
        } else {
          GLCHECK(glDisableVertexAttribArray(i));
        }
      }
    }
  }

  static void Execute(RenderContext::Data *ctx, const DisplayList::SetupPipelineData &d, const Mem<uint8_t> *payloads) {

    ChangePipeline(ctx, d);

    ctx->model_matrix = d.model_matrix;
    auto pi = GetResource(ctx, d.pipeline.id, &ctx->pipelines, &ctx->back_end->pipelines);

    for (auto i = 1; i < kAutomaticUniformCount; ++i) {
      if (pi.second->uniforms_location[i] >= 0) {
        float m[16];
        uint8_t range;
        Uniforms[i].compute(ctx, m, &range);
        switch (range) {
          case 3: GLCHECK(glUniformMatrix3fv(pi.second->uniforms_location[i],1, GL_FALSE, m)); break;
          case 4: GLCHECK(glUniformMatrix4fv(pi.second->uniforms_location[i],1, GL_FALSE, m)); break;
          default: OnError(ctx, "Unexpected range for glUniformMatrix"); break;
        }
      }
    }

    if (pi.second->uniforms_location[0] >= 0) {
      if (payloads) {
        size_t count = payloads->count/(sizeof(float)*4);
        GLCHECK(glUniform4fv(pi.second->uniforms_location[0], count, (const float*) payloads->data.get()));
      } else {
        OnError(ctx, "Expecting a payloads shader requires uniform_data but none was given");
      }
    }

    if (d.scissor.f[2] > 0.0f && d.scissor.f[3] > 0.0f) {
      GLCHECK(glScissor(d.scissor.f[0], d.scissor.f[1], d.scissor.f[2], d.scissor.f[3]));
      GLCHECK(glEnable(GL_SCISSOR_TEST));
    } else {
      GLCHECK(glDisable(GL_SCISSOR_TEST));
    }
  }

  static void BeforeRenderGeometry(RenderContext::Data *ctx) {
    auto pi = GetResource(ctx, ctx->last_pipeline.pipeline.id, &ctx->pipelines, &ctx->back_end->pipelines);
    auto &last_pipeline_command = ctx->last_pipeline;

    size_t tex_unit = 0;
    for (auto i = 0; i < kMaxTextureUnits; ++i) {
      if (pi.second->texture_uniforms_location[i] >= 0) {
        auto ti = GetResource(ctx, last_pipeline_command.texture[i].id, &ctx->textures, &ctx->back_end->textures);
        if (!ti.first ) {
          OnError(ctx, "Missing texture.");
          return;
        }
        
        GLCHECK(glActiveTexture(GL_TEXTURE0+tex_unit));
        switch (ti.first->info.type) {
        #ifdef PX_RENDER_BACKEND_GL
          case TextureType::T1D: GLCHECK(glBindTexture(GL_TEXTURE_1D, ti.second->texture)); break;
        #endif
          case TextureType::T2D: GLCHECK(glBindTexture(GL_TEXTURE_2D, ti.second->texture)); break;
          case TextureType::T3D: GLCHECK(glBindTexture(GL_TEXTURE_3D, ti.second->texture)); break;
          default:
            OnError(ctx, "Invalid texture type...");
            return;
        }

        GLCHECK(glUniform1i(pi.second->texture_uniforms_location[i], tex_unit));
        tex_unit++;
      }
    }

    for (auto i = 0; i < kMaxVertexAttribs; ++i) {
      uint32_t attrib_format = pi.first->info.attribs[i].format;
      if (attrib_format) {
        size_t buffer_index = pi.first->info.attribs[i].buffer_index;
        size_t buffer_id = last_pipeline_command.buffer[buffer_index].id;
        if (!buffer_id) {
          OnError(ctx, "Expected Valid buffer (see pipeline declaration)");
          return;
        }
        auto buffer = GetResource(ctx, buffer_id, &ctx->buffers, &ctx->back_end->buffers);
        if (buffer.second->buffer == 0) {
          OnError(ctx, "Invalid OpenGL-buffer (vertex data)");
        }
        PX_RENDER_DEBUG_FUNC(4000, "     (Attrib %u) vertex buffer %u\n", i, buffer_id)
        GLCHECK(glBindBuffer(GL_ARRAY_BUFFER, buffer.second->buffer));
        GLint a_size = (attrib_format & VertexFormat::NumComponentsMask) >> VertexFormat::NumComponentsShift;
        GLenum a_type = TranslateVertexType(attrib_format);
        GLboolean a_normalized = (attrib_format & VertexFormat::Normalized)?GL_TRUE:GL_FALSE;
        GLsizei a_stride = pi.first->info.attribs[i].stride;
        GLsizei a_offset = pi.first->info.attribs[i].offset;
        GLCHECK(glVertexAttribPointer(i, a_size, a_type, a_normalized, a_stride, (void*) a_offset));
      } else {
        break;
      }
    }
  }

  static void Execute(RenderContext::Data *ctx, const DisplayList::RenderData &d, const Mem<uint8_t> *payloads) {
    PX_RENDER_DEBUG_FUNC(3000, "    Render With Pipeline %u, index buffer %u\n", ctx->last_pipeline.pipeline.id, d.index_buffer.id)
    auto b = GetResource(ctx, d.index_buffer.id, &ctx->buffers, &ctx->back_end->buffers);
    auto p = GetResource(ctx,ctx->last_pipeline.pipeline.id, &ctx->pipelines, &ctx->back_end->pipelines);

    if (!b.second->buffer) OnError(ctx, "Invalid Index buffer...");

    BeforeRenderGeometry(ctx);
    GLCHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.second->buffer));
    GLCHECK(glDrawElementsInstanced(Translate(p.first->info.primitive), d.count, Translate(d.type), (void*)d.offset, d.instances));
  }
  

  void DestroyBackEndResource(RenderContext::Data *ctx, const GPUResource::Type::Enum type, uint32_t pos) {
    BackEnd *b = ctx->back_end;
    switch (type) {
      case GPUResource::Type::Invalid:
        OnError(ctx, "Trying to destroy an invalid resource");
        break;
      case GPUResource::Type::Buffer:
        GLCHECK(glDeleteBuffers(1, &b->buffers[pos].buffer));
        b->buffers[pos].buffer = 0;
        break;
      case GPUResource::Type::Pipeline:
        GLCHECK(glDeleteProgram(b->pipelines[pos].program));
        b->pipelines[pos].program = 0;
        break;
      case GPUResource::Type::Texture:
        GLCHECK(glDeleteTextures(1, &b->textures[pos].texture));
        b->textures[pos].texture = 0;
        break;
      case GPUResource::Type::Framebuffer:
        GLCHECK(glDeleteFramebuffers(1, &b->framebuffers[pos].framebuffer));
        b->framebuffers[pos].framebuffer = 0;
        break;
      default:
        OnError(ctx, "Invalid Resource");
    }
  }

  #undef GLCHECK
  #undef GLCHECK_STR
  #undef GLCHECK_STR_STR

}

#endif
