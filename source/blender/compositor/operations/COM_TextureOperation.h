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

#include "BLI_listbase.h"
#include "COM_MultiThreadedOperation.h"
#include "DNA_texture_types.h"
#include "MEM_guardedalloc.h"

#include "RE_pipeline.h"
#include "RE_texture.h"

namespace blender::compositor {

/**
 * Base class for all renderlayeroperations
 *
 * \todo Rename to operation.
 */
class TextureBaseOperation : public MultiThreadedOperation {
 private:
  Tex *m_texture;
  const RenderData *m_rd;
  SocketReader *m_inputSize;
  SocketReader *m_inputOffset;
  struct ImagePool *m_pool;
  bool m_sceneColorManage;

 protected:
  /**
   * Determine the output resolution.
   */
  void determine_canvas(const rcti &preferred_area, rcti &r_area) override;

  /**
   * Constructor
   */
  TextureBaseOperation();

 public:
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  void setTexture(Tex *texture)
  {
    this->m_texture = texture;
  }
  void initExecution() override;
  void deinitExecution() override;
  void setRenderData(const RenderData *rd)
  {
    this->m_rd = rd;
  }
  void setSceneColorManage(bool sceneColorManage)
  {
    this->m_sceneColorManage = sceneColorManage;
  }

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};

class TextureOperation : public TextureBaseOperation {
 public:
  TextureOperation();
};
class TextureAlphaOperation : public TextureBaseOperation {
 public:
  TextureAlphaOperation();
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};

}  // namespace blender::compositor
