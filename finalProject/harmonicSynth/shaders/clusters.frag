#version 330

// Variables rom the Application:
uniform float time; // The time our application has been running.
uniform vec3 cam_pos;

// Internal Variables:
float step_size = 0.01; // The distance each ray of light travels per step.
float hitSurf = 0.01; // The distance from the ray to the object within we consider the ray to have hit.
int max_steps = 1024; // The maximum amount of steps the ray can take before it's considered to have missed all surfaces.
vec3 box_min = vec3(-1.0); // The minimum corner of the bounding box.
vec3 box_max = vec3(1.0); // The maximum corner of the bounding box.

// Variables from the Vertex Shader:
in vec3 ray_dir, ray_origin; // The direction and origin of the ray.

layout (location = 0) out vec4 frag_out0; // The output color of the fragment shader.


// Check if the ray intersects the bounding box:
vec3 rayBoxIntersect(const vec3 b_min, const vec3 b_max, const vec3 r_o, const vec3 r_d) {
  vec3 inv_dir = 1.0 / r_d; // Inverse of the ray direction.
  vec3 tbot = inv_dir * (b_min - r_o); // The distance to the minimum corner of the bounding box.
  vec3 ttop = inv_dir * (b_max - r_o); // The distance to the maximum corner of the bounding box.
  vec3 tmin = min(ttop, tbot); // The minimum distance to the bounding box.
  vec3 tmax = max(ttop, tbot); // The maximum distance to the bounding box.

  // What's going on here?
  vec2 traverse = max(tmin.xx, tmin.yz);
  float traverse_low = max(traverse.x, traverse.y);
  traverse = min(tmax.xx, tmax.yz);
  float traverse_high = min(traverse.x, traverse.y);

  // Encode the different measurements of the boudning box to the ray in a vec3.
  return vec3(float(traverse_high > max(traverse_low, 0.0)), traverse_low, traverse_high);
}

// Signed distance field formula for a sphere:
float sphereSDF(vec3 center, float radius, vec3 toPoint){
  return length(center - toPoint) - radius;
}

// Signed distance field formula for a box:
// float boxSDF(vec3 center, vec3 size, vec3 toPoint) {
//   vec3 d = abs(center - toPoint) - size;
//   return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
// }

// The SDF of our scene:
float scene(vec3 p){
  float d1 = sphereSDF(vec3(-0.5, 0, 0), 0.5, p); // Sphere one.
  float d2 = sphereSDF(vec3(0.5, 0, 0), 0.1, p); // Sphere two.
  float d3 = sphereSDF(vec3(0, 0.5, 0), 0.1, p); // Sphere three.
  float d4 = sphereSDF(vec3(0, -0.5, 0), 0.2, p); // Sphere four.
  float k = 8.0; // The smoothness coefficient of the minimum.
  float res = exp2(-k * d1) + exp2(-k * d2) + exp2(-k * d3) + exp2(-k * d4); // Calculate the smooth minimum.
  float smoothMin = -log2(res) / k; // Total distance. 
  return smoothMin;
}

// Get the normals of the objects in the scene:
vec3 getNormals(float s, vec3 r_o, vec3 r_d) {
    const float e = 0.01; // The epsilon value.
    vec3 p = r_o + s * r_d; // The position of the ray.
    // p -= noise.rgb;
    float nx = scene(vec3(p.x + e, p.y, p.z)) - scene(vec3(p.x - e, p.y, p.z)); // The x component of the normal.
    float ny = scene(vec3(p.x, p.y + e, p.z)) - scene(vec3(p.x, p.y - e, p.z)); // The y component of the normal.
    float nz = scene(vec3(p.x, p.y, p.z + e)) - scene(vec3(p.x, p.y, p.z - e)); // The z component of the normal.
    return normalize(vec3(nx, ny, nz)); // Return the normal.
}

// Lighting for the scene:
vec3 lighting(float d, vec3 r_o, vec3 r_d){
  float surface = d; // The distance to the surface.
  vec3 rayOrigin = r_o; // The origin of the ray.
  vec3 rayDir = r_d; // The direction of the ray.
  vec3 lightPos = cam_pos; // The position of the light source, which will be the same as our camera nav.
	vec3 n = getNormals(surface, rayOrigin, rayDir); // Get the normals of the objects.
	vec3 l = normalize(lightPos); // The direction of the light source.
	//vec3 rd = rayDir;
	vec3 r = reflect(l, n); // The reflection of the light off of the normal.
	vec3 kd = vec3(1.0, 1.0, 1.0); // The diffuse reflection coefficient, or the color of surface.
	vec3 ks = vec3(0.5); // Specular reflection coefficient.
	float s = 3.5; // Specular exponent, controlling the tightness of specular highlights.
	float diff = max(dot(n, l), 0.05); // Diffuse reflection calculation.
	float spec = pow(max(dot(r, rayDir), 0.5), s); // Specular reflection calculation.
	return kd * diff + ks * spec; // Combine diffuse and specular reflection components using material coefficients.
}

void main() {
  vec3 ro = ray_origin; // The origin of the ray, from the vertex shader.
  vec3 rd = ray_dir; // The direction of the ray, from the vertex shader.

  vec3 slice_min = box_min; // The minimum corner of the bounding box.
  vec3 slice_max = box_max; // The maximum corner of the bounding box.

  vec3 boxHit = rayBoxIntersect(slice_min, slice_max, ro, rd); // Calculate whether the ray intersects the bounding box.

  vec4 color = vec4(0.0); // The color of the pixel.

  // If the ray intersects the bounding box:
  if (boxHit.x > 0) {
    float dist = boxHit.y; // The distance to the bounding box.
    float dist_max = boxHit.z; // The maximum distance to the bounding box.
    vec3 box_inverse = 1.0 / box_max; // The inverse of the maximum boundary of the bounding box.

    // Iterate steps to find the intersection of the ray with the scene:
    for (int i = 0; i < max_steps && dist < dist_max; ++i) {
      vec3 ray_pos = ro + rd * dist; // The current position of the ray.
      ray_pos *= box_inverse; // Ray position in the bounding box space.
      float d = scene(ray_pos); // Apply the position of the ray to the signed distance field function.
      if (d < hitSurf) {
        color = vec4(lighting(d, ray_pos, rd), 1.0); // Shade the pixel based on the distance to the object.
        break;
      }
      dist += step_size; // Move the ray foward by step size.
    }
  }

  frag_out0 = color; // Send the pixel color out.

}
