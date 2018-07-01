# px_render
Single Header C++ Command Based backend renderer 

Written in C++11/14, with no dependency, and easy to integrate. See a complete example
[test1.cpp](https://github.com/pplux/px/blob/master/examples/px_render_example_rtt.cpp).

The API itself is render agnostic, and will support multiple backends in the future (OpenGL, Vulkan, DX,...) but right now it only supports OpenGL to start with.

## Goals:
* Multithreaded Rendering, designed to be used in multithreded applications
* Command Based Rendering, allows to build the render commands in advance in a separte thread
* Reentrant API, everything is held in a 'Context' class, making very easy to support multiple windows, and so on 
* No dependency on other libraries, portable and easy to extend
* Implicit task graph dependency for single or groups of tasks

Thanks to [bgfx](https://github.com/bkaradzic/bgfx) and [sokol_gfx.h](https://github.com/floooh/sokol/blob/master/sokol_gfx.h) they've been a great inspiration. I should write about them and to enumerate how px_render.h is different, *sometime in the future...*

## API:

Example of different parts of the API extracted from the examples, not meant to be compilable as it is, this is just an illustration.

```cpp

// Somewhere in your application you create a Context to hold all the state of an instance
// of px_render

px_render::RenderContext ctx;

// somehere, in the rendering thread, you should call to something similar to 
void RenderThread() {
  ctx.init(); //<-- it can be parametrized...
  bool running = true;
  while(running) {
    px_render::RenderContext::Result::Enum result = ctx.executeOnGPU();
    switch(ctx.executeOnGPU) {
      case px_render::RenderContext::Result::OK:
        /* Nothing to do really... a DisplayList finished*/
        break;
      case px_render::RenderContext::Result::OK_Swap:
        /*render done, swap window*/
        break;
      case px_render::RenderContext::Result::Finished:
        /* ctx.finish() was called and no render needs to be done */
        running = false;
        break;
    }
  }
}

// To Draw, from any other thread, Create Resources then a DisplayList and submit it
void Render {
  using px_render;

  struct {
    float vertex_data[] = {
      -1.0, -1.0, .0,     0.0, 0.0, 
       1.0, -1.0, .0,     1.0, 0.0,
       1.0,  1.0, .0,     1.0, 1.0,
      -1.0,  1.0, .0,     0.0, 1.0
      };
    uint16_t index_data[] = {0, 2, 1,  0, 3, 2};

    Buffer vertex_buff = ctx.createBuffer({sizeof(vertex_data), Usage::Static});
    Buffer index_buff  = ctx.createBuffer({sizeof(index_data),  Usage::Static});
  } quad;
  
  // Create a pipeline ....
  Pipeline material;
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
  material = ctx.createPipeline(pipeline_info);  

  // on the main loop of the logic, use the resources created, build a display list and submit it
  while(true) {
    DisplayList dl;
    dl.setupViewCommand()
      .set_viewport({0,0,640,640})
      .set_projection_matrix(proj)
      .set_view_matrix(view)
      ;
    dl.clearCommand()
      .set_color({0.2f,0.2f,0.2f,1.0f})
      .set_clear_color(true)
      .set_clear_depth(true)
      ;
    dl.fillBufferCommand()
      .set_buffer(quad.instance_buff)
      .set_data(quad.instance_positions)
      .set_size(sizeof(quad.instance_positions))
      ;
    dl.setupPipelineCommand()
      .set_pipeline(cube.material)
      .set_buffer(0,cube.vertex_buff)
      .set_buffer(1,cube.instance_buff)
      .set_model_matrix(model)
      .set_texture(0, cube.texture)
      ;
    dl.renderCommand()
      .set_index_buffer(quad.index_buff)
      .set_count(6) // 2 triangles
      .set_type(IndexFormat::UInt16)
      .set_instances(100)
      ;
    ctx.submitDisplayList(std::move(dl));
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
