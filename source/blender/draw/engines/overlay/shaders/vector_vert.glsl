
uniform vec3 objPosition;
uniform vec3 vector;
uniform float scale;
uniform float min_clamp;
uniform vec3 colour;

flat out vec4 finalColor;

vec3 verts[5] = vec3[5](vec3(0.0, 0.0, 0.0),   // origin of arrow
                        vec3(0.0, 0.0, 0.0),   // neck
                        vec3(0.0, 0.0, 0.0),   // point on the right
                        vec3(0.0, 0.0, 0.0),   // topmost point
                        vec3(0.0, 0.0, 0.0));  // point on the left

const int indices[6] = int[6](0, 3, 3, 2, 3, 4);

void main()
{

  GPU_INTEL_VERTEX_SHADER_WORKAROUND

  vec3 pos = vec3(0.0, 0.0, 0.0);
  vec3 v = normalize(vector);
  vec4 dir = ViewMatrixInverse * vec4(0.0, 0.0, -1.0, 0.0);  // viewing direction
  vec3 dir1 = normalize(cross(dir.xyz, v));  // perpendicular to view dir. and length of arrow
  if (gl_VertexID % 6 != 0) {

    verts[3] = v * length(vector) * scale + v * min_clamp;            // topmost point
    verts[1] = v * length(vector) * scale + v * min_clamp - v * 0.2;  // neck
    verts[2] = verts[1] + dir1 * 0.0866;                              // right side point of head
    verts[4] = verts[1] - dir1 * 0.0866;                              // left side point of head
  }
  pos += verts[indices[gl_VertexID % 6]];
  pos += objPosition;
  finalColor = vec4(colour, 1.0);
  vec3 world_pos = point_object_to_world(pos);
  gl_Position = point_world_to_ndc(world_pos);
}