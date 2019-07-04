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

#include <cstdint>

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
        Geometry_Tangent    = 1<<3, // Vec3f
        Material            = 1<<10,
        ComputeBounds       = 1<<11,
        All = 0xFFFFFFFF
      };
    };

    void init(RenderContext *_ctx, const tinygltf::Model &_model, uint32_t import_flags = Flags::All);
    void freeResources();
    ~GLTF() { freeResources(); }

    struct Primitive {
      uint32_t node = 0;
      uint32_t mesh = 0;
      uint32_t index_offset = 0; // in uint32_t units
      uint32_t index_count = 0;
      int32_t material = -1;
      Vec3 bounds_min = {0.0f, 0.0f, 0.0f}; // bounds in world coordinates
      Vec3 bounds_max = {0.0f, 0.0f, 0.0f}; // bounds in world coordinates
    };

    struct Node {
      Mat4 transform; // transformation relative to its parent
      Mat4 model;     // model->world transform
      uint32_t parent = 0;
    };

    struct Material {
      std::string name;
      struct {
        int32_t texture = -1;
        Vec4 factor = {1.0f, 1.0f, 1.0f, 1.0f};
      } base_color;
      struct {
        int32_t texture = -1;
        float metallic_factor = 0.5f;
        float roughness_factor = 0.5f;
      } metallic_roughness;
      struct {
        int32_t texture = -1;
        float factor = 1.0f; // normal-scale
      } normal;
      struct {
        int32_t texture = -1;
        float factor = 1.0f; // occlusion-strength
      } occlusion;
      struct {
        int32_t texture = -1;
        Vec3 factor = {1.0f, 1.0f, 1.0f};
      } emmisive;
    };

    struct Texture {
      std::string uri;
      px_render::Texture::Info info;
      px_render::Texture tex;
    };

    RenderContext *ctx = nullptr;
    Buffer vertex_buffer;
    Buffer index_buffer;
    uint32_t num_primitives = 0;
    uint32_t num_nodes = 0;
    uint32_t num_materials = 0;
    uint32_t num_textures = 0;
    std::unique_ptr<Node[]> nodes;
    std::unique_ptr<Primitive[]> primitives;
    std::unique_ptr<Texture[]> textures;
    std::unique_ptr<Material[]> materials;
    Vec3 bounds_min = {0.0f, 0.0f, 0.0f}; // bounds in world coordinates
    Vec3 bounds_max = {0.0f, 0.0f, 0.0f}; // bounds in world coordinates
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
    using IndexTraverseFunc = std::function<void(const tinygltf::Model &model, uint32_t index)>;

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
      int scene_index = std::max(model.defaultScene, 0);
      const tinygltf::Scene &scene = model.scenes[scene_index];
      for (uint32_t i = 0; i < scene.nodes.size(); ++i) {
        NodeTraverse(model, scene.nodes[i], InvalidNode, func);
      }
    }

    static void IndexTraverse(const tinygltf::Model &model, const tinygltf::Accessor &index_accesor, IndexTraverseFunc func) {
      const tinygltf::BufferView &buffer_view = model.bufferViews[index_accesor.bufferView];
      const tinygltf::Buffer &buffer = model.buffers[buffer_view.buffer];
      const uint8_t* base = &buffer.data.at(buffer_view.byteOffset + index_accesor.byteOffset);
      switch (index_accesor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
          const uint32_t *p = (uint32_t*) base;
          for (size_t i = 0; i < index_accesor.count; ++i) {
            func(model, p[i]);
          }
        }; break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
          const uint16_t *p = (uint16_t*) base;
          for (size_t i = 0; i < index_accesor.count; ++i) {
            func(model, p[i]);
          }
        }; break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
          const uint8_t *p = (uint8_t*) base;
          for (size_t i = 0; i < index_accesor.count; ++i) {
            func(model, p[i]);
          }
        }; break;
      }
    }

    static void ExtractVertexData(uint32_t v_pos, const uint8_t *base, int accesor_componentType, int accesor_type, bool accesor_normalized, uint32_t byteStride, float *output, uint8_t max_num_comp) {
      float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      uint32_t ncomp = 1;
      switch (accesor_type) {
        case TINYGLTF_TYPE_SCALAR: ncomp = 1; break;
        case TINYGLTF_TYPE_VEC2:   ncomp = 2; break;
        case TINYGLTF_TYPE_VEC3:   ncomp = 3; break;
        case TINYGLTF_TYPE_VEC4:   ncomp = 4; break;
        default:
          assert(!"invalid type");
      }
      switch (accesor_componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
          const float *data = (float*)(base+byteStride*v_pos);
          for (uint32_t i = 0; (i < ncomp); ++i) {
            v[i] = data[i];
          }
        }
        // TODO SUPPORT OTHER FORMATS
        break;
        default:
          assert(!"Conversion Type from float to -> ??? not implemented yet");
          break;
      }
      for (uint32_t i = 0; i < max_num_comp; ++i) {
        output[i] = v[i];
      }
    }

    struct MaterialCache {
      // matp between GLTF materials, and px_render materials
      std::map<uint32_t, uint32_t> index;
      std::vector<GLTF::Texture> textures;
      std::vector<GLTF::Material> materials;
      std::map<std::pair<int,int>, uint32_t> texture_index;
      px_render::DisplayList texture_dl;

      auto find(const tinygltf::Model &model, const tinygltf::Material &mat, const char *name, bool *found) -> decltype(mat.values.find(name)) {
        *found = true;
        auto const &i = mat.values.find(name);
        if (i != mat.values.end()) return i;
        auto const &ai = mat.additionalValues.find(name);
        if (ai != mat.additionalValues.end()) return ai;
        *found = false;
        return mat.values.end();
      }

      static SamplerFiltering::Enum TranslateFiltering(int i) {
        switch (i) {
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
          return SamplerFiltering::Linear;
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
          return SamplerFiltering::Nearest;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
          return SamplerFiltering::LinearMipmapLinear;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
          return SamplerFiltering::LinearMipmapNearest;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
          return SamplerFiltering::NearestMipmapLinear;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
          return SamplerFiltering::NearestMipmapNearest;
        }
        return SamplerFiltering::Linear; // (default?)

      }

      static SamplerWrapping::Enum TranslateWrapping(int i) {
        switch (i) {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
          return SamplerWrapping::Clamp;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
          return SamplerWrapping::MirroredRepeat;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
          return SamplerWrapping::Repeat;
        }
        return SamplerWrapping::Clamp; // (default?)
      }

      int32_t texture(px_render::RenderContext *ctx, const tinygltf::Model &model, const tinygltf::Material &mat, const char *name) {
        bool found;
        auto i = find(model, mat, name, &found);
        if (found) {
          const auto &j = i->second.json_double_value.find("index");
          if (j != i->second.json_double_value.end()) {
            int gltf_texture_pos = (int) j->second;
            assert(gltf_texture_pos >= 0 && "Invalid texture index");
            // extract texture data from model, first, then search for
            // existing texture in array
            const tinygltf::Texture &tex = model.textures[gltf_texture_pos];
            int sampler_idx = tex.sampler;
            int source_idx = tex.source;
            auto found = texture_index.find(std::make_pair(source_idx, sampler_idx));
            if (found != texture_index.end()) {
              return found->second;
            }
            // new one
            GLTF::Texture new_texture;
            const tinygltf::Image &image = model.images[source_idx];
            new_texture.info.width = image.width;
            new_texture.info.height = image.height;
            new_texture.info.depth = 1;
            new_texture.info.type = TextureType::T2D;
            new_texture.info.usage = Usage::Static;

            switch(image.component) {
              case 1: new_texture.info.format = px_render::TexelsFormat::R_U8; break;
              case 2: new_texture.info.format = px_render::TexelsFormat::RG_U8; break;
              case 3: new_texture.info.format = px_render::TexelsFormat::RGB_U8; break;
              case 4: new_texture.info.format = px_render::TexelsFormat::RGBA_U8; break;
              default:
                assert(!"Invalid format...");
            }

            if (model.samplers.size() > 0)
            {
                const tinygltf::Sampler &sampler = model.samplers[sampler_idx]; 
                new_texture.info.magnification_filter = TranslateFiltering(sampler.magFilter);
                new_texture.info.minification_filter = TranslateFiltering(sampler.minFilter);
                new_texture.info.wrapping[0] = TranslateWrapping(sampler.wrapR);
                new_texture.info.wrapping[1] = TranslateWrapping(sampler.wrapS);
                new_texture.info.wrapping[2] = TranslateWrapping(sampler.wrapT);
            }

            new_texture.tex = ctx->createTexture(new_texture.info);
            texture_dl.fillTextureCommand().set_texture(new_texture.tex).set_data(&image.image[0]).set_build_mipmap(true);
            texture_dl.commitLastCommand();

            // store
            int32_t new_texture_index = textures.size();
            textures.push_back(std::move(new_texture));
            texture_index[std::make_pair(source_idx, sampler_idx)] = new_texture_index;
            return new_texture_index;
          }
        }
        return -1;
      }

      float texture_scale(const tinygltf::Model &model, const tinygltf::Material &mat, const char *name) {
        bool found;
        auto i = find(model, mat, name, &found);
        if (found) {
          const auto &j = i->second.json_double_value.find("scale");
          if (j != i->second.json_double_value.end()) {
            return j->second;
          }
        }
        return 1.0f;
      }
      Vec4 factor4(const tinygltf::Model &model, const tinygltf::Material &mat, const char *name, float df) {
        bool found;
        auto i = find(model, mat, name, &found);
        if (found) {
          auto cf = i->second.ColorFactor();
          return Vec4 { 
            (float)cf[0], (float)cf[1], (float)cf[2], (float)cf[3],
          };
        }
        return Vec4{df,df,df,df};
      }
      Vec3 factor3(const tinygltf::Model &model, const tinygltf::Material &mat, const char *name, float df) {
        bool found;
        auto i = find(model, mat, name, &found);
        if (found) {
          auto cf = i->second.ColorFactor();
          return Vec3 { 
            (float)cf[0], (float)cf[1], (float)cf[2]
          };
        }
        return Vec3{df,df,df};
      }
      float factor1(const tinygltf::Model &model, const tinygltf::Material &mat, const char *name, float df) {
        bool found;
        auto i = find(model, mat, name, &found);
        if (found) {
          return i->second.Factor();
        }
        return df;
      }


      void load(px_render::RenderContext*ctx, const tinygltf::Model &model, int material_index) {
        if (index.find(material_index) != index.end()) return;
        const tinygltf::Material &mat = model.materials[material_index];
        GLTF::Material material;
        material.name = mat.name;
        material.base_color.texture = texture(ctx, model, mat, "baseColorTexture");
        material.base_color.factor = factor4(model, mat, "baseColorFactor", 1.0f);
        material.emmisive.texture = texture(ctx, model, mat, "emissiveTexture");
        material.emmisive.factor = factor3(model, mat, "emissiveFactor",0.0f);
        material.metallic_roughness.texture = texture(ctx, model, mat, "metallicRoughnessTexture");
        material.metallic_roughness.metallic_factor = factor1(model, mat, "metallicFactor", 0.5f);
        material.metallic_roughness.roughness_factor = factor1(model, mat, "roughnessFactor", 0.5f);
        material.normal.texture = texture(ctx, model,mat, "normalTexture");
        material.normal.factor = texture_scale(model, mat, "normalTexture");

        index[material_index] = materials.size();
        materials.push_back(std::move(material));
      }
    }; // Material Cache
  } // GLTF_Imp

  void GLTF::init(RenderContext *_ctx, const tinygltf::Model &model, uint32_t flags) {
    freeResources();
    ctx = _ctx;
    // nodes 1st pass, count number of nodes+primitives
    uint32_t total_nodes = 1; // always add one artificial root node
    uint32_t total_primitives = 0;
    uint32_t total_num_vertices = 0;
    uint32_t total_num_indices = 0;
    GLTF_Imp::MaterialCache material_cache;

    uint32_t const vertex_size = 0
      + (flags&Flags::Geometry_Position?  sizeof(float)*3: 0)
      + (flags&Flags::Geometry_Normal?    sizeof(float)*3: 0)
      + (flags&Flags::Geometry_TexCoord0? sizeof(float)*2: 0)
      + (flags&Flags::Geometry_Tangent?   sizeof(float)*4: 0)
      ;

    GLTF_Imp::NodeTraverse(model,
      [&total_nodes, &total_primitives, &total_num_vertices, &total_num_indices, flags, _ctx, &material_cache]
      (const tinygltf::Model model, uint32_t n_pos, uint32_t p_pos) {
      const tinygltf::Node &n = model.nodes[n_pos];
      total_nodes++;
      if (n.mesh >= 0) {
        const tinygltf::Mesh &mesh = model.meshes[n.mesh];
        for(size_t i = 0; i < mesh.primitives.size(); ++i) {
          const tinygltf::Primitive &primitive = mesh.primitives[i];
          if (primitive.indices >= 0) {
            uint32_t min_vertex_index = (uint32_t)-1;
            uint32_t max_vertex_index = 0;
            // TODO: It would be nice to have a cache of index accessors (key=pirmitive.indices)
            // that way if two or more geometries are identical (because they use the same index buffer)
            // then the can reuse the same vertex data. Currently, vertex data is extracted for every
            // primitive....
            GLTF_Imp::IndexTraverse(model, model.accessors[primitive.indices],
              [&min_vertex_index, &max_vertex_index, &total_num_vertices, &total_num_indices]
              (const tinygltf::Model&, uint32_t index) {
                min_vertex_index = std::min(min_vertex_index, index);
                max_vertex_index = std::max(max_vertex_index, index);
                total_num_indices++;
            });
            total_num_vertices += (max_vertex_index - min_vertex_index +1);

            if (flags & Flags::Material) {
              if (primitive.material >= 0) {
                material_cache.load(_ctx, model, primitive.material);
              }
            }

            total_primitives++;
          }
        }
      }
    });

    nodes = std::unique_ptr<Node[]>(new Node[total_nodes]);
    primitives = std::unique_ptr<Primitive[]>(new Primitive[total_primitives]);
    std::unique_ptr<float[]> vertex_data {new float[total_num_vertices*vertex_size/sizeof(float)]};
    std::unique_ptr<uint32_t[]> index_data {new uint32_t[total_num_indices]};

    // fill with 0s vertex data
    memset(vertex_data.get(), 0, sizeof(float)*total_num_vertices);

    // nodes 2nd pass, gather info
    nodes[0].model = nodes[0].transform = Mat4::Identity();
    nodes[0].parent = 0; 
    auto node_map = std::unique_ptr<uint32_t[]>(new uint32_t[model.nodes.size()]);
    uint32_t current_node = 1;
    uint32_t current_primitive = 0;
    uint32_t current_mesh = 0;
    uint32_t current_vertex = 0;
    uint32_t current_index = 0;
    GLTF_Imp::NodeTraverse(model,
      [&current_node, &current_primitive, &node_map, &current_mesh,
       &current_vertex, &current_index, &index_data, &vertex_data,
       &material_cache,
       total_nodes, total_primitives, total_num_vertices, vertex_size, flags, this]
      (const tinygltf::Model &model, uint32_t n_pos, uint32_t p_pos) mutable {
        const tinygltf::Node &gltf_n = model.nodes[n_pos];
        Node &node = nodes[current_node];
        // gather node transform or compute it
        if (gltf_n.matrix.size() == 16) {
          for(size_t i = 0; i < 16; ++i) node.transform.f[i] = (float) gltf_n.matrix[i];
        } else {
          Vec3 s = {1.0f, 1.0f, 1.0f};
          Vec4 r = {0.0f, 1.0f, 0.0f, 0.0f};
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
              uint32_t min_vertex_index = (uint32_t)-1;
              uint32_t max_vertex_index = 0;
              uint32_t index_count = 0;
              GLTF_Imp::IndexTraverse(model, model.accessors[gltf_p.indices],
                [&min_vertex_index, &max_vertex_index, &current_index, &index_count, &index_data, &current_vertex]
                (const tinygltf::Model&, uint32_t index) {
                  min_vertex_index = std::min(min_vertex_index, index);
                  max_vertex_index = std::max(max_vertex_index, index);
                  index_data[current_index+index_count] = index;
                  index_count++;
              });

              Primitive &primitive = primitives[current_primitive];
              primitive.node = current_node;
              primitive.mesh = current_mesh;
              primitive.index_count = index_count;
              primitive.index_offset = current_index;
              current_index += index_count;

              if (flags & Flags::Material) {
                if (gltf_p.material >= 0) {
                  primitive.material = material_cache.index[gltf_p.material];
                }
              }

              using AttribWritter = std::function<void(float *w, uint32_t p)> ;

              AttribWritter w_position  = [](float *w, uint32_t p) {};
              AttribWritter w_normal    = [](float *w, uint32_t p) {};
              AttribWritter w_texcoord0 = [](float *w, uint32_t p) {};
              AttribWritter w_tangent   = [](float *w, uint32_t p) {};

              uint32_t vertex_stride_float = vertex_size/sizeof(float);
              for (const auto &attrib : gltf_p.attributes) {
                AttribWritter *writter = nullptr;
                unsigned int max_components = 0;
                if ((flags & Flags::Geometry_Position) && attrib.first.compare("POSITION") == 0) {
                  writter = &w_position;
                  max_components = 3;
                } else if ((flags & Flags::Geometry_Normal) && attrib.first.compare("NORMAL") == 0) {
                  writter = &w_normal;
                  max_components = 3;
                } else if ((flags & Flags::Geometry_TexCoord0) && attrib.first.compare("TEXCOORD_0") == 0) {
                  writter = &w_texcoord0;
                  max_components = 2;
                } else if ((flags & Flags::Geometry_Tangent) && attrib.first.compare("TANGENT") == 0) {
                  writter = &w_tangent;
                  max_components = 4;
                }

                if (!writter) continue;
                
                const tinygltf::Accessor &accesor = model.accessors[attrib.second];
                const tinygltf::BufferView &buffer_view = model.bufferViews[accesor.bufferView];
                const tinygltf::Buffer &buffer = model.buffers[buffer_view.buffer];
                const uint8_t* base = &buffer.data.at(buffer_view.byteOffset + accesor.byteOffset);
                int byteStride = accesor.ByteStride(buffer_view);
                const bool normalized = accesor.normalized;

                switch (accesor.type) {
                  case TINYGLTF_TYPE_SCALAR: max_components = std::min(max_components, 1u); break;
                  case TINYGLTF_TYPE_VEC2:   max_components = std::min(max_components, 2u); break;
                  case TINYGLTF_TYPE_VEC3:   max_components = std::min(max_components, 3u); break;
                  case TINYGLTF_TYPE_VEC4:   max_components = std::min(max_components, 4u); break;
                }

                switch (accesor.componentType) {
                  case TINYGLTF_COMPONENT_TYPE_FLOAT: *writter =
                    [base, byteStride, max_components](float *w, uint32_t p) {
                      const float *f = (const float *)(base + p*byteStride);
                      for (unsigned int i = 0; i < max_components; ++i) {
                        w[i] = f[i];
                      }
                    }; break;
                  case TINYGLTF_COMPONENT_TYPE_DOUBLE: *writter =
                    [base, byteStride, max_components](float *w, uint32_t p) {
                      const double*f = (const double*)(base + p*byteStride);
                      for (unsigned int i = 0; i < max_components; ++i) {
                        w[i] = (float)f[i];
                      }
                    }; break;
                  case TINYGLTF_COMPONENT_TYPE_BYTE: *writter =
                    [base, byteStride, max_components,normalized](float *w, uint32_t p) {
                      const int8_t *f = (const int8_t*)(base + p*byteStride);
                      for (unsigned int i = 0; i < max_components; ++i) {
                        w[i] = normalized?f[i]/(float)128:f[i];
                      }
                    }; break;
                  case TINYGLTF_COMPONENT_TYPE_SHORT: *writter =
                    [base, byteStride, max_components,normalized](float *w, uint32_t p) {
                      const int16_t *f = (const int16_t*)(base + p*byteStride);
                      for (unsigned int i = 0; i < max_components; ++i) {
                        w[i] = normalized?f[i]/(float)32768:f[i];
                      }
                    }; break;
                  case TINYGLTF_COMPONENT_TYPE_INT: *writter =
                    [base, byteStride, max_components,normalized](float *w, uint32_t p) {
                      const int32_t *f = (const int32_t*)(base + p*byteStride);
                      for (unsigned int i = 0; i < max_components; ++i) {
                        w[i] = normalized?f[i]/(float)2147483648:f[i];
                      }
                    }; break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: *writter =
                    [base, byteStride, max_components,normalized](float *w, uint32_t p) {
                      const uint8_t *f = (const uint8_t*)(base + p*byteStride);
                      for (unsigned int i = 0; i < max_components; ++i) {
                        w[i] = normalized?f[i]/(float)255:f[i];
                      }
                    }; break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: *writter =
                    [base, byteStride, max_components,normalized](float *w, uint32_t p) {
                      const uint16_t *f = (const uint16_t*)(base + p*byteStride);
                      for (unsigned int i = 0; i < max_components; ++i) {
                        w[i] = normalized?f[i]/(float)65535:f[i];
                      }
                    }; break;
                  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: *writter =
                    [base, byteStride, max_components,normalized](float *w, uint32_t p) {
                      const uint32_t *f = (const uint32_t*)(base + p*byteStride);
                      for (unsigned int i = 0; i < max_components; ++i) {
                        w[i] = normalized?f[i]/(float)4294967295:f[i];
                      }
                    }; break;
                  default:
                    assert(!"Not supported component type (yet)");
                }
              }

              uint32_t vertex_offset = 0;

              if (flags & Flags::Geometry_Position) {
                for (uint32_t i = 0 ; i <= max_vertex_index-min_vertex_index; ++i) {
                  w_position(&vertex_data[vertex_offset+(current_vertex+i)*vertex_stride_float], i+min_vertex_index);
                }
                if (flags & Flags::ComputeBounds) {
                  bounds_min = { FLT_MAX, FLT_MAX, FLT_MAX };
                  bounds_max = { FLT_MIN, FLT_MIN, FLT_MIN };
                  for (uint32_t i = 0; i <= max_vertex_index - min_vertex_index; ++i) {
                    Vec3 v = *(const Vec3*)&vertex_data[vertex_offset + (current_vertex + i)*vertex_stride_float];
                    Vec4 vt = Mat4::Mult(node.model, Vec4{ v.v.x, v.v.y, v.v.z, 1.0f });
                    vt.v.x /= vt.v.w;
                    vt.v.y /= vt.v.w;
                    vt.v.z /= vt.v.w;
                    primitive.bounds_min.v.x = std::min(primitive.bounds_min.v.x, vt.v.x);
                    primitive.bounds_min.v.y = std::min(primitive.bounds_min.v.y, vt.v.y);
                    primitive.bounds_min.v.z = std::min(primitive.bounds_min.v.z, vt.v.z);
                    primitive.bounds_max.v.x = std::max(primitive.bounds_max.v.x, vt.v.x);
                    primitive.bounds_max.v.y = std::max(primitive.bounds_max.v.y, vt.v.y);
                    primitive.bounds_max.v.z = std::max(primitive.bounds_max.v.z, vt.v.z);
                  }
                }
                vertex_offset += 3;
              }
              if (flags & Flags::Geometry_Normal) {
                for (uint32_t i = 0; i <= max_vertex_index-min_vertex_index; ++i) {
                  w_normal(&vertex_data[vertex_offset+(current_vertex+i)*vertex_stride_float], i+min_vertex_index);
                }
                vertex_offset += 3;
              }
              if (flags & Flags::Geometry_TexCoord0) {
                for (uint32_t i = 0; i <= max_vertex_index-min_vertex_index; ++i) {
                  w_texcoord0(&vertex_data[vertex_offset+(current_vertex+i)*vertex_stride_float], i+min_vertex_index);
                }
                vertex_offset += 2;
              }
              if (flags & Flags::Geometry_Tangent) {
                  for (uint32_t i = 0; i <= max_vertex_index - min_vertex_index; ++i) {
                      w_texcoord0(&vertex_data[vertex_offset + (current_vertex + i)*vertex_stride_float], i + min_vertex_index);
                  }
                  vertex_offset += 4;
              }

              for (uint32_t i = 0; i < primitive.index_count; ++i) {
                index_data[primitive.index_offset+i] += current_vertex;
                index_data[primitive.index_offset+i] -= min_vertex_index;
              }
              
              current_vertex += max_vertex_index - min_vertex_index+1;
              current_primitive++;
            }
          }
          current_mesh++;
        }
        current_node++;
    });

    num_textures = material_cache.textures.size();
    num_materials = material_cache.materials.size();
    textures = std::unique_ptr<Texture[]>(new Texture[num_textures]);
    materials = std::unique_ptr<Material[]>(new Material[num_materials]);
    for(uint32_t i = 0; i < num_textures; ++i) { textures[i] = material_cache.textures[i]; }
    for(uint32_t i = 0; i < num_materials; ++i) { materials[i] = material_cache.materials[i]; }

    ctx->submitDisplayList(std::move(material_cache.texture_dl));
    DisplayList dl;
    vertex_buffer = ctx->createBuffer({BufferType::Vertex, vertex_size*total_num_vertices, Usage::Static});
    index_buffer  = ctx->createBuffer({BufferType::Index, sizeof(uint32_t)*total_num_indices, Usage::Static});
    dl.fillBufferCommand()
      .set_buffer(vertex_buffer)
      .set_data(vertex_data.get())
      .set_size(vertex_size*total_num_vertices);
    dl.fillBufferCommand()
      .set_buffer(index_buffer)
      .set_data(index_data.get())
      .set_size(sizeof(uint32_t)*total_num_indices);
    ctx->submitDisplayList(std::move(dl));

    num_nodes = total_nodes;
    num_primitives = total_primitives;
    
    if (flags & Flags::ComputeBounds) {
      bounds_min = {FLT_MAX, FLT_MAX, FLT_MAX};
      bounds_max = {FLT_MIN, FLT_MIN, FLT_MIN};
      for (uint32_t i = 0; i < total_primitives; ++i) {
        Vec3 bmin = primitives[i].bounds_min;
        Vec3 bmax = primitives[i].bounds_max;
        bounds_min.v.x = std::min(bounds_min.v.x, bmin.v.x);
        bounds_min.v.y = std::min(bounds_min.v.y, bmin.v.y);
        bounds_min.v.z = std::min(bounds_min.v.z, bmin.v.z);
        bounds_max.v.x = std::max(bounds_max.v.x, bmax.v.x);
        bounds_max.v.y = std::max(bounds_max.v.y, bmax.v.y);
        bounds_max.v.z = std::max(bounds_max.v.z, bmax.v.z);
      }
    }
  }

  void GLTF::freeResources() {
    if (num_nodes) {
      num_nodes = 0;
      num_primitives = 0;
      nodes.reset();
      primitives.reset();
      DisplayList dl;
      dl.destroy(vertex_buffer);
      dl.destroy(index_buffer);
      ctx->submitDisplayList(std::move(dl));
    }
  }
}

#endif // PX_RENDER_GLTF_IMPLEMENTATION