/*
 * Copyright 2021 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "blender/blender_gpu_display.h"

#include "device/device.h"
#include "util/util_logging.h"
#include "util/util_opengl.h"

CCL_NAMESPACE_BEGIN

/* --------------------------------------------------------------------
 * BlenderDisplayShader.
 */

unique_ptr<BlenderDisplayShader> BlenderDisplayShader::create(BL::RenderEngine &b_engine,
                                                              BL::Scene &b_scene)
{
  if (b_engine.support_display_space_shader(b_scene)) {
    return make_unique<BlenderDisplaySpaceShader>(b_engine, b_scene);
  }

  return make_unique<BlenderFallbackDisplayShader>();
}

int BlenderDisplayShader::get_position_attrib_location()
{
  if (position_attribute_location_ == -1) {
    const uint shader_program = get_shader_program();
    position_attribute_location_ = glGetAttribLocation(shader_program, position_attribute_name);
  }
  return position_attribute_location_;
}

int BlenderDisplayShader::get_tex_coord_attrib_location()
{
  if (tex_coord_attribute_location_ == -1) {
    const uint shader_program = get_shader_program();
    tex_coord_attribute_location_ = glGetAttribLocation(shader_program, tex_coord_attribute_name);
  }
  return tex_coord_attribute_location_;
}

/* --------------------------------------------------------------------
 * BlenderFallbackDisplayShader.
 */

/* TODO move shaders to standalone .glsl file. */
static const char *FALLBACK_VERTEX_SHADER =
    "#version 330\n"
    "uniform vec2 fullscreen;\n"
    "in vec2 texCoord;\n"
    "in vec2 pos;\n"
    "out vec2 texCoord_interp;\n"
    "\n"
    "vec2 normalize_coordinates()\n"
    "{\n"
    "   return (vec2(2.0) * (pos / fullscreen)) - vec2(1.0);\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(normalize_coordinates(), 0.0, 1.0);\n"
    "   texCoord_interp = texCoord;\n"
    "}\n\0";

static const char *FALLBACK_FRAGMENT_SHADER =
    "#version 330\n"
    "uniform sampler2D image_texture;\n"
    "in vec2 texCoord_interp;\n"
    "out vec4 fragColor;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   fragColor = texture(image_texture, texCoord_interp);\n"
    "}\n\0";

static void shader_print_errors(const char *task, const char *log, const char *code)
{
  LOG(ERROR) << "Shader: " << task << " error:";
  LOG(ERROR) << "===== shader string ====";

  stringstream stream(code);
  string partial;

  int line = 1;
  while (getline(stream, partial, '\n')) {
    if (line < 10) {
      LOG(ERROR) << " " << line << " " << partial;
    }
    else {
      LOG(ERROR) << line << " " << partial;
    }
    line++;
  }
  LOG(ERROR) << log;
}

static int compile_fallback_shader(void)
{
  const struct Shader {
    const char *source;
    const GLenum type;
  } shaders[2] = {{FALLBACK_VERTEX_SHADER, GL_VERTEX_SHADER},
                  {FALLBACK_FRAGMENT_SHADER, GL_FRAGMENT_SHADER}};

  const GLuint program = glCreateProgram();

  for (int i = 0; i < 2; i++) {
    const GLuint shader = glCreateShader(shaders[i].type);

    string source_str = shaders[i].source;
    const char *c_str = source_str.c_str();

    glShaderSource(shader, 1, &c_str, NULL);
    glCompileShader(shader);

    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

    if (!compile_status) {
      GLchar log[5000];
      GLsizei length = 0;
      glGetShaderInfoLog(shader, sizeof(log), &length, log);
      shader_print_errors("compile", log, c_str);
      return 0;
    }

    glAttachShader(program, shader);
  }

  /* Link output. */
  glBindFragDataLocation(program, 0, "fragColor");

  /* Link and error check. */
  glLinkProgram(program);

  /* TODO(sergey): Find a way to nicely de-duplicate the error checking. */
  GLint link_status;
  glGetProgramiv(program, GL_LINK_STATUS, &link_status);
  if (!link_status) {
    GLchar log[5000];
    GLsizei length = 0;
    /* TODO(sergey): Is it really program passed to glGetShaderInfoLog? */
    glGetShaderInfoLog(program, sizeof(log), &length, log);
    shader_print_errors("linking", log, FALLBACK_VERTEX_SHADER);
    shader_print_errors("linking", log, FALLBACK_FRAGMENT_SHADER);
    return 0;
  }

  return program;
}

