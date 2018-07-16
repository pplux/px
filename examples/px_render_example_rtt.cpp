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


namespace Cube {
  float vertex_data[] = {
    -1.0, -1.0, -1.0,  0.1, 0.1, 0.3, 1.0,   0.0, 0.0, 
    1.0, -1.0, -1.0,   0.1, 0.1, 0.3, 1.0,   1.0, 0.0,
    1.0,  1.0, -1.0,   0.1, 0.1, 0.3, 1.0,   1.0, 1.0,
    -1.0,  1.0, -1.0,  0.1, 0.1, 0.3, 1.0,   0.0, 1.0,

    -1.0, -1.0,  1.0,  0.1, 0.1, 1.0, 1.0,   0.0, 0.0,
    1.0, -1.0,  1.0,   0.1, 0.1, 1.0, 1.0,   1.0, 0.0,
    1.0,  1.0,  1.0,   0.1, 0.1, 1.0, 1.0,   1.0, 1.0,
    -1.0,  1.0,  1.0,  0.1, 0.1, 1.0, 1.0,   0.0, 1.0,
                                            
    -1.0, -1.0, -1.0,  0.3, 0.1, 0.1, 1.0,   0.0, 0.0,
    -1.0,  1.0, -1.0,  0.3, 0.1, 0.1, 1.0,   1.0, 0.0,
    -1.0,  1.0,  1.0,  0.3, 0.1, 0.1, 1.0,   1.0, 1.0,
    -1.0, -1.0,  1.0,  0.3, 0.1, 0.1, 1.0,   0.0, 1.0,
                                            
    1.0, -1.0, -1.0,   1.0, 0.1, 0.1, 1.0,   0.0, 0.0,
    1.0,  1.0, -1.0,   1.0, 0.1, 0.1, 1.0,   1.0, 0.0,
    1.0,  1.0,  1.0,   1.0, 0.1, 0.1, 1.0,   1.0, 1.0,
    1.0, -1.0,  1.0,   1.0, 0.1, 0.1, 1.0,   0.0, 1.0,
                                            
    -1.0, -1.0, -1.0,  0.1, 0.3, 0.1, 1.0,   0.0, 0.0,
    -1.0, -1.0,  1.0,  0.1, 0.3, 0.1, 1.0,   1.0, 0.0,
    1.0, -1.0,  1.0,   0.1, 0.3, 0.1, 1.0,   1.0, 1.0,
    1.0, -1.0, -1.0,   0.1, 0.3, 0.1, 1.0,   0.0, 1.0,
                                            
    -1.0,  1.0, -1.0,  0.1, 1.0, 0.1, 1.0,   0.0, 0.0,
    -1.0,  1.0,  1.0,  0.1, 1.0, 0.1, 1.0,   1.0, 0.0,
    1.0,  1.0,  1.0,   0.1, 1.0, 0.1, 1.0,   1.0, 1.0,
    1.0,  1.0, -1.0,   0.1, 1.0, 0.1, 1.0,   0.0, 1.0
  };

  uint16_t index_data[] = {
    0, 2, 1,  0, 3, 2, 6, 4, 5,  7, 4, 6,
    8, 10, 9,  8, 11, 10, 14, 12, 13,  15, 12, 14,
    16, 18, 17,  16, 19, 18, 22, 20, 21,  23, 20, 22
  };
}


namespace Quad {
  float vertex_data[] = {
    -1.0, -1.0, .0,     0.0, 0.0, 
     1.0, -1.0, .0,     1.0, 0.0,
     1.0,  1.0, .0,     1.0, 1.0,
    -1.0,  1.0, .0,     0.0, 1.0
    };
  uint16_t index_data[] = {0, 2, 1,  0, 3, 2};
}

struct {
  RenderContext ctx;
  Mat4 proj;
  Mat4 proj_fb;
  Mat4 view;
  Mat4 view_fb;
  Framebuffer fb;
  struct {
    Vec3 instance_positions[5000];
    Pipeline material;
    Buffer vertex_buff;
    Buffer index_buff;
    Buffer instance_buff;
    Texture texture;
  } cube;
  struct {
    Pipeline material;
    Buffer vertex_buff;
    Buffer index_buff;
  } quad;
} State = {};


