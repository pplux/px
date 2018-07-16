#include "common/render_common.h"

// demo
#include "deps/imgui_demo.cpp"

using namespace px_render;

struct {
  Pipeline material;
  Buffer vertex_buff;
  Buffer index_buff;
} State ;

float vertex_data[] = {
  -1.0, -1.0, 0.0,   1.0, 0.0, 0.0,
   1.0, -1.0, 0.0,   0.0, 1.0, 0.0,
   0.0,  0.8, 0.0,   0.0, 0.0, 1.0,
};

uint16_t index_data[] = {
  0, 1, 2,
};

void init(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {

  freopen("px_log.txt", "w", stdout);
  
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
  State.material = ctx->createPipeline(pipeline_info);

  State.vertex_buff = ctx->createBuffer({BufferType::Vertex, sizeof(vertex_data), Usage::Static});
  State.index_buff  = ctx->createBuffer({BufferType::Index,  sizeof(index_data),  Usage::Static});
  // upload data
  DisplayList dl;
  dl.fillBufferCommand()
    .set_buffer(State.vertex_buff)
    .set_data(vertex_data)
    .set_size(sizeof(vertex_data));
  dl.fillBufferCommand()
    .set_buffer(State.index_buff)
    .set_data(index_data)
    .set_size(sizeof(index_data));

  ctx->submitDisplayList(std::move(dl));
}

void finish(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {
}

void render(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {
  Mat4 proj;
  Mat4 view;

  gb_mat4_perspective((gbMat4*)&proj, gb_to_radians(45.f), sapp_width()/(float)sapp_height(), 0.05f, 900.0f);
  gb_mat4_look_at((gbMat4*)&view, {0.f,0.0f,3.f}, {0.f,0.f,0.0f}, {0.f,1.f,0.f});

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
  dl.setupPipelineCommand()
    .set_pipeline(State.material)
    .set_buffer(0,State.vertex_buff)
    ;
  dl.renderCommand()
    .set_index_buffer(State.index_buff)
    .set_count(3)
    .set_type(IndexFormat::UInt16)
    ;
  
  ImGui::NewFrame();
  ImGui::ShowDemoWindow();
  ImGui::Render();
  ImGui_Impl_pxrender_RenderDrawData(ImGui::GetDrawData(), &dl);

  ctx->submitDisplayListAndSwap(std::move(dl));
}
