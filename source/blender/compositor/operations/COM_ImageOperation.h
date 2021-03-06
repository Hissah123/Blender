/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Copyright 2011, Blender Foundation.
 */

#pragma once

#include "BKE_image.h"
#include "BLI_listbase.h"
#include "BLI_utildefines.h"
#include "COM_MultiThreadedOperation.h"
#include "MEM_guardedalloc.h"

#include "RE_pipeline.h"
#include "RE_texture.h"

namespace blender::compositor {

/**
 * \brief Base class for all image operations
 */
class BaseImageOperation : public MultiThreadedOperation {
 protected:
  ImBuf *m_buffer;
  Image *m_image;
  ImageUser *m_imageUser;
  /* TODO: Remove raw buffers when removing Tiled implementation. */
  float *m_imageFloatBuffer;
  unsigned int *m_imageByteBuffer;
  float *m_depthBuffer;

  MemoryBuffer *depth_buffer_;
  int m_imageheight;
  int m_imagewidth;
  int m_framenumber;
  int m_numberOfChannels;
  const RenderData *m_rd;
  const char *m_viewName;

  BaseImageOperation();
  /**
   * Determine the output resolution. The resolution is retrieved from the Renderer
   */
  void determine_canvas(const rcti &preferred_area, rcti &r_area) override;

  virtual ImBuf *getImBuf();

 public:
  void initExecution() override;
  void deinitExecution() override;
  void setImage(Image *image)
  {
    this->m_image = image;
  }
  void setImageUser(ImageUser *imageuser)
  {
    this->m_imageUser = imageuser;
  }
  void setRenderData(const RenderData *rd)
  {
    this->m_rd = rd;
  }
  void setViewName(const char *viewName)
  {
    this->m_viewName = viewName;
  }
  void setFramenumber(int framenumber)
  {
    this->m_framenumber = framenumber;
  }
};
class ImageOperation : public BaseImageOperation {
 public:
  /**
   * Constructor
   */
  ImageOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};
class ImageAlphaOperation : public BaseImageOperation {
 public:
  /**
   * Constructor
   */
  ImageAlphaOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};
class ImageDepthOperation : public BaseImageOperation {
 public:
  /**
   * Constructor
   */
  ImageDepthOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};

}  // namespace blender::compositor
