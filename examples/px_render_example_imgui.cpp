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
// demo
#include "deps/imgui_demo.cpp"

namespace px_render {
  class ImGui {
  public:
    void init(RenderContext *ctx);
    void finish();
    void beginFrame(uint16_t window_width, uint16_t window_height);
    DisplayList endFrame();

    ~ImGui() {
      assert(ctx_ == nullptr && imgui_ctx_ == nullptr && "destroy() not called");
    }

  private: 
    RenderContext *ctx_ = nullptr;
    ImGuiContext *imgui_ctx_ = nullptr;
    Pipeline pipeline_;
    Texture font_;
    Buffer vertex_;
    Buffer index_;
    uint32_t vertex_size_ = 0;
    uint32_t index_size_ = 0;
  };
}

namespace px_render {

  void ImGui::init(RenderContext *_ctx) {
    assert(ctx_ == nullptr && imgui_ctx_ == nullptr && "init() called twice");
    ctx_ = _ctx;
    imgui_ctx_ = ::ImGui::CreateContext();

    // -- Create Pipeline Object ---------------------------------------------
    px_render::Pipeline::Info pinfo;
    #define GLSL(...) "#version 330\n" #__VA_ARGS__
    pinfo.shader.vertex = GLSL(
      uniform mat4 u_projection;
      in vec2 pos;
      in vec2 uv;
      in vec4 color;
      out vec2 frag_uv;
      out vec4 frag_color;
      void main() {
        frag_uv = uv;
        frag_color = color;
        gl_Position = u_projection*vec4(pos, 0.0, 1.0);
      } 
    );
    pinfo.shader.fragment = GLSL(
      uniform sampler2D u_tex0;
      in vec2 frag_uv;
      in vec4 frag_color;
      out vec4 color;
      void main() {
        color = vec4(frag_color.rgb, texture(u_tex0, frag_uv).r*frag_color.a);
      }
    );
    #undef GLSL
    pinfo.attribs[0] = {"pos", VertexFormat::Float2};
    pinfo.attribs[1] = {"uv", VertexFormat::Float2};
    pinfo.attribs[2] = {"color", VertexFormat::UInt8 | VertexFormat::NumComponents4 | VertexFormat::Normalized};
    pinfo.textures[0] = TextureType::T2D;
    pinfo.blend.enabled = true;
    pinfo.blend.op_rgb = pinfo.blend.op_alpha = BlendOp::Add;
    pinfo.blend.src_rgb = pinfo.blend.src_alpha = BlendFactor::SrcAlpha;
    pinfo.blend.dst_rgb = pinfo.blend.dst_alpha = BlendFactor::OneMinusSrcAlpha;
    pinfo.cull = Cull::Disabled;
    pinfo.depth_func = CompareFunc::Always;
    pinfo.depth_write = false;
    pipeline_ = ctx_->createPipeline(pinfo);

    // -- Default font texture -----------------------------------------------
    int font_width, font_height;
    unsigned char *font_pixels;
    ::ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&font_pixels, &font_width, &font_height);
    Texture::Info tinfo;
    tinfo.width = font_width;
    tinfo.height = font_height;
    tinfo.format = TexelsFormat::R_U8;
    font_ = ctx_->createTexture(tinfo);
    DisplayList tex_dl;
    tex_dl.fillTextureCommand()
      .set_texture(font_)
      .set_data(font_pixels);
    ctx_->submitDisplayList(std::move(tex_dl));
    ::ImGui::GetIO().Fonts->TexID = (void*)&font_;

  }

  void ImGui::finish() {
    ::ImGui::DestroyContext(imgui_ctx_);
    imgui_ctx_ = nullptr;
    ctx_ = nullptr;
  }

  void ImGui::beginFrame(uint16_t window_width, uint16_t window_height) {
    ::ImGui::SetCurrentContext(imgui_ctx_);
    ::ImGui::GetIO().DisplaySize = {(float)window_width, (float)window_height};
    ::ImGui::NewFrame();
  }

  DisplayList ImGui::endFrame() {
    DisplayList dl;
    ::ImGui::Render();
    ImDrawData* draw_data = ::ImGui::GetDrawData();
    ImGuiIO& io = ::ImGui::GetIO();

    int fb_width = (int)(draw_data->DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) return dl;
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    const px_render::Mat4 proj =
    {
       2.0f/(R-L),   0.0f,         0.0f,   0.0f ,
       0.0f,         2.0f/(T-B),   0.0f,   0.0f ,
       0.0f,         0.0f,        -1.0f,   0.0f ,
       (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f 
    };
    dl.setupViewCommand()
      .set_projection_matrix(proj)
      .set_viewport({0,0, (uint16_t) fb_width, (uint16_t) fb_height});
      ;
    
    ImVec2 pos = draw_data->DisplayPos;

    for(int n = 0; n < draw_data->CmdListsCount; ++n) {
      const ImDrawList *cmd_list = draw_data->CmdLists[n];
      size_t required_vertex_size = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
      if (required_vertex_size > vertex_size_) {
        dl.destroy(vertex_);
        vertex_ = ctx_->createBuffer({BufferType::Vertex, required_vertex_size, Usage::Stream});
        vertex_size_ = required_vertex_size;
      }
      dl.fillBufferCommand()
        .set_buffer(vertex_)
        .set_data(cmd_list->VtxBuffer.Data)
        .set_size(required_vertex_size)
        ;
      size_t required_index_size = cmd_list->IdxBuffer.Size *sizeof(ImDrawIdx);
      if (required_index_size > index_size_) {
        dl.destroy(index_);
        index_ = ctx_->createBuffer({BufferType::Index, required_index_size, Usage::Stream});
        index_size_ = required_index_size;
      }
      dl.fillBufferCommand()
        .set_buffer(index_)
        .set_data(cmd_list->IdxBuffer.Data)
        .set_size(required_index_size)
        ;
      uint32_t idx_buffer_offset = 0;
      for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i) {
        const ImDrawCmd *cmd = &cmd_list->CmdBuffer[cmd_i];
        if (cmd->UserCallback) {
          cmd->UserCallback(cmd_list, cmd);
          continue;
        }
      
       Vec4 clip_rect = { cmd->ClipRect.x - pos.x, cmd->ClipRect.y - pos.y, cmd->ClipRect.z - pos.x, cmd->ClipRect.w - pos.y};
       if (clip_rect.v.x < fb_width && clip_rect.v.y < fb_height && clip_rect.v.z >= 0.0f && clip_rect.v.w >= 0.0f) {
        // invert y on scissor
         Vec4 scissor_rect = { clip_rect.f[0], fb_height - clip_rect.f[3], clip_rect.f[2] - clip_rect.f[0], clip_rect.f[3] - clip_rect.f[1]};
         dl.setupPipelineCommand()
          .set_pipeline(pipeline_)
          .set_buffer(0, vertex_)
          .set_texture(0, *(Texture*)cmd->TextureId)
          .set_scissor(scissor_rect)
          ;
         dl.renderCommand()
          .set_index_buffer(index_)
          .set_offset(sizeof(ImDrawIdx)*idx_buffer_offset)
          .set_count(cmd->ElemCount)
          .set_type(IndexFormat::UInt16)
          ;
         idx_buffer_offset += cmd->ElemCount;
       }
      }
      
    }

    return dl;
  }

}

struct {
  px_render::RenderContext ctx;
  px_render::ImGui gui;
  px_sched::Scheduler sched;
  px_sched::Sync frame;
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
  State.gui.init(&State.ctx);
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
  State.ctx.submitDisplayList(std::move(dl));

  ImGuiIO &io = ImGui::GetIO();
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

  State.gui.beginFrame(sapp_width(), sapp_height());
  gui();
  State.ctx.submitDisplayListAndSwap(State.gui.endFrame());
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
  State.gui.finish();
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
