// px_sched: Task Scheduler
#define PX_SCHED_IMPLEMENTATION
#include "../px_sched.h"

// gb_math & sokol_app (Window Support)
#define GB_MATH_IMPLEMENTATION
#define SOKOL_WIN32_NO_GL_LOADER
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "deps/gb_math.h"
#include "deps/sokol_app.h"

// OpenGL + px_render
#include "deps/glad.c"
#define PX_RENDER_IMPLEMENTATION
#include "../px_render.h"


struct {
  px_render::RenderContext ctx;
  px_sched::Scheduler sched;
  px_sched::Sync render_end;
  std::atomic<bool> running;
} State;

void Render();

void init() {
  gladLoadGL();
  State.ctx.init();
  State.sched.init();
  State.running = true;
  State.sched.run(Render, &State.render_end);
}

void frame() {
  while(true) {
    px_render::RenderContext::Result::Enum result = State.ctx.executeOnGPU();
    if (result != px_render::RenderContext::Result::OK) break;
  }
}

void cleanup() {
  State.ctx.finish();
  State.running = false;
  State.sched.waitFor(State.render_end);
  State.sched.stop();
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

#define GLSL(...) "#version 330\n" #__VA_ARGS__

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

void Render() {
  using namespace px_render;
  Mat4 proj;
  Mat4 proj_fb;
  Mat4 view;
  Mat4 view_fb;
  gb_mat4_perspective((gbMat4*)&proj, 45.f, 1024/(float)768, 0.05f, 900.0f);
  gb_mat4_perspective((gbMat4*)&proj_fb, 45.f, 1.0f, 0.05f, 900.0f);
  gb_mat4_look_at((gbMat4*)&view, {0.f,3.f,-3.f}, {0.f,0.f,0.0f}, {0.f,1.f,0.f});
  gb_mat4_look_at((gbMat4*)&view_fb, {0.f,10.f,-20.f}, {0.f,0.f,0.0f}, {0.f,1.f,0.f});

  struct {
    Vec3 instance_positions[5000];
    
    Pipeline material;
    Buffer vertex_buff = State.ctx.createBuffer({sizeof(Cube::vertex_data), Usage::Static});
    Buffer index_buff = State.ctx.createBuffer({sizeof(Cube::index_data), Usage::Static});
    Buffer instance_buff = State.ctx.createBuffer({sizeof(instance_positions), Usage::Stream});
    Texture texture;
  } cube;

  struct {
    Pipeline material;
    Buffer vertex_buff = State.ctx.createBuffer({sizeof(Quad::vertex_data), Usage::Static});
    Buffer index_buff = State.ctx.createBuffer({sizeof(Quad::index_data), Usage::Static});
  } quad;

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
    cube.material = State.ctx.createPipeline(pipeline_info);


    Texture::Info tex_info;
    tex_info.format = TexelsFormat::R_U8;
    tex_info.width = 4;
    tex_info.height = 4;
    tex_info.magnification_filter = SamplerFiltering::Nearest;
    tex_info.minification_filter = SamplerFiltering::Nearest;
    cube.texture = State.ctx.createTexture(tex_info);
    {
      DisplayList dl;
      dl.fillBufferCommand()
        .set_buffer(cube.vertex_buff)
        .set_data(Cube::vertex_data)
        .set_size(sizeof(Cube::vertex_data));
      dl.fillBufferCommand()
        .set_buffer(cube.index_buff)
        .set_data(Cube::index_data)
        .set_size(sizeof(Cube::index_data));

      uint8_t tex_data[16] = {
        255, 0, 255, 0,
        0, 255, 0, 255,
        255, 0, 255, 0,
        0, 255, 0, 255,
      };
      dl.fillTextureCommand()
        .set_texture(cube.texture)
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
    quad.material = State.ctx.createPipeline(pipeline_info);
    DisplayList dl;
    dl.fillBufferCommand()
      .set_buffer(quad.vertex_buff)
      .set_data(Quad::vertex_data)
      .set_size(sizeof(Quad::vertex_data));
    dl.fillBufferCommand()
      .set_buffer(quad.index_buff)
      .set_data(Quad::index_data)
      .set_size(sizeof(Quad::index_data));
    State.ctx.submitDisplayList(std::move(dl));
  }

  // Create Framebuffer (offscreen rendering)
  Texture::Info fb_tex_color = {640,640};
  Texture::Info fb_tex_depth = {640,640};
  fb_tex_color.format = TexelsFormat::RGBA_U8;
  fb_tex_depth.format = TexelsFormat::Depth_U16;

  Framebuffer fb = State.ctx.createFramebuffer({fb_tex_color, fb_tex_depth});

  float v = 0;
  while(State.running) {
    for (int i = 0; i < sizeof(cube.instance_positions) / sizeof(Vec3); ++i) {
      cube.instance_positions[i] = {(float)(i%1000)*3.0f,5.0f*(float)(gb_sin(i*GB_MATH_PI/10+v)), (i/1000)*3.0f};
    }
    Mat4 model;
    gb_mat4_rotate((gbMat4*)model.f, {0,1,0}, v);
    v+= 0.01;
    DisplayList dl;
    dl.setupViewCommand()
      .set_viewport({0,0,640,640})
      .set_projection_matrix(proj_fb)
      .set_view_matrix(view_fb)
      .set_framebuffer(fb)
      ;
    dl.clearCommand()
      .set_color({0.2f,0.2f,0.2f,1.0f})
      .set_clear_color(true)
      .set_clear_depth(true)
      ;
    dl.fillBufferCommand()
      .set_buffer(cube.instance_buff)
      .set_data(cube.instance_positions)
      .set_size(sizeof(cube.instance_positions))
      ;
    dl.setupPipelineCommand()
      .set_pipeline(cube.material)
      .set_buffer(0,cube.vertex_buff)
      .set_buffer(1,cube.instance_buff)
      .set_model_matrix(model)
      .set_texture(0, cube.texture)
      ;
    dl.renderCommand()
      .set_index_buffer(cube.index_buff)
      .set_count(sizeof(Cube::index_data)/sizeof(uint16_t))
      .set_type(IndexFormat::UInt16)
      .set_instances(sizeof(cube.instance_positions)/sizeof(cube.instance_positions[0]))
      ;
    // now render to the main FrameBuffer...
    dl.setupViewCommand()
      .set_viewport({0,0,1024,768})
      .set_projection_matrix(proj)
      .set_view_matrix(view)
      ;
    dl.clearCommand()
      .set_color({0.5f,0.7f,0.8f,1.0f})
      .set_clear_color(true)
      .set_clear_depth(true)
      ;
    dl.setupPipelineCommand()
      .set_pipeline(quad.material)
      .set_buffer(0,quad.vertex_buff)
      .set_texture(0, fb.color_texture())
      .set_model_matrix(model)
      ;
    dl.renderCommand()
      .set_index_buffer(quad.index_buff)
      .set_count(sizeof(Quad::index_data)/sizeof(uint16_t))
      .set_type(IndexFormat::UInt16)
      ;

    State.ctx.submitDisplayListAndSwap(std::move(dl));
   }
   // Free resources 
   // This is just an example, all resources are freed when the context is 
   // closed anyway.
   DisplayList dl;
   dl.destroy(cube.texture)
     .destroy(cube.index_buff)
     .destroy(cube.material)
     .destroy(cube.vertex_buff)
     .destroy(cube.instance_buff)
     ;
   State.ctx.submitDisplayList(std::move(dl));
}
