#pragma once

#include "taichi/rhi/device.h"

#include "glad/gl.h"
#include "GLFW/glfw3.h"

namespace taichi::lang {
namespace opengl {

class GLDevice;

void check_opengl_error(const std::string &msg = "OpenGL");

class GLResourceSet : public ShaderResourceSet {
 public:
  GLResourceSet() = default;
  explicit GLResourceSet(const GLResourceSet &other) = default;

  ~GLResourceSet() override;

  GLResourceSet &rw_buffer(uint32_t binding, DevicePtr ptr, size_t size) final;
  GLResourceSet &rw_buffer(uint32_t binding, DeviceAllocation alloc) final;

  GLResourceSet &buffer(uint32_t binding, DevicePtr ptr, size_t size) final;
  GLResourceSet &buffer(uint32_t binding, DeviceAllocation alloc) final;

  GLResourceSet &image(uint32_t binding,
                       DeviceAllocation alloc,
                       ImageSamplerConfig sampler_config) final;
  GLResourceSet &rw_image(uint32_t binding,
                          DeviceAllocation alloc,
                          int lod) final;

  struct BufferBinding {
    GLuint buffer;
    size_t offset;
    size_t size;
  };

  const std::unordered_map<uint32_t, BufferBinding> &ssbo_binding_map() {
    return ssbo_binding_map_;
  }

  const std::unordered_map<uint32_t, BufferBinding> &ubo_binding_map() {
    return ubo_binding_map_;
  }

  const std::unordered_map<uint32_t, GLuint> &texture_binding_map() {
    return texture_binding_map_;
  }

 private:
  std::unordered_map<uint32_t, BufferBinding> ssbo_binding_map_;
  std::unordered_map<uint32_t, BufferBinding> ubo_binding_map_;
  std::unordered_map<uint32_t, GLuint> texture_binding_map_;
};

class GLPipeline : public Pipeline {
 public:
  GLPipeline(const PipelineSourceDesc &desc, const std::string &name);
  ~GLPipeline() override;

  GLuint get_program() {
    return program_id_;
  }

 private:
  GLuint program_id_;
};

class GLCommandList : public CommandList {
 public:
  explicit GLCommandList(GLDevice *device) : device_(device) {
  }
  ~GLCommandList() override;

  void bind_pipeline(Pipeline *p) override;
  RhiResult bind_shader_resources(ShaderResourceSet *res,
                                  int set_index = 0) final;
  RhiResult bind_raster_resources(RasterResources *res) final;
  void buffer_barrier(DevicePtr ptr, size_t size) override;
  void buffer_barrier(DeviceAllocation alloc) override;
  void memory_barrier() override;
  void buffer_copy(DevicePtr dst, DevicePtr src, size_t size) override;
  void buffer_fill(DevicePtr ptr, size_t size, uint32_t data) override;
  void dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1) override;

  // These are not implemented in compute only device
  void begin_renderpass(int x0,
                        int y0,
                        int x1,
                        int y1,
                        uint32_t num_color_attachments,
                        DeviceAllocation *color_attachments,
                        bool *color_clear,
                        std::vector<float> *clear_colors,
                        DeviceAllocation *depth_attachment,
                        bool depth_clear) override;
  void end_renderpass() override;
  void draw(uint32_t num_verticies, uint32_t start_vertex = 0) override;
  void set_line_width(float width) override;
  void draw_indexed(uint32_t num_indicies,
                    uint32_t start_vertex = 0,
                    uint32_t start_index = 0) override;
  void image_transition(DeviceAllocation img,
                        ImageLayout old_layout,
                        ImageLayout new_layout) override;
  void buffer_to_image(DeviceAllocation dst_img,
                       DevicePtr src_buf,
                       ImageLayout img_layout,
                       const BufferImageCopyParams &params) override;
  void image_to_buffer(DevicePtr dst_buf,
                       DeviceAllocation src_img,
                       ImageLayout img_layout,
                       const BufferImageCopyParams &params) override;

  // GL only stuff
  void run_commands();

 private:
  struct Cmd {
    virtual void execute() {
    }
    virtual ~Cmd() {
    }
  };

  struct CmdBindPipeline : public Cmd {
    GLuint program{0};
    void execute() override;
  };

  struct CmdBindBufferToIndex : public Cmd {
    GLuint buffer{0};
    GLuint index{0};
    GLuint offset{0};
    GLuint size{0};
    GLenum target{GL_SHADER_STORAGE_BUFFER};
    void execute() override;
  };

  struct CmdBindTextureToIndex : public Cmd {
    GLuint texture{0};
    GLuint index{0};
    GLenum target{GL_TEXTURE_2D};
    void execute() override;
  };

  struct CmdBufferBarrier : public Cmd {
    void execute() override;
  };

  struct CmdBufferCopy : public Cmd {
    GLuint src{0}, dst{0};
    size_t src_offset{0}, dst_offset{0};
    size_t size{0};
    void execute() override;
  };

