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

#include "COM_VectorCurveOperation.h"

#include "BKE_colortools.h"

namespace blender::compositor {

VectorCurveOperation::VectorCurveOperation()
{
  this->addInputSocket(DataType::Vector);
  this->addOutputSocket(DataType::Vector);

  this->m_inputProgram = nullptr;
}
void VectorCurveOperation::initExecution()
{
  CurveBaseOperation::initExecution();
  this->m_inputProgram = this->getInputSocketReader(0);
}

void VectorCurveOperation::executePixelSampled(float output[4],
                                               float x,
                                               float y,
                                               PixelSampler sampler)
{
  float input[4];

  this->m_inputProgram->readSampled(input, x, y, sampler);

  BKE_curvemapping_evaluate_premulRGBF(this->m_curveMapping, output, input);
}

void VectorCurveOperation::deinitExecution()
{
  CurveBaseOperation::deinitExecution();
  this->m_inputProgram = nullptr;
}

void VectorCurveOperation::update_memory_buffer_partial(MemoryBuffer *output,
                                                        const rcti &area,
                                                        Span<MemoryBuffer *> inputs)
{
  CurveMapping *curve_map = this->m_curveMapping;
  for (BuffersIterator<float> it = output->iterate_with(inputs, area); !it.is_end(); ++it) {
    BKE_curvemapping_evaluate_premulRGBF(curve_map, it.out, it.in(0));
  }
}

}  // namespace blender::compositor
