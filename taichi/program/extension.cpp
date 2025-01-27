#include "extension.h"

#include <unordered_map>
#include <unordered_set>

namespace taichi::lang {

bool is_extension_supported(Arch arch, Extension ext) {
  static std::unordered_map<Arch, std::unordered_set<Extension>> arch2ext = {
      {Arch::x64,
       {Extension::sparse, Extension::quant, Extension::quant_basic,
        Extension::data64, Extension::adstack, Extension::assertion,
        Extension::extfunc, Extension::dynamic_index, Extension::mesh}},
      {Arch::arm64,
       {Extension::sparse, Extension::quant, Extension::quant_basic,
        Extension::data64, Extension::adstack, Extension::assertion,
        Extension::dynamic_index, Extension::mesh}},
      {Arch::cuda,
       {Extension::sparse, Extension::quant, Extension::quant_basic,
        Extension::data64, Extension::adstack, Extension::bls,
        Extension::assertion, Extension::dynamic_index, Extension::mesh}},
      // TODO: supporting quant in metal(tests randomly crashed)
      {Arch::metal,
       {Extension::adstack, Extension::assertion, Extension::dynamic_index,
        Extension::sparse}},
      {Arch::opengl, {Extension::dynamic_index, Extension::extfunc}},
      {Arch::gles, {}},
      {Arch::vulkan, {Extension::dynamic_index}},
      {Arch::dx11, {Extension::dynamic_index}},
      {Arch::cc, {Extension::data64, Extension::extfunc, Extension::adstack}},
  };
  // if (with_opengl_extension_data64())
  // arch2ext[Arch::opengl].insert(Extension::data64); // TODO: singleton
  const auto &exts = arch2ext[arch];
  return exts.find(ext) != exts.end();
}

}  // namespace taichi::lang
