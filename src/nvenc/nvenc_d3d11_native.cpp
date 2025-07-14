/**
 * @file src/nvenc/nvenc_d3d11_native.cpp
 * @brief Definitions for native Direct3D11 NVENC encoder.
 */
#ifdef _WIN32
  // this include
  #include "nvenc_d3d11_native.h"

  // local includes
  #include "nvenc_utils.h"

namespace nvenc {

  nvenc_d3d11_native::nvenc_d3d11_native(ID3D11Device *d3d_device):
      nvenc_d3d11(NV_ENC_DEVICE_TYPE_DIRECTX),
      d3d_device(d3d_device) {
    device = d3d_device;
  }

  nvenc_d3d11_native::~nvenc_d3d11_native() {
    if (encoder) {
      destroy_encoder();
    }
  }

  ID3D11Texture2D *
    nvenc_d3d11_native::get_input_texture() {
    return d3d_input_texture.GetInterfacePtr();
  }

  bool nvenc_d3d11_native::create_and_register_input_buffer() {
    if (encoder_params.buffer_format == NV_ENC_BUFFER_FORMAT_YUV444_10BIT) {
      BOOST_LOG(error) << "NvEnc: 10-bit 4:4:4 encoding is incompatible with D3D11 surface formats, use CUDA interop";
      return false;
    }

    if (!d3d_input_texture) {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = encoder_params.width;
      desc.Height = encoder_params.height;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = dxgi_format_from_nvenc_format(encoder_params.buffer_format);
      desc.SampleDesc.Count = 1;
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_RENDER_TARGET;
      if (d3d_device->CreateTexture2D(&desc, nullptr, &d3d_input_texture) != S_OK) {
        BOOST_LOG(error) << "NvEnc: couldn't create input texture";
        return false;
      }
    }

    if (!registered_input_buffer) {
      NV_ENC_REGISTER_RESOURCE register_resource = {min_struct_version(NV_ENC_REGISTER_RESOURCE_VER, 3, 4)};
      register_resource.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
      register_resource.width = encoder_params.width;
      register_resource.height = encoder_params.height;
      register_resource.resourceToRegister = d3d_input_texture.GetInterfacePtr();
      register_resource.bufferFormat = encoder_params.buffer_format;
      register_resource.bufferUsage = NV_ENC_INPUT_IMAGE;

      if (nvenc_failed(nvenc->nvEncRegisterResource(encoder, &register_resource))) {
        BOOST_LOG(error) << "NvEnc: NvEncRegisterResource() failed: " << last_nvenc_error_string;
        return false;
      }

      registered_input_buffer = register_resource.registeredResource;
    }

    return true;
  }

  bool nvenc_d3d11_native::dump_frame_to_cpu(std::vector<uint8_t>& out_rgba, int x1, int y1, int x2, int y2) {
    if (!d3d_input_texture) return false;
    ID3D11Device* device = d3d_device;
    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);
    if (!context) return false;

    D3D11_TEXTURE2D_DESC desc;
    d3d_input_texture->GetDesc(&desc);
    D3D11_TEXTURE2D_DESC cpu_desc = desc;
    cpu_desc.Usage = D3D11_USAGE_STAGING;
    cpu_desc.BindFlags = 0;
    cpu_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    cpu_desc.MiscFlags = 0;

    ID3D11Texture2D* cpu_tex = nullptr;
    if (device->CreateTexture2D(&cpu_desc, nullptr, &cpu_tex) != S_OK) {
        context->Release();
        return false;
    }
    context->CopyResource(cpu_tex, d3d_input_texture);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (context->Map(cpu_tex, 0, D3D11_MAP_READ, 0, &mapped) != S_OK) {
        cpu_tex->Release();
        context->Release();
        return false;
    }
    int roi_w = x2 - x1;
    int roi_h = y2 - y1;
    DXGI_FORMAT format = desc.Format;
    /*BOOST_LOG(info) << "dump_frame_to_cpu: x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2
                << " roi_w=" << roi_w << " roi_h=" << roi_h << " RowPitch=" << mapped.RowPitch
                << " format=" << format;*/
    if (format == 103) { // DXGI_FORMAT_NV12
        out_rgba.resize(roi_w * roi_h); // 只拷贝Y分量
        for (int y = 0; y < roi_h; ++y) {
            const uint8_t* src = (const uint8_t*)mapped.pData + (y1 + y) * mapped.RowPitch + x1;
            memcpy(&out_rgba[y * roi_w], src, roi_w);
        }
    } else {
        out_rgba.resize(roi_w * roi_h * 4);
        for (int y = 0; y < roi_h; ++y) {
            const uint8_t* src = (const uint8_t*)mapped.pData + (y1 + y) * mapped.RowPitch + x1 * 4;
            memcpy(&out_rgba[y * roi_w * 4], src, roi_w * 4);
        }
    }
    context->Unmap(cpu_tex, 0);
    cpu_tex->Release();
    context->Release();
    return true;
  }

}  // namespace nvenc
#endif
