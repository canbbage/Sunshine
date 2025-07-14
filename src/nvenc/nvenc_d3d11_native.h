/**
 * @file src/nvenc/nvenc_d3d11_native.h
 * @brief Declarations for native Direct3D11 NVENC encoder.
 */
#pragma once
#ifdef _WIN32
  // standard includes
  #include <comdef.h>
  #include <d3d11.h>

  // local includes
  #include "nvenc_d3d11.h"

namespace nvenc {

  /**
   * @brief Native Direct3D11 NVENC encoder.
   */
  class nvenc_d3d11_native final: public nvenc_d3d11 {
  public:
    /**
     * @param d3d_device Direct3D11 device used for encoding.
     */
    explicit nvenc_d3d11_native(ID3D11Device *d3d_device);
    ~nvenc_d3d11_native();

    ID3D11Texture2D *get_input_texture() override;

    bool dump_frame_to_cpu(std::vector<uint8_t>& out_rgba, int x1, int y1, int x2, int y2) override;

  private:
    bool create_and_register_input_buffer() override;

    const ID3D11DevicePtr d3d_device;
    ID3D11Texture2DPtr d3d_input_texture;
  };

}  // namespace nvenc
#endif
