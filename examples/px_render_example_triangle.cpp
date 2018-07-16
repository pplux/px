// Emscripten config
#ifdef __EMSCRIPTEN__
// Compile with  emcc --std=c++14 -s USE_WEBGL2=1 px_render_example_rtt.cpp -o rtt.html
#  define SOKOL_GLES3
#  define PX_RENDER_BACKEND_GLES
#  define GLSL(...) "#version 300 es\n precision highp float;\n" #__VA_ARGS__
#else
#  define PX_RENDER_BACKEND_GL //< Default
#  define SOKOL_WIN32_NO_GL_LOADER
#  define SOKOL_GLCORE33
#define GLSL(...) "#version 330\n" #__VA_ARGS__
// OpenGL + px_render
#  include "deps/glad.c"
#endif

// gb_math & sokol_app (Window Support)
#define GB_MATH_IMPLEMENTATION
#define SOKOL_IMPL
#include "deps/gb_math.h"
#include "deps/sokol_app.h"

#define PX_RENDER_IMPLEMENTATION
#include "../px_render.h"

using namespace px_render;

namespace Geometry {
  float vertex_data[] = {
    -1.0, -1.0, 0.0,   1.0, 0.0, 0.0,
     1.0, -1.0, 0.0,   0.0, 1.0, 0.0,
     0.0,  0.8, 0.0,   0.0, 0.0, 1.0,
  };

  uint16_t index_data[] = {
    0, 1, 2,
  };
}

struct {
  RenderContext ctx;
  Mat4 proj;
  Mat4 view;
  Pipeline material;
  Buffer vertex_buff;
  Buffer index_buff;
} State = {};

void init() {
  #ifdef PX_RENDER_BACKEND_GL
  gladLoadGL();
  #endif
  State.ctx.init();
  gb_mat4_perspective((gbMat4*)&State.proj, gb_to_radians(45.f), 1024/(float)768, 0.05f, 900.0f);
  gb_mat4_look_at((gbMat4*)&State.view, {0.f,0.0f,3.f}, {0.f,0.f,0.0f}, {0.f,1.f,0.f});

  State.vertex_buff = State.ctx.createBuffer({BufferType::Vertex, sizeof(Geometry::vertex_data), Usage::Static});
  State.index_buff  = State.ctx.createBuffer({BufferType::Index,  sizeof(Geometry::index_data),  Usage::Static});

  Pipeline::Info pipeline_info;
  pipeline_info.shader.vertex = GLSL(
    uniform mat4 u_viewProjection;
    in vec3 position;
    in vec3 color;
    out vec3 v_color;
    void main() {
      gl_Position = u_viewProjection * vec4(position,1.0);
      v_color = color;
    }
  );
  pipeline_info.shader.fragment = GLSL(
    in vec3 v_color;
    out vec4 color_out;
    void main() {
      color_out = vec4(v_color,1.0);
    }
  );
  pipeline_info.attribs[0] = {"position", VertexFormat::Float3};
  pipeline_info.attribs[1] = {"color", VertexFormat::Float3};
  State.material = State.ctx.createPipeline(pipeline_info);

  // upload data
  DisplayList dl;
  dl.fillBufferCommand()
    .set_buffer(State.vertex_buff)
    .set_data(Geometry::vertex_data)
    .set_size(sizeof(Geometry::vertex_data));
  dl.fillBufferCommand()
    .set_buffer(State.index_buff)
    .set_data(Geometry::index_data)
    .set_size(sizeof(Geometry::index_data));

  State.ctx.submitDisplayList(std::move(dl));
}

void frame() {
  DisplayList dl;
  dl.setupViewCommand()
    .set_viewport({0,0,1024,768})
    .set_projection_matrix(State.proj)
    .set_view_matrix(State.view)
    ;
  dl.clearCommand()
    .set_color({0.5f,0.7f,0.8f,1.0f})
    .set_clear_color(true)
    .set_clear_depth(true)
    ;
  dl.setupPipelineCommand()
    .set_pipeline(State.material)
    .set_buffer(0,State.vertex_buff)
    ;
  dl.renderCommand()
    .set_index_buffer(State.index_buff)
    .set_count(3)
    .set_type(IndexFormat::UInt16)
    ;
  State.ctx.submitDisplayListAndSwap(std::move(dl));

  // GPU Render...
  while(true) {
    RenderContext::Result::Enum result = State.ctx.executeOnGPU();
    if (result != RenderContext::Result::OK) break;
  }
}

void cleanup() {
  State.ctx.finish();
}

sapp_desc sokol_main(int argc, char **argv) {
  sapp_desc d = {};
  d.init_cb = init;
  d.frame_cb = frame;
  d.cleanup_cb = cleanup;
  d.width = 1024;
  d.height = 768;
  d.window_title = "PX-Render Test";
  return d;
}

