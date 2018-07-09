// Emscripten config
#ifdef __EMSCRIPTEN__
// Compile with  emcc --std=c++14 -s USE_WEBGL2=1 px_render_example_rtt.cpp -o rtt.html
#  define SOKOL_GLES3
#  define PX_RENDER_BACKEND_GLES
#else
#  define PX_RENDER_BACKEND_GL //< Default
#  define SOKOL_WIN32_NO_GL_LOADER
#  define SOKOL_GLCORE33
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

#define PX_SCHED_IMPLEMENTATION
#include "../px_sched.h"

#include "deps/imgui.h"
#include "deps/imgui.cpp"
#include "deps/imgui_draw.cpp"
#include "deps/imgui_impl_pxrender.h"
#include "deps/imgui_impl_pxrender.cpp"
// demo
#include "deps/imgui_demo.cpp"


struct {
  px_render::RenderContext ctx;
  px_sched::Scheduler sched;
  px_sched::Sync frame;
  ImGuiContext *imgui_ctx = nullptr;
  bool btn_down[SAPP_MAX_MOUSEBUTTONS];
  bool btn_up[SAPP_MAX_MOUSEBUTTONS];
} State;


void setup_input() {
  auto& io = ImGui::GetIO();
  io.KeyMap[ImGuiKey_Tab] = SAPP_KEYCODE_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = SAPP_KEYCODE_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = SAPP_KEYCODE_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = SAPP_KEYCODE_UP;
  io.KeyMap[ImGuiKey_DownArrow] = SAPP_KEYCODE_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = SAPP_KEYCODE_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = SAPP_KEYCODE_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = SAPP_KEYCODE_HOME;
  io.KeyMap[ImGuiKey_End] = SAPP_KEYCODE_END;
  io.KeyMap[ImGuiKey_Delete] = SAPP_KEYCODE_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = SAPP_KEYCODE_BACKSPACE;
  io.KeyMap[ImGuiKey_Space] = SAPP_KEYCODE_SPACE;
  io.KeyMap[ImGuiKey_Enter] = SAPP_KEYCODE_ENTER;
  io.KeyMap[ImGuiKey_Escape] = SAPP_KEYCODE_ESCAPE;
  io.KeyMap[ImGuiKey_A] = SAPP_KEYCODE_A;
  io.KeyMap[ImGuiKey_C] = SAPP_KEYCODE_C;
  io.KeyMap[ImGuiKey_V] = SAPP_KEYCODE_V;
  io.KeyMap[ImGuiKey_X] = SAPP_KEYCODE_X;
  io.KeyMap[ImGuiKey_Y] = SAPP_KEYCODE_Y;
  io.KeyMap[ImGuiKey_Z] = SAPP_KEYCODE_Z;
}

void init() {
  State.sched.init();
  #ifdef PX_RENDER_BACKEND_GL
    gladLoadGL();
  #endif
  State.ctx.init();
  State.imgui_ctx = ImGui::CreateContext();
  ImGui_Impl_pxrender_Init(&State.ctx);
  setup_input();
}

void gui() {
  ImGui::ShowDemoWindow();
}

void render() {
  px_render::DisplayList dl;
  dl.setupViewCommand()
    .set_viewport({0,0, (uint16_t) sapp_width(), (uint16_t) sapp_height()})
    ;
  dl.clearCommand()
    .set_clear_color(true)
    .set_clear_depth(true)
    .set_color({0.1f,0.2f,0.1f, 1.0f})
  ;

  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = {(float)sapp_width(), (float)sapp_height()};

  for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++) {
    if (State.btn_down[i]) {
      State.btn_down[i] = false;
      io.MouseDown[i] = true;
    }
    else if (State.btn_up[i]) {
      State.btn_up[i] = false;
      io.MouseDown[i] = false;
    }
  }
  
  ImGui::NewFrame();
  gui();
  ImGui::Render();

  ImGui_Impl_pxrender_RenderDrawData(ImGui::GetDrawData(), &dl);
  State.ctx.submitDisplayListAndSwap(std::move(dl));
}

void frame() {
  State.sched.waitFor(State.frame);
  State.sched.run({render}, &State.frame);
  // GPU Render...
  for(;;) {
    px_render::RenderContext::Result::Enum result = State.ctx.executeOnGPU();
    if (result != px_render::RenderContext::Result::OK) break;
  };
}

void cleanup() {
  ImGui_Impl_pxrender_Shutdown();
  State.ctx.finish();
}

void input(const sapp_event* event) {
  auto& io = ImGui::GetIO();
  io.KeyAlt = (event->modifiers & SAPP_MODIFIER_ALT) != 0;
  io.KeyCtrl = (event->modifiers & SAPP_MODIFIER_CTRL) != 0;
  io.KeyShift = (event->modifiers & SAPP_MODIFIER_SHIFT) != 0;
  io.KeySuper = (event->modifiers & SAPP_MODIFIER_SUPER) != 0;
  switch (event->type) {
  case SAPP_EVENTTYPE_MOUSE_DOWN:
    io.MousePos.x = event->mouse_x;
    io.MousePos.y = event->mouse_y;
    State.btn_down[event->mouse_button] = true;
    break;
  case SAPP_EVENTTYPE_MOUSE_UP:
    io.MousePos.x = event->mouse_x;
    io.MousePos.y = event->mouse_y;
    State.btn_up[event->mouse_button] = true;
    break;
  case SAPP_EVENTTYPE_MOUSE_MOVE:
    io.MousePos.x = event->mouse_x;
    io.MousePos.y = event->mouse_y;
    break;
  case SAPP_EVENTTYPE_MOUSE_ENTER:
  case SAPP_EVENTTYPE_MOUSE_LEAVE:
    for (int i = 0; i < 3; i++) {
      State.btn_down[i] = false;
      State.btn_up[i] = false;
      io.MouseDown[i] = false;
    }
    break;
  case SAPP_EVENTTYPE_MOUSE_SCROLL:
    io.MouseWheelH = event->scroll_x;
    io.MouseWheel = event->scroll_y;
    break;
  case SAPP_EVENTTYPE_TOUCHES_BEGAN:
    State.btn_down[0] = true;
    io.MousePos.x = event->touches[0].pos_x;
    io.MousePos.y = event->touches[0].pos_y;
    break;
  case SAPP_EVENTTYPE_TOUCHES_MOVED:
    io.MousePos.x = event->touches[0].pos_x;
    io.MousePos.y = event->touches[0].pos_y;
    break;
  case SAPP_EVENTTYPE_TOUCHES_ENDED:
    State.btn_up[0] = true;
    io.MousePos.x = event->touches[0].pos_x;
    io.MousePos.y = event->touches[0].pos_y;
    break;
  case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
    State.btn_up[0] = State.btn_down[0] = false;
    break;
  case SAPP_EVENTTYPE_KEY_DOWN:
    io.KeysDown[event->key_code] = true;
    break;
  case SAPP_EVENTTYPE_KEY_UP:
    io.KeysDown[event->key_code] = false;
    break;
  case SAPP_EVENTTYPE_CHAR:
    io.AddInputCharacter((ImWchar)event->char_code);
    break;
  default:
    break;
  }
}

sapp_desc sokol_main(int argc, char **argv) {
  sapp_desc d = {};
  d.init_cb = init;
  d.frame_cb = frame;
  d.cleanup_cb = cleanup;
  d.event_cb = input;
  d.width = 1024;
  d.height = 768;
  d.window_title = "PX-Render Test";
  setup_input;
  return d;
}
