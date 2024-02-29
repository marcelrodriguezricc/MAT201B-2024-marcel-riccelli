#version 330

// Variables rom the Application:
uniform vec3 boxMin, boxMax; // The minimum and maximum corners of the bounding box.
uniform int maxSteps; // The maximum amount of steps each ray of light can take before it is considered to have missed all objects.
uniform float stepSize; // The distance each ray of light travels per step.
uniform float time; // The time our application has been running.

// Variables from the Vertex Shader:
in vec3 ray_dir, ray_origin; // The direction and origin of the ray.

// Internal Variables:
vec4 color = vec4(1.0, 1.0, 1.0, 1.0); // The color of the metaballs.

layout (location = 0) out vec4 frag_out0; // The output color of the fragment shader.

// Check if the ray intersects the bounding box:
vec3 rayBoxIntersect(const vec3 box_min, const vec3 box_max, const vec3 r_o, const vec3 r_d) {
  vec3 inv_dir = 1.0 / r_d; // Inverse of the ray direction.
  vec3 tbot = inv_dir * (b_min - r_o); // The distance to the minimum corner of the bounding box.
  vec3 ttop = inv_dir * (b_max - r_o); // The distance to the maximum corner of the bounding box.
  vec3 tmin = min(ttop, tbot); // The minimum distance to the bounding box.
  vec3 tmax = max(ttop, tbot); // The maximum distance to the bounding box.

  // What's going on here?
  vec2 traverse = max(tmin.xx, tmin.yz);
  traverse = min(tmax.xx, tmax.yz);
  float traverse_low = max(traverse.x, traverse.y);
  float traverse_high = min(traverse.x, traverse.y);

  // Encode the different measurements of the boudning box to the ray in a vec3.
  return vec3(float(traverse_high > max(traverse_low, 0.0)), traverse_low, traverse_high);
}

// Signed distance field formula for a sphere.
float sphereSDF(vec3 center, float radius, vec3 toPoint){
  return length(center - toPoint) - radius;
}

// Signed distance field formula for a box.
float boxSDF(vec3 center, vec3 size, vec3 toPoint) {
  vec3 d = abs(center - toPoint) - size;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

// The SDF of our scene:
float scene(vec3 p){
  float d1 = sphereSDF(vec3(-0.1,0,0), 0.25, p); // Sphere one.
  float d2 = sphereSDF(vec3(0.1,0,0), 0.25, p); // Sphere two.
  float d = min(d1,d2); // Minimum function to create union between surfaces.
  return d;
}

// vec3 getNormals(in Surface s, in Ray r) {
//     const float e = 0.01;
//     vec3 p = r.o+s.t*r.d;
//     p -= noise.rgb;
//     float nx = scene(vec3(p.x+e,p.y,p.z)).t-scene(vec3(p.x-e,p.y,p.z)).t;
//     float ny = scene(vec3(p.x,p.y+e,p.z)).t-scene(vec3(p.x,p.y-e,p.z)).t;
//     float nz = scene(vec3(p.x,p.y,p.z+e)).t-scene(vec3(p.x,p.y,p.z-e)).t;
//     return normalize(vec3(nx,ny,nz));
// }

void main() {
  vec3 ro = ray_origin; // The origin of the ray, from the vertex shader.
  vec3 rd = ray_dir; // The direction of the ray, from the vertex shader.

  vec3 slice_min = box_min; // The minimum corner of the bounding box.
  vec3 slice_max = box_max; // The maximum corner of the bounding box.

  vec3 boxHit = rayBoxIntersect(slice_min, slice_max, ro, rd); // Calculate whether the ray intersects the bounding box.
  
  vec4 rayColor = vec4(0.0); // The color of the ray if it doesn't hit any of the objecs.

    // If the ray intersects the bounding box:
    if (boxHit.x > 0) {
      float dist = boxHit.y; // The distance to the bounding box.
      float dist_max = boxHit.z; // The maximum distance to the bounding box.
      vec3 box_inverse = 1.0 / box_max; // The inverse of the maximum corner of the bounding box.

      // Iterate steps to find the intersection of the ray with the scene:
      for (int i = 0; i < maxSteps && dist < dist_max && color.a < 1.; ++i) {
        vec3 ray_pos = ro + rd * dist; // The current position of the ray.
        ray_pos *= box_inverse; // Ray position in the bounding box space.
        float d = scene(ray_pos); // Apply the position of the ray to the signed distance field function.
        color = vec4(1.0); // If the ray hits the scene, set the color to white.
        dist += stepSize; // Move the ray foward by step size.
      }
    }

  frag_out0 = color; // Send the pixel color out.

}
