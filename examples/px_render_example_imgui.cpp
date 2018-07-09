#include "common/render_common.h"
// demo
#include "deps/imgui_demo.cpp"

void init(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {
}

void finish(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {
}

void render(px_render::RenderContext *ctx, px_sched::Scheduler *sched) {
  px_render::DisplayList dl;
  dl.setupViewCommand()
    .set_viewport({0,0, (uint16_t) sapp_width(), (uint16_t) sapp_height()})
    ;
  dl.clearCommand()
    .set_clear_color(true)
    .set_clear_depth(true)
    .set_color({0.1f,0.2f,0.1f, 1.0f})
  ;
  
  ImGui::NewFrame();
  ImGui::ShowDemoWindow();
  ImGui::Render();
  ImGui_Impl_pxrender_RenderDrawData(ImGui::GetDrawData(), &dl);

  ctx->submitDisplayListAndSwap(std::move(dl));
}
