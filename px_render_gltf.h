/* -----------------------------------------------------------------------------
Copyright (c) 2018 Jose L. Hidalgo (PpluX)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
----------------------------------------------------------------------------- */

// USAGE
//
// In *ONE* C++ file you need to declare
// #define PX_RENDER_GLTF_IMPLEMENTATION
// before including the file that contains px_render_gltf.h
// 
// px_render_gltf must be included *AFTER* px_render and tiny_gltf.h (when expanding
// the implementation)

#ifndef PX_RENDER_GLTF
#define PX_RENDER_GLTF

#ifndef PX_RENDER
#error px_render must be included before px_render_gltf (because gltf plugin does not include px_render.h)
#endif

namespace px_render {

  struct GLTF {
    struct Flags {
      enum Enum {
        Geometry_Position   = 1<<0, // Vec3f
        Geometry_Normal     = 1<<1, // Vec3f
        Geometry_TexCoord0  = 1<<2, // Vec2f
        Material_DiffuseTex = 1<<3,
      };
    };

    void init(RenderContext *_ctx, const tinygltf::Model &_model);
    void freeResources();
    ~GLTF() { freeResources(); }

    struct Primitive {
      uint32_t flags; // GLTF::Flags mask (identify what attributes the geometry has)
      uint32_t node;
      uint32_t mesh;
      uint32_t vertex_buff;
      uint32_t index_buff;
      uint32_t index_offset;
      uint32_t index_count;
      uint32_t diffuse_texture;
    };

    struct Node {
      Mat4 transform; // transformation relative to its parent
      Mat4 model;     // model->world transform
      uint32_t parent = 0;
    };

    RenderContext *ctx = nullptr;
    std::unique_ptr<Buffer[]> buffers;
    std::unique_ptr<Node[]> nodes;
    std::unique_ptr<Primitive[]> primitives;
    std::unique_ptr<Texture[]> textures;
  };
  
}

#endif //PX_RENDER_GLTF


//----------------------------------------------------------------------------
#if defined(PX_RENDER_GLTF_IMPLEMENTATION) && !defined(PX_RENDER_GLTF_IMPLEMENTATION_DONE)

#define PX_RENDER_GTLF_IMPLEMENTATION_DONE
#include <functional>

#ifndef TINY_GLTF_H_
#error tiny_gltf must be included before px_render_gltf (because gltf plugin does not include tiny_gltf.h)
#endif // PX_RENDER_GLTF_IMPLEMENTATION

namespace px_render {

  namespace GLTF_Imp {

    using NodeTraverseFunc = std::function<void(const tinygltf::Model &model, uint32_t raw_node_pos, uint32_t raw_parent_node_pos)>;

    static void NodeTraverse(
        const tinygltf::Model &model,
        uint32_t raw_node_pos,
        uint32_t raw_parent_node_pos,
        NodeTraverseFunc func) {
      func(model, raw_node_pos, raw_parent_node_pos);
      const tinygltf::Node &node = model.nodes[raw_node_pos];
      for (uint32_t i = 0; i < node.children.size(); ++i) {
        NodeTraverse(model, node.children[i], raw_node_pos, func);
      }
    }

    static const uint32_t InvalidNode = (uint32_t)-1;

    static void NodeTraverse(const tinygltf::Model &model, NodeTraverseFunc func) {
      const tinygltf::Scene &scene = model.scenes[model.defaultScene];
      for (uint32_t i = 0; i < scene.nodes.size(); ++i) {
        NodeTraverse(model, scene.nodes[i], InvalidNode, func);
      }
    }

  }

