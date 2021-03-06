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

#include "COM_HueSaturationValueCorrectOperation.h"

#include "BLI_math.h"

#include "BKE_colortools.h"

namespace blender::compositor {

HueSaturationValueCorrectOperation::HueSaturationValueCorrectOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Color);

  this->m_inputProgram = nullptr;
}
void HueSaturationValueCorrectOperation::initExecution()
{
  CurveBaseOperation::initExecution();
  this->m_inputProgram = this->getInputSocketReader(0);
}

void HueSaturationValueCorrectOperation::executePixelSampled(float output[4],
                                                             float x,
                                                             float y,
                                                             PixelSampler sampler)
{
  float hsv[4], f;

  this->m_inputProgram->readSampled(hsv, x, y, sampler);

  /* adjust hue, scaling returned default 0.5 up to 1 */
  f = BKE_curvemapping_evaluateF(this->m_curveMapping, 0, hsv[0]);
  hsv[0] += f - 0.5f;

  /* adjust saturation, scaling returned default 0.5 up to 1 */
  f = BKE_curvemapping_evaluateF(this->m_curveMapping, 1, hsv[0]);
  hsv[1] *= (f * 2.0f);

  /* adjust value, scaling returned default 0.5 up to 1 */
  f = BKE_curvemapping_evaluateF(this->m_curveMapping, 2, hsv[0]);
  hsv[2] *= (f * 2.0f);

  hsv[0] = hsv[0] - floorf(hsv[0]); /* mod 1.0 */
  CLAMP(hsv[1], 0.0f, 1.0f);

  output[0] = hsv[0];
  output[1] = hsv[1];
  output[2] = hsv[2];
  output[3] = hsv[3];
}

void HueSaturationValueCorrectOperation::deinitExecution()
{
  CurveBaseOperation::deinitExecution();
  this->m_inputProgram = nullptr;
}

void HueSaturationValueCorrectOperation::update_memory_buffer_partial(MemoryBuffer *output,
                                                                      const rcti &area,
                                                                      Span<MemoryBuffer *> inputs)
{
  float hsv[4];
  for (BuffersIterator<float> it = output->iterate_with(inputs, area); !it.is_end(); ++it) {
    copy_v4_v4(hsv, it.in(0));

    /* Adjust hue, scaling returned default 0.5 up to 1. */
    float f = BKE_curvemapping_evaluateF(this->m_curveMapping, 0, hsv[0]);
    hsv[0] += f - 0.5f;

    /* Adjust saturation, scaling returned default 0.5 up to 1. */
    f = BKE_curvemapping_evaluateF(this->m_curveMapping, 1, hsv[0]);
    hsv[1] *= (f * 2.0f);

    /* Adjust value, scaling returned default 0.5 up to 1. */
    f = BKE_curvemapping_evaluateF(this->m_curveMapping, 2, hsv[0]);
    hsv[2] *= (f * 2.0f);

    hsv[0] = hsv[0] - floorf(hsv[0]); /* Mod 1.0. */
    CLAMP(hsv[1], 0.0f, 1.0f);

    copy_v4_v4(it.out, hsv);
  }
}

}  // namespace blender::compositor
