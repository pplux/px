#include "common/render_common.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STB_IMAGE_IMPLEMENTATION
#define PX_RENDER_GLTF_IMPLEMENTATION
#include "deps/tiny_gltf.h"
#include "../px_render_gltf.h"

using namespace px_render;

struct {
  Pipeline material;
  GLTF gltf;
  std::atomic<bool> gltf_ready = {false};
} State ;

void init(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {
  Pipeline::Info pipeline_info;
  pipeline_info.shader.vertex = GLSL(
    uniform mat4 u_viewProjection;
    in vec3 position;
    in vec3 normal;
    in vec2 uv;
    out vec2 v_tex;
    void main() {
      gl_Position = u_viewProjection * vec4(position,1.0);
      v_tex = uv;
    }
  );
  pipeline_info.shader.fragment = GLSL(
    out vec4 color_out;
    in vec2 v_tex;
    uniform sampler2D u_tex0;
    void main() {
      color_out = texture(u_tex0, v_tex);
    }
  );
  pipeline_info.attribs[0] = {"position", VertexFormat::Float3};
  pipeline_info.attribs[1] = {"normal", VertexFormat::Float3};
  pipeline_info.attribs[2] = {"uv", VertexFormat::Float2};
  pipeline_info.textures[0] = TextureType::T2D;
  State.material = ctx->createPipeline(pipeline_info);

  sched->run([ctx]{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warning;
    if (!loader.LoadASCIIFromFile(&model, &err, &warning, "t2/Scene.gltf")) {
      fprintf(stderr,"Error loading GLTF %s", err.c_str());
      assert(!"GLTF_ERROR");
    }
     State.gltf.init(ctx, model);
     State.gltf_ready = true;
  });
}

void finish(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {
}

void render(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {
  Mat4 proj;
  Mat4 view;
  
  float dmin = std::min<float>({State.gltf.bounds_min.f[0], State.gltf.bounds_min.f[1], State.gltf.bounds_min.f[2]});
  float dmax = std::max<float>({State.gltf.bounds_max.f[0], State.gltf.bounds_max.f[1], State.gltf.bounds_max.f[2]});

  gb_mat4_perspective((gbMat4*)&proj, gb_to_radians(45.f), sapp_width()/(float)sapp_height(), 1.0f, (dmax-dmin)*2.0f);
  gb_mat4_look_at((gbMat4*)&view, {0.f,(dmax-dmin)*1.2f,0.0f}, {0.f,0.f,0.0f}, {0.f,0.f,1.f});
  static float angle = 0.0f;
  angle += 0.01f;
  view = Mat4::Mult(view, Mat4::SRT({1.0f,1.0f,1.0f}, {0.0f, 0.0f, 1.0f, angle}, {0.0f, 0.0f, 0.0f}));
  px_render::DisplayList dl;
  dl.setupViewCommand()
    .set_viewport({0,0, (uint16_t) sapp_width(), (uint16_t) sapp_height()})
    .set_projection_matrix(proj)
    .set_view_matrix(view)
    ;
  dl.clearCommand()
    .set_color({0.5f,0.7f,0.8f,1.0f})
    .set_clear_color(true)
    .set_clear_depth(true)
    ;
  if (State.gltf_ready) {
    for(uint32_t i = 0; i < State.gltf.num_primitives; ++i) {
      const auto &node_idx = State.gltf.primitives[i].node;
      const auto &model = State.gltf.nodes[node_idx].model;
      const auto &material = State.gltf.materials[State.gltf.primitives[i].material];
      dl.setupPipelineCommand()
        .set_pipeline(State.material)
        .set_buffer(0,State.gltf.vertex_buffer)
        .set_model_matrix(model)
        .set_texture(0, State.gltf.textures[material.base_color.texture].tex);
        ;
      dl.renderCommand()
        .set_index_buffer(State.gltf.index_buffer)
        .set_count(State.gltf.primitives[i].index_count)
        .set_offset(State.gltf.primitives[i].index_offset*sizeof(uint32_t))
        .set_type(IndexFormat::UInt32)
        ;
     }
  }
  
  ImGui::NewFrame();
  ImGui::Render();
  ImGui_Impl_pxrender_RenderDrawData(ImGui::GetDrawData(), &dl);

  ctx->submitDisplayListAndSwap(std::move(dl));
}