void init() {
  #ifdef PX_RENDER_BACKEND_GL
  gladLoadGL();
  #endif
  State.ctx.init();
  gb_mat4_perspective((gbMat4*)&State.proj, gb_to_radians(45.f), 1024/(float)768, 0.05f, 900.0f);
  gb_mat4_perspective((gbMat4*)&State.proj_fb, gb_to_radians(45.f), 1.0f, 0.05f, 900.0f);
  gb_mat4_look_at((gbMat4*)&State.view, {0.f,0.5f,-3.f}, {0.f,0.f,0.0f}, {0.f,1.f,0.f});
  gb_mat4_look_at((gbMat4*)&State.view_fb, {0.f,10.f,-20.f}, {0.f,0.f,0.0f}, {0.f,1.f,0.f});

  State.cube.vertex_buff = State.ctx.createBuffer({BufferType::Vertex, sizeof(Cube::vertex_data), Usage::Static});
  State.cube.index_buff = State.ctx.createBuffer({BufferType::Index, sizeof(Cube::index_data), Usage::Static});
  State.cube.instance_buff = State.ctx.createBuffer({BufferType::Vertex, sizeof(State.cube.instance_positions), Usage::Stream});

  State.quad.vertex_buff = State.ctx.createBuffer({BufferType::Vertex, sizeof(Quad::vertex_data), Usage::Static});
  State.quad.index_buff = State.ctx.createBuffer({BufferType::Index, sizeof(Quad::index_data), Usage::Static});

  { // Cube Material & Object setup 
    Pipeline::Info pipeline_info;
    pipeline_info.shader.vertex = GLSL(
      uniform mat4 u_modelViewProjection;
      in vec3 position;
      in vec3 instance_position;
      in vec4 color;
      in vec2 uv;
      out vec4 v_color;
      out vec2 v_uv;
      void main() {
        gl_Position = u_modelViewProjection * vec4(position + instance_position,1.0);
        v_color = color;
        v_uv = uv;
      }
    );
    pipeline_info.shader.fragment = GLSL(
      in vec4 v_color;
      in vec2 v_uv;
      uniform sampler2D u_tex0;
      out vec4 color_out;
      void main() {
        color_out = vec4(v_color.rgb*texture(u_tex0, v_uv).r,1.0);
      }
    );
    pipeline_info.attribs[0] = {"position", VertexFormat::Float3};
    pipeline_info.attribs[1] = {"color", VertexFormat::Float4};
    pipeline_info.attribs[2] = {"uv", VertexFormat::Float2};
    pipeline_info.attribs[3] = {"instance_position", VertexFormat::Float3, 1, VertexStep::PerInstance};
    pipeline_info.textures[0] = TextureType::T2D;
    State.cube.material = State.ctx.createPipeline(pipeline_info);


    Texture::Info tex_info;
    tex_info.format = TexelsFormat::R_U8;
    tex_info.width = 4;
    tex_info.height = 4;
    tex_info.magnification_filter = SamplerFiltering::Nearest;
    tex_info.minification_filter = SamplerFiltering::Nearest;
    State.cube.texture = State.ctx.createTexture(tex_info);
    {
      DisplayList dl;
      dl.fillBufferCommand()
        .set_buffer(State.cube.vertex_buff)
        .set_data(Cube::vertex_data)
        .set_size(sizeof(Cube::vertex_data));
      dl.fillBufferCommand()
        .set_buffer(State.cube.index_buff)
        .set_data(Cube::index_data)
        .set_size(sizeof(Cube::index_data));

      uint8_t tex_data[16] = {
        255, 0, 255, 0,
        0, 255, 0, 255,
        255, 0, 255, 0,
        0, 255, 0, 255,
      };
      dl.fillTextureCommand()
        .set_texture(State.cube.texture)
        .set_data(tex_data)
        ;
      State.ctx.submitDisplayList(std::move(dl));
    }
  } // Cube Setup


  { // Quad Material & Object setup 
    Pipeline::Info pipeline_info;
    pipeline_info.shader.vertex = GLSL(
      uniform mat4 u_modelViewProjection;
      in vec3 position;
      in vec4 color;
      in vec2 uv;
      out vec2 v_uv;
      void main() {
        gl_Position = u_modelViewProjection * vec4(position,1.0);
        v_uv = uv;
      }
    );
    pipeline_info.shader.fragment = GLSL(
      in vec2 v_uv;
      uniform sampler2D u_tex0;
      out vec4 color_out;
      void main() {
        color_out = texture(u_tex0, v_uv);
      }
    );
    pipeline_info.attribs[0] = {"position", VertexFormat::Float3};
    pipeline_info.attribs[1] = {"uv", VertexFormat::Float2};
    pipeline_info.textures[0] = TextureType::T2D;
    pipeline_info.cull = Cull::Disabled;
    State.quad.material = State.ctx.createPipeline(pipeline_info);
    DisplayList dl;
    dl.fillBufferCommand()
      .set_buffer(State.quad.vertex_buff)
      .set_data(Quad::vertex_data)
      .set_size(sizeof(Quad::vertex_data));
    dl.fillBufferCommand()
      .set_buffer(State.quad.index_buff)
      .set_data(Quad::index_data)
      .set_size(sizeof(Quad::index_data));
    State.ctx.submitDisplayList(std::move(dl));
  }

  // Create Framebuffer (offscreen rendering)
  Texture::Info fb_tex_color = {640,640};
  Texture::Info fb_tex_depth = {640,640};
  fb_tex_color.format = TexelsFormat::RGBA_U8;
  fb_tex_depth.format = TexelsFormat::Depth_U16;

  State.fb = State.ctx.createFramebuffer({fb_tex_color, fb_tex_depth});
}

