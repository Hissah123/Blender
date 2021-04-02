
/**
 * Film accumulation utils functions.
 **/

#pragma BLENDER_REQUIRE(eevee_shader_shared.hh)
#pragma BLENDER_REQUIRE(eevee_camera_lib.glsl)

vec4 film_data_encode(FilmData film, vec4 data, float weight)
{
  /* TODO(fclem) Depth should be converted to radial depth in panoramic projection. */
  if (film.data_type == FILM_DATA_COLOR) {
    data = safe_color(data);
    data.rgb = log2(1.0 + data.rgb);
    data *= weight;
  }
  return data;
}

vec4 film_data_decode(FilmData film, vec4 data, float weight)
{
  if (film.data_type == FILM_DATA_COLOR) {
    data *= safe_rcp(weight);
    data.rgb = exp2(data.rgb) - 1.0;
  }
  return data;
}

/* Returns uv's position in the previous frame. */
vec2 film_uv_history_get(CameraData camera, CameraData camera_history, vec2 uv)
{
#if 0 /* TODO reproject history */
  vec3 V = camera_world_from_uv(camera, uv);
  vec3 V_prev = transform_point(hitory_mat, V);
  vec2 uv_history = camera_uv_from_world(camera_history, V_prev);
  return uv_history;
#endif
  return uv;
}

/* -------------------------------------------------------------------- */
/** \name Filter
 * \{ */

float film_filter_weight(CameraData camera, vec2 offset)
{
#if 1 /* Faster */
  /* Gaussian fitted to Blackman-Harris. */
  float r = len_squared(offset) / sqr(camera.filter_size);
  const float sigma = 0.284;
  const float fac = -0.5 / (sigma * sigma);
  float weight = exp(fac * r);
#else
  /* Blackman-Harris filter. */
  float r = M_2PI * saturate(0.5 + length(offset) / (2.0 * camera.filter_size));
  float weight = 0.35875 - 0.48829 * cos(r) + 0.14128 * cos(2.0 * r) - 0.01168 * cos(3.0 * r);
#endif
  /* Always return a weight above 0 to avoid blind spots between samples. */
  return max(weight, 1e-6);
}

void film_process_sample(CameraData camera,
                         FilmData film,
                         mat4 input_persmat,
                         mat4 input_persinv,
                         sampler2D input_tx,
                         vec2 sample_offset,
                         inout vec4 data,
                         inout float weight)
{
  /* Project sample from destrination space to source texture. */
  vec2 sample_center = gl_FragCoord.xy;
  vec3 V_dst = camera_world_from_uv(camera, (sample_center + sample_offset) / vec2(film.extent));
  /* Pixels outside of projection range. */
  if (V_dst == vec3(0.0)) {
    return;
  }

  bool is_persp = camera.type != CAMERA_ORTHO;
  vec2 uv_src = camera_uv_from_world(input_persmat, is_persp, V_dst);
  /* Snap to sample actual location (pixel center). */
  vec2 input_size = vec2(textureSize(input_tx, 0));
  vec2 texel_center_src = floor(uv_src * input_size) + 0.5;
  uv_src = texel_center_src / input_size;
  /* Discard pixels outside of input range. */
  if (any(greaterThan(abs(uv_src - 0.5), vec2(0.5)))) {
    return;
  }

  /* Reproject sample location in destination space to have correct distance metric. */
  vec3 V_src = camera_world_from_uv(input_persinv, uv_src);
  vec2 uv_dst = camera_uv_from_world(camera, V_src);
  vec2 sample_dst = uv_dst * vec2(film.extent);

  /* Equirectangular projection might wrap and have more than one point mapping to the same
   * original coordinate. We need to get the closest pixel center.
   * NOTE: This is wrong for projection outside the main frame. */
  if (camera.type == CAMERA_PANO_EQUIRECT) {
    vec3 V_center = camera_world_from_uv(camera, sample_center / vec2(film.extent));
    sample_center = camera_uv_from_world(camera, V_center) * vec2(film.extent);
  }
  /* Compute filter weight and add to weighted sum. */
  vec2 offset = sample_dst - sample_center;
  float sample_weight = film_filter_weight(camera, offset);
  vec4 sample_data = textureLod(input_tx, uv_src, 0.0);
  data += film_data_encode(film, sample_data, sample_weight);
  weight += sample_weight;
}

/** \} */