void BlenderFallbackDisplayShader::bind(int width, int height)
{
  create_shader_if_needed();

  if (!shader_program_) {
    return;
  }

  glUseProgram(shader_program_);
  glUniform1i(image_texture_location_, 0);
  glUniform2f(fullscreen_location_, width, height);
}

void BlenderFallbackDisplayShader::unbind()
{
}

uint BlenderFallbackDisplayShader::get_shader_program()
{
  return shader_program_;
}

void BlenderFallbackDisplayShader::create_shader_if_needed()
{
  if (shader_program_ || shader_compile_attempted_) {
    return;
  }

  shader_compile_attempted_ = true;

  shader_program_ = compile_fallback_shader();
  if (!shader_program_) {
    return;
  }

  glUseProgram(shader_program_);

  image_texture_location_ = glGetUniformLocation(shader_program_, "image_texture");
  if (image_texture_location_ < 0) {
    LOG(ERROR) << "Shader doesn't contain the 'image_texture' uniform.";
    destroy_shader();
    return;
  }

  fullscreen_location_ = glGetUniformLocation(shader_program_, "fullscreen");
  if (fullscreen_location_ < 0) {
    LOG(ERROR) << "Shader doesn't contain the 'fullscreen' uniform.";
    destroy_shader();
    return;
  }
}

void BlenderFallbackDisplayShader::destroy_shader()
{
  glDeleteProgram(shader_program_);
  shader_program_ = 0;
}

/* --------------------------------------------------------------------
 * BlenderDisplaySpaceShader.
 */

BlenderDisplaySpaceShader::BlenderDisplaySpaceShader(BL::RenderEngine &b_engine,
                                                     BL::Scene &b_scene)
    : b_engine_(b_engine), b_scene_(b_scene)
{
  DCHECK(b_engine_.support_display_space_shader(b_scene_));
}

void BlenderDisplaySpaceShader::bind(int /*width*/, int /*height*/)
{
  b_engine_.bind_display_space_shader(b_scene_);
}

void BlenderDisplaySpaceShader::unbind()
{
  b_engine_.unbind_display_space_shader();
}

uint BlenderDisplaySpaceShader::get_shader_program()
{
  if (!shader_program_) {
    glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<int *>(&shader_program_));
  }

  if (!shader_program_) {
    LOG(ERROR) << "Error retrieving shader program for display space shader.";
  }

  return shader_program_;
}

/* --------------------------------------------------------------------
 * BlenderGPUDisplay.
 */

BlenderGPUDisplay::BlenderGPUDisplay(BL::RenderEngine &b_engine, BL::Scene &b_scene)
    : display_shader_(BlenderDisplayShader::create(b_engine, b_scene))
{
  /* TODO(sergey): Think of whether it makes more sense to do it on-demand. */
  gpu_context_create();
}

BlenderGPUDisplay::~BlenderGPUDisplay()
{
  gpu_resources_destroy();
}

void BlenderGPUDisplay::reset(BufferParams &buffer_params)
{
  /* TODO(sergey): Ideally, all the reset logic will happen in the base class. See the TODO note
   * around definition of `texture_outdated_`. */

  thread_scoped_lock lock(mutex);

  const GPUDisplayParams old_params = params_;

  GPUDisplay::reset(buffer_params);

  /* If the parameters did change tag texture as unusable. This avoids drawing old texture content
   * in an updated configuration of the viewport. For example, avoids drawing old frame when render
   * border did change.
   * If the parameters did not change, allow drawing the current state of the texture, which will
   * not count as an up-to-date redraw. This will avoid flickering when doping camera navigation by
   * showing a previously rendered frame for until the new one is ready. */
  if (old_params.modified(params_)) {
    texture_size_ = make_int2(0, 0);
  }

  texture_outdated_ = true;
}

void BlenderGPUDisplay::copy_pixels_to_texture(const half4 *rgba_pixels, int width, int height)
{
  rgba_pixels_.resize(width * height);
  memcpy(rgba_pixels_.data(), rgba_pixels, sizeof(half4) * width * height);

  texture_size_ = make_int2(width, height);

  need_update_texture_ = true;
}

void BlenderGPUDisplay::get_cuda_buffer()
{
  /* TODO(sergey): Needs implementation. */
}