void frame() {
  // Render could be done on other thread, but for simplicity and
  // to make the example work along with EMSCRIPTEN let's pretend
  static float v = 0;
  for (int i = 0; i < sizeof(State.cube.instance_positions) / sizeof(Vec3); ++i) {
    State.cube.instance_positions[i] = {
      (float)(i%1000)*3.0f,
      5.0f*(float)(gb_sin(i*GB_MATH_PI/10+v)),
      (i/1000)*3.0f};
  }
  Mat4 model;
  gb_mat4_rotate((gbMat4*)model.f, {0,1,0}, v);
  v+= 0.01;
  DisplayList dl;
  dl.setupViewCommand()
    .set_viewport({0,0,640,640})
    .set_projection_matrix(State.proj_fb)
    .set_view_matrix(State.view_fb)
    .set_framebuffer(State.fb)
    ;
  dl.clearCommand()
    .set_color({0.2f,0.2f,0.2f,1.0f})
    .set_clear_color(true)
    .set_clear_depth(true)
    ;
  dl.fillBufferCommand()
    .set_buffer(State.cube.instance_buff)
    .set_data(State.cube.instance_positions)
    .set_size(sizeof(State.cube.instance_positions))
    ;
  dl.setupPipelineCommand()
    .set_pipeline(State.cube.material)
    .set_buffer(0,State.cube.vertex_buff)
    .set_buffer(1,State.cube.instance_buff)
    .set_model_matrix(model)
    .set_texture(0, State.cube.texture)
    ;
  dl.renderCommand()
    .set_index_buffer(State.cube.index_buff)
    .set_count(sizeof(Cube::index_data)/sizeof(uint16_t))
    .set_type(IndexFormat::UInt16)
    .set_instances(sizeof(State.cube.instance_positions)/sizeof(State.cube.instance_positions[0]))
    ;
  // now render to the main FrameBuffer...
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

  gb_mat4_rotate((gbMat4*)model.f, {0,1,0}, v*0.25);
  dl.setupPipelineCommand()
    .set_pipeline(State.quad.material)
    .set_buffer(0,State.quad.vertex_buff)
    .set_texture(0, State.fb.color_texture())
    .set_model_matrix(model)
    ;
  dl.renderCommand()
    .set_index_buffer(State.quad.index_buff)
    .set_count(sizeof(Quad::index_data)/sizeof(uint16_t))
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

