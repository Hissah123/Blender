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
#include "BLI_utildefines.h"
#include "COM_MultiThreadedOperation.h"
#include "DNA_scene_types.h"
#include "MEM_guardedalloc.h"

#include "RE_pipeline.h"

namespace blender::compositor {

/**
 * Base class for all renderlayeroperations
 *
 * \todo Rename to operation.
 */
class RenderLayersProg : public MultiThreadedOperation {
 protected:
  /**
   * Reference to the scene object.
   */
  Scene *m_scene;

  /**
   * layerId of the layer where this operation needs to get its data from
   */
  short m_layerId;

  /**
   * viewName of the view to use (unless another view is specified by the node
   */
  const char *m_viewName;

  const MemoryBuffer *layer_buffer_;

  /**
   * Cached instance to the float buffer inside the layer.
   * TODO: To be removed with tiled implementation.
   */
  float *m_inputBuffer;

  /**
   * Render-pass where this operation needs to get its data from.
   */
  std::string m_passName;

  int m_elementsize;

  /**
   * \brief render data used for active rendering
   */
  const RenderData *m_rd;

  /**
   * Determine the output resolution. The resolution is retrieved from the Renderer
   */
  void determine_canvas(const rcti &preferred_area, rcti &r_area) override;

  /**
   * retrieve the reference to the float buffer of the renderer.
   */
  inline float *getInputBuffer()
  {
    return this->m_inputBuffer;
  }

  void doInterpolation(float output[4], float x, float y, PixelSampler sampler);

 public:
  /**
   * Constructor
   */
  RenderLayersProg(const char *passName, DataType type, int elementsize);
  /**
   * setter for the scene field. Will be called from
   * \see RenderLayerNode to set the actual scene where
   * the data will be retrieved from.
   */
  void setScene(Scene *scene)
  {
    this->m_scene = scene;
  }
  Scene *getScene() const
  {
    return this->m_scene;
  }
  void setRenderData(const RenderData *rd)
  {
    this->m_rd = rd;
  }
  void setLayerId(short layerId)
  {
    this->m_layerId = layerId;
  }
  short getLayerId() const
  {
    return this->m_layerId;
  }
  void setViewName(const char *viewName)
  {
    this->m_viewName = viewName;
  }
  const char *getViewName()
  {
    return this->m_viewName;
  }
  void initExecution() override;
  void deinitExecution() override;
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  std::unique_ptr<MetaData> getMetaData() override;

  virtual void update_memory_buffer_partial(MemoryBuffer *output,
                                            const rcti &area,
                                            Span<MemoryBuffer *> inputs) override;
};

class RenderLayersAOOperation : public RenderLayersProg {
 public:
  RenderLayersAOOperation(const char *passName, DataType type, int elementsize)
      : RenderLayersProg(passName, type, elementsize)
  {
  }
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};

class RenderLayersAlphaProg : public RenderLayersProg {
 public:
  RenderLayersAlphaProg(const char *passName, DataType type, int elementsize)
      : RenderLayersProg(passName, type, elementsize)
  {
  }
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};

class RenderLayersDepthProg : public RenderLayersProg {
 public:
  RenderLayersDepthProg(const char *passName, DataType type, int elementsize)
      : RenderLayersProg(passName, type, elementsize)
  {
  }
  void executePixelSampled(float output[4], float x, float y, PixelSampler sampler) override;

  void update_memory_buffer_partial(MemoryBuffer *output,
                                    const rcti &area,
                                    Span<MemoryBuffer *> inputs) override;
};

}  // namespace blender::compositor