bool BlenderGPUDisplay::draw()
{
  const bool transparent = true;  // TODO(sergey): Derive this from Film.

  thread_scoped_lock lock(mutex);

  if (texture_size_ == make_int2(0, 0)) {
    /* Empty texture, nothing to draw. */
    return false;
  }

  if (!gpu_resources_ensure()) {
    return false;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id_);

  {
    /* TODO(sergey): Once the GPU display have own OpenGL context this should happen in
     * copy_pixels_to_texture(). */
    if (need_update_texture_) {
      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   GL_RGBA16F,
                   texture_size_.x,
                   texture_size_.y,
                   0,
                   GL_RGBA,
                   GL_HALF_FLOAT,
                   rgba_pixels_.data());
      need_update_texture_ = false;
      texture_outdated_ = false;
    }
  }

  if (transparent) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }

  display_shader_->bind(params_.full_size.x, params_.full_size.y);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  update_vertex_buffer();

  /* TODO(sergey): Does it make sense/possible to cache/reuse the VAO? */
  GLuint vertex_array_object;
  glGenVertexArrays(1, &vertex_array_object);
  glBindVertexArray(vertex_array_object);

  const int texcoord_attribute = display_shader_->get_tex_coord_attrib_location();
  const int position_attribute = display_shader_->get_position_attrib_location();

  glEnableVertexAttribArray(texcoord_attribute);
  glEnableVertexAttribArray(position_attribute);

  glVertexAttribPointer(
      texcoord_attribute, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const GLvoid *)0);
  glVertexAttribPointer(position_attribute,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        4 * sizeof(float),
                        (const GLvoid *)(sizeof(float) * 2));

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  display_shader_->unbind();

  glDeleteVertexArrays(1, &vertex_array_object);
  glBindTexture(GL_TEXTURE_2D, 0);

  if (transparent) {
    glDisable(GL_BLEND);
  }

  return !texture_outdated_;
}

void BlenderGPUDisplay::gpu_context_create()
{
}

bool BlenderGPUDisplay::gpu_resources_ensure()
{
  if (gpu_resource_creation_attempted_) {
    return gpu_resources_created_;
  }
  gpu_resource_creation_attempted_ = true;

  /* TODO(sergey): Once this GPU display keeps track of its texture and OpenGL context the
   * texture creation might need to be moved somewhere else. */
  if (!texture_id_) {
    if (!create_texture()) {
      return false;
    }
  }

  if (!vertex_buffer_) {
    glGenBuffers(1, &vertex_buffer_);
    if (!vertex_buffer_) {
      LOG(ERROR) << "Error creating vertex buffer.";
      return false;
    }
  }

  gpu_resources_created_ = true;

  return true;
}

void BlenderGPUDisplay::gpu_resources_destroy()
{
  if (vertex_buffer_ != 0) {
    glDeleteBuffers(1, &vertex_buffer_);
  }

  if (texture_id_) {
    glDeleteTextures(1, &texture_id_);
    texture_id_ = 0;
  }
}

bool BlenderGPUDisplay::create_texture()
{
  DCHECK(!texture_id_);

  glGenTextures(1, &texture_id_);

  if (!texture_id_) {
    LOG(ERROR) << "Error creating texture.";
    return false;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id_);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

void BlenderGPUDisplay::update_vertex_buffer()
{
  /* invalidate old contents - avoids stalling if the buffer is still waiting in queue to be
   * rendered. */
  glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(float), NULL, GL_STREAM_DRAW);

  float *vpointer = reinterpret_cast<float *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
  if (!vpointer) {
    return;
  }

  vpointer[0] = 0.0f;
  vpointer[1] = 0.0f;
  vpointer[2] = params_.offset.x;
  vpointer[3] = params_.offset.y;

  vpointer[4] = 1.0f;
  vpointer[5] = 0.0f;
  vpointer[6] = (float)params_.size.x + params_.offset.x;
  vpointer[7] = params_.offset.y;

  vpointer[8] = 1.0f;
  vpointer[9] = 1.0f;
  vpointer[10] = (float)params_.size.x + params_.offset.x;
  vpointer[11] = (float)params_.size.y + params_.offset.y;

  vpointer[12] = 0.0f;
  vpointer[13] = 1.0f;
  vpointer[14] = params_.offset.x;
  vpointer[15] = (float)params_.size.y + params_.offset.y;

  glUnmapBuffer(GL_ARRAY_BUFFER);
}

CCL_NAMESPACE_END
