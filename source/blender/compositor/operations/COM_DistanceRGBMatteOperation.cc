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

#include "COM_DistanceRGBMatteOperation.h"
#include "BLI_math.h"

namespace blender::compositor {

DistanceRGBMatteOperation::DistanceRGBMatteOperation()
{
  this->addInputSocket(DataType::Color);
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Value);

  this->m_inputImageProgram = nullptr;
  this->m_inputKeyProgram = nullptr;
  flags.can_be_constant = true;
}

void DistanceRGBMatteOperation::initExecution()
{
  this->m_inputImageProgram = this->getInputSocketReader(0);
  this->m_inputKeyProgram = this->getInputSocketReader(1);
}

void DistanceRGBMatteOperation::deinitExecution()
{
  this->m_inputImageProgram = nullptr;
  this->m_inputKeyProgram = nullptr;
}

float DistanceRGBMatteOperation::calculateDistance(const float key[4], const float image[4])
{
  return len_v3v3(key, image);
}

void DistanceRGBMatteOperation::executePixelSampled(float output[4],
                                                    float x,
                                                    float y,
                                                    PixelSampler sampler)
{
  float inKey[4];
  float inImage[4];

  const float tolerance = this->m_settings->t1;
  const float falloff = this->m_settings->t2;

  float distance;
  float alpha;

  this->m_inputKeyProgram->readSampled(inKey, x, y, sampler);
  this->m_inputImageProgram->readSampled(inImage, x, y, sampler);

  distance = this->calculateDistance(inKey, inImage);

  /* Store matte(alpha) value in [0] to go with
   * COM_SetAlphaMultiplyOperation and the Value output.
   */

  /* Make 100% transparent. */
  if (distance < tolerance) {
    output[0] = 0.0f;
  }
  /* In the falloff region, make partially transparent. */
  else if (distance < falloff + tolerance) {
    distance = distance - tolerance;
    alpha = distance / falloff;
    /* Only change if more transparent than before. */
    if (alpha < inImage[3]) {
      output[0] = alpha;
    }
    else { /* leave as before */
      output[0] = inImage[3];
    }
  }
  else {
    /* leave as before */
    output[0] = inImage[3];
  }
}

void DistanceRGBMatteOperation::update_memory_buffer_partial(MemoryBuffer *output,
                                                             const rcti &area,
                                                             Span<MemoryBuffer *> inputs)
{
  for (BuffersIterator<float> it = output->iterate_with(inputs, area); !it.is_end(); ++it) {
    const float *in_image = it.in(0);
    const float *in_key = it.in(1);

    float distance = this->calculateDistance(in_key, in_image);
    const float tolerance = this->m_settings->t1;
    const float falloff = this->m_settings->t2;

    /* Store matte(alpha) value in [0] to go with
     * COM_SetAlphaMultiplyOperation and the Value output.
     */

    /* Make 100% transparent. */
    if (distance < tolerance) {
      it.out[0] = 0.0f;
    }
    /* In the falloff region, make partially transparent. */
    else if (distance < falloff + tolerance) {
      distance = distance - tolerance;
      const float alpha = distance / falloff;
      /* Only change if more transparent than before. */
      if (alpha < in_image[3]) {
        it.out[0] = alpha;
      }
      else { /* Leave as before. */
        it.out[0] = in_image[3];
      }
    }
    else {
      /* Leave as before. */
      it.out[0] = in_image[3];
    }
  }
}

}  // namespace blender::compositor
