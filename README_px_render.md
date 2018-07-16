# px_render
Single Header C++ Command Based backend renderer 

Written in C++11/14, with no dependency, and easy to integrate. See a complete example
[HelloWorldTriangle](https://github.com/pplux/px/blob/master/examples/px_render_example_triangle.cpp),  [RenderToTexture](https://github.com/pplux/px/blob/master/examples/px_render_example_rtt.cpp).

The API itself is render agnostic, and will support multiple backends in the future (OpenGL, Vulkan, DX,...) but right now it only supports OpenGL to start with.

## Goals:
* Multithreaded Rendering, designed to be used in multithreded applications
* Command Based Rendering, allows to build the render commands in advance in a separte thread
* Reentrant API, everything is held in a 'Context' class, making very easy to support multiple windows, and so on 
* No dependency on other libraries, portable and easy to extend
* Implicit task graph dependency for single or groups of tasks

Thanks to [bgfx](https://github.com/bkaradzic/bgfx) and [sokol_gfx.h](https://github.com/floooh/sokol/blob/master/sokol_gfx.h) they've been a great inspiration, you can read about why px_render and how it compares to bgfx and sokol_gfx [here](https://pplux.github.io/why_px_render.html).

## API:

Example of different parts of the API extracted from the examples, not meant to be compilable as it is, this is just an illustration.

```cpp

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

```

*WIP* *WIP* *WIP* *WIP* *WIP* *WIP*


## TODO's
* [  ] improve documentation
* [  ] More tests! and examples
* [  ] Proper stencil support
* [  ] Compute shaders support
* [  ] WebGL, RaspberryPI, etc.., it should work
* [  ] Add Vulkan, DX render, other backends 