  struct CmdBufferFill : public Cmd {
    GLuint buffer{0};
    size_t offset{0};
    size_t size{0};
    uint32_t data{0};
    void execute() override;
  };

  struct CmdDispatch : public Cmd {
    uint32_t x{0}, y{0}, z{0};
    void execute() override;
  };

  struct CmdImageTransition : public Cmd {
    void execute() override;
  };

  struct CmdBufferToImage : public Cmd {
    BufferImageCopyParams params;
    GLuint image{0};
    GLuint buffer{0};
    size_t offset{0};
    GLDevice *device{nullptr};
    void execute() override;
  };

  struct CmdImageToBuffer : public Cmd {
    BufferImageCopyParams params;
    GLuint image{0};
    GLuint buffer{0};
    size_t offset{0};
    GLDevice *device{nullptr};
    void execute() override;
  };

  std::vector<std::unique_ptr<Cmd>> recorded_commands_;
  GLDevice *device_{nullptr};
};

class GLStream : public Stream {
 public:
  explicit GLStream(GLDevice *device) : device_(device) {
  }
  ~GLStream() override;

  std::unique_ptr<CommandList> new_command_list() override;
  StreamSemaphore submit(
      CommandList *cmdlist,
      const std::vector<StreamSemaphore> &wait_semaphores = {}) override;
  StreamSemaphore submit_synced(
      CommandList *cmdlist,
      const std::vector<StreamSemaphore> &wait_semaphores = {}) override;

  void command_sync() override;

 private:
  GLDevice *device_{nullptr};
};

class GLDevice : public GraphicsDevice {
 public:
  GLDevice();
  ~GLDevice() override;

  Arch arch() const override {
    return Arch::opengl;
  }

  DeviceAllocation allocate_memory(const AllocParams &params) override;
  void dealloc_memory(DeviceAllocation handle) override;

  GLint get_devalloc_size(DeviceAllocation handle);

  std::unique_ptr<Pipeline> create_pipeline(
      const PipelineSourceDesc &src,
      std::string name = "Pipeline") override;

  ShaderResourceSet *create_resource_set() final {
    return new GLResourceSet;
  }

  RasterResources *create_raster_resources() final {
    TI_NOT_IMPLEMENTED;
  }

  // Mapping can fail and will return nullptr
  RhiResult map_range(DevicePtr ptr, uint64_t size, void **mapped_ptr) final;
  RhiResult map(DeviceAllocation alloc, void **mapped_ptr) final;

  void unmap(DevicePtr ptr) final;
  void unmap(DeviceAllocation alloc) final;

  // Strictly intra device copy (synced)
  void memcpy_internal(DevicePtr dst, DevicePtr src, uint64_t size) override;

  // Each thraed will acquire its own stream
  Stream *get_compute_stream() override;

  std::unique_ptr<Pipeline> create_raster_pipeline(
      const std::vector<PipelineSourceDesc> &src,
      const RasterParams &raster_params,
      const std::vector<VertexInputBinding> &vertex_inputs,
      const std::vector<VertexInputAttribute> &vertex_attrs,
      std::string name = "Pipeline") override;

  Stream *get_graphics_stream() override;

  void wait_idle() override;

  std::unique_ptr<Surface> create_surface(const SurfaceConfig &config) override;
  DeviceAllocation create_image(const ImageParams &params) override;
  void destroy_image(DeviceAllocation handle) override;

  void image_transition(DeviceAllocation img,
                        ImageLayout old_layout,
                        ImageLayout new_layout) override;
  void buffer_to_image(DeviceAllocation dst_img,
                       DevicePtr src_buf,
                       ImageLayout img_layout,
                       const BufferImageCopyParams &params) override;
  void image_to_buffer(DevicePtr dst_buf,
                       DeviceAllocation src_img,
                       ImageLayout img_layout,
                       const BufferImageCopyParams &params) override;

  GLuint get_image_gl_dims(GLuint image) const {
    return image_to_dims_.at(image);
  }

  GLuint get_image_gl_internal_format(GLuint image) const {
    return image_to_int_format_.at(image);
  }

 private:
  GLStream stream_;
  std::unordered_map<GLuint, GLbitfield> buffer_to_access_;
  std::unordered_map<GLuint, GLuint> image_to_dims_;
  std::unordered_map<GLuint, GLuint> image_to_int_format_;
};

class GLSurface : public Surface {
 public:
  ~GLSurface() override;

  StreamSemaphore acquire_next_image() override;
  DeviceAllocation get_target_image() override;
  void present_image(
      const std::vector<StreamSemaphore> &wait_semaphores = {}) override;
  std::pair<uint32_t, uint32_t> get_size() override;
  BufferFormat image_format() override;
  void resize(uint32_t width, uint32_t height) override;
};

}  // namespace opengl
}  // namespace taichi::lang