  void GLTF::init(RenderContext *_ctx, const tinygltf::Model &model) {
    freeResources();
    ctx = _ctx;
    DisplayList dl;
    // Buffers
    const size_t num_buffers = model.bufferViews.size();
    buffers = std::unique_ptr<Buffer[]>(new Buffer[num_buffers]);
    for (size_t i = 0; i < num_buffers; i++) {
      const tinygltf::BufferView &buffer_view = model.bufferViews[i];
      const tinygltf::Buffer &buffer = model.buffers[buffer_view.buffer];
      BufferType::Enum type;
      switch (buffer_view.target) {
        case TINYGLTF_TARGET_ARRAY_BUFFER:
          type = BufferType::Vertex;
          break;
        case TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER:
          type = BufferType::Index;
          break;
        default:
          assert(!"Invalid target");
      }
      buffers[i] = ctx->createBuffer({type,buffer.data.size(), Usage::Static});
      dl.fillBufferCommand()
        .set_buffer(buffers[i])
        .set_data(&buffer.data.at(0)+buffer_view.byteOffset)
        .set_size(buffer_view.byteLength);
    }
    // nodes 1st pass, count number of nodes+primitives
    uint32_t total_nodes = 1; // always add one artificial root node
    uint32_t total_primitives = 0;
    GLTF_Imp::NodeTraverse(model, [&total_nodes, &total_primitives](const tinygltf::Model model, uint32_t n_pos, uint32_t p_pos) {
      const tinygltf::Node &n = model.nodes[n_pos];
      total_nodes++;
      if (n.mesh >= 0) {
        const tinygltf::Mesh &mesh = model.meshes[n.mesh];
        for(size_t i = 0; i < mesh.primitives.size(); ++i) {
          const tinygltf::Primitive &primitive = mesh.primitives[i];
          if (primitive.indices >= 0) {
            total_primitives++;
          }
        }
      }
    });
    // nodes 2nd pass, gather info
    nodes = std::unique_ptr<Node[]>(new Node[total_nodes]);
    primitives = std::unique_ptr<Primitive[]>(new Primitive[total_primitives]);
    nodes[0].model = nodes[0].transform = Mat4::Identity();
    nodes[0].parent = 0; 
    auto node_map = std::unique_ptr<uint32_t[]>(new uint32_t[model.nodes.size()]);
    uint32_t current_node = 1;
    uint32_t current_primitive = 0;
    GLTF_Imp::NodeTraverse(model,
      [&current_node, &current_primitive, &node_map, total_nodes, total_primitives, this]
      (const tinygltf::Model &model, uint32_t n_pos, uint32_t p_pos) mutable {
        const tinygltf::Node &gltf_n = model.nodes[n_pos];
        Node &node = nodes[current_node];
        // gather node transform or compute it
        if (gltf_n.matrix.size() == 16) {
          for(size_t i = 0; i < 16; ++i) node.transform.f[i] = gltf_n.matrix[i];
        } else {
          Vec3 s = {1.0f, 1.0f, 1.0f};
          Vec4 r = {0.0f, 0.0f, 0.0f, 0.0f};
          Vec3 t = {0.0f, 0.0f, 0.0f};
          if (gltf_n.scale.size() == 3) {
            s = Vec3{
              (float) gltf_n.scale[0],
              (float) gltf_n.scale[1],
              (float) gltf_n.scale[2]};
          }
          if (gltf_n.rotation.size() == 4) {
            r = Vec4{
              (float) gltf_n.rotation[1], // axis-x
              (float) gltf_n.rotation[2], // axis-y
              (float) gltf_n.rotation[3], // axis-z
              (float) gltf_n.rotation[0]}; // angle
          }
          if (gltf_n.translation.size() == 3) {
            t = Vec3{
              (float) gltf_n.translation[0],
              (float) gltf_n.translation[1],
              (float) gltf_n.translation[2]};
          }
          node.transform = Mat4::SRT(s,r,t);
        }
        // compute node-parent relationship
        node_map[n_pos] = current_node;
        if (p_pos != GLTF_Imp::InvalidNode) {
          node.parent = node_map[p_pos];
          node.model = Mat4::Mult(nodes[node.parent].model, node.transform);
        } else {
          node.parent = 0;
          node.model = node.transform;
        }
        // gather primitive info 
        if (gltf_n.mesh >= 0) {
          const tinygltf::Mesh &mesh = model.meshes[gltf_n.mesh];
          for(size_t i = 0; i < mesh.primitives.size(); ++i) {
            const tinygltf::Primitive &gltf_p = mesh.primitives[i];
            if (gltf_p.indices >= 0) {
              Primitive &primitive = primitives[current_primitive];
              primitive.node = current_node;
              primitive.mesh = gltf_n.mesh; // maybe a mesh counter would be better
              
              for (const auto &atrib : gltf_p.attributes) {

              }
              
              current_primitive++;
            }
          }
        }



        current_node++;
    });


    // submit all data transactions
    ctx->submitDisplayList(std::move(dl));
  }

  void GLTF::freeResources() {
    // TODO
  }
}

#endif // PX_RENDER_GLTF_IMPLEMENTATION