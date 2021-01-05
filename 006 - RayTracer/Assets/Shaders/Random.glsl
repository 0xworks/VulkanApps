#extension GL_EXT_control_flow_attributes : require


// Generates a seed for a random number generator from 2 inputs plus a backoff
// https://github.com/nvpro-samples/optix_prime_baking/blob/master/random.h
// https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
uint InitRandomSeed(uint val0, uint val1) {
   uint v0 = val0, v1 = val1, s0 = 0;
   
   [[unroll]] 
   for (uint n = 0; n < 16; n++) {
      s0 += 0x9e3779b9;
      v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
      v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
   }

   return v0;
}


uint RandomInt(inout uint seed) {
   // This is the "ranqd1" algorithm from Numerical Recipes.
   // Numerical Recipes has this to say about "ranqd1":
   //    The quick and dirty generators are very fast, but they are machine dependent, nonportable, and at best
   //    only as good as a 32-bit linear congruential generator ever is - in our view not good enough
   //    in many situations.
   //    We would use these only in very special cases, where speed is critical.
   seed = 1664525 * seed + 1013904223;
   return seed;
}


// Generates random float in half open interval [0.0, 1.0)
float RandomFloat(inout uint seed) {
   // This is "ranqd2" from Numerical Recipes.
   // Similar comments to "ranqd1" above apply.
   const uint one = 0x3f800000;
   const uint msk = 0x007fffff;
   return uintBitsToFloat(one | (msk & (RandomInt(seed) >> 9))) - 1.0;
}


// Generates random float in half open interval [minValue, maxValue)
float RandomFloat(float minValue, float maxValue, inout uint seed) {
   return minValue + (maxValue - minValue) * RandomFloat(seed);
}


// Generates random points inside the unit disk.
// This excludes points right on the boundary of the disk.
vec2 RandomInUnitDisk(inout uint seed) {
   vec2 p;
   do {
      p = 2.0 * vec2(RandomFloat(seed), RandomFloat(seed)) - 1.0;
   } while (dot(p, p) >= 1.0f);
   return p;
}


// Generates random points inside the unit sphere.
// This excludes points right on the surface of the sphere.
vec3 RandomInUnitSphere(inout uint seed) {
   vec3 p;
   do {
      p = 2.0 * vec3(RandomFloat(seed), RandomFloat(seed), RandomFloat(seed)) - 1.0;
   } while (dot(p, p) >= 1.0);
   return p;
}


// Generate a random vector of unit length.
// Note that this particular algorithm will never generate (0, 0, 1).
// It can generate any other unit vector, including (0, 0, -1), (1, 0, 0), and (0, 1, 0).
vec3 RandomUnitVector(inout uint seed) {
   const float a = RandomFloat(0.0, 2.0 * 3.1415926535897932384626433832795, seed);
   const float z = RandomFloat(-1.0, 1.0, seed);  // z is random float from -1.0 up to (but not including) 1.0
   const float r = sqrt(1.0 - z * z);
   return vec3(r * cos(a), r * sin(a), z);
}


mat3x3 GetOrthoNormalBasis(const vec3 normal) {
   vec3 helper = vec3(1.0, 0.0, 0.0);
   if (abs(normal.x) > 0.99f) {
      helper = vec3(0.0, 0.0, 1.0);
   }
   vec3 tangent = normalize(cross(normal, helper));
   vec3 bitangent = normalize(cross(normal, tangent));
   return mat3x3(tangent, bitangent, normal);
}


// alpha = 0 -> uniform sampling
// alpha = 1 -> cosine sampling
// alpha > 1 -> phong sampling
vec3 RandomOnUnitHemisphere(const vec3 normal, const float alpha, inout uint seed) {
   const float cosTheta = pow((1.0 - RandomFloat(0.0, 1.0, seed)), 1.0 / (alpha + 1.0));
   const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
   const float phi = RandomFloat(0.0, 2.0 * 3.1415926535897932384626433832795, seed);
   return GetOrthoNormalBasis(normal) * vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}


// Generate a unit vector on the hemisphere about specified normal,
// with uniform probability distribution.
// Excludes vectors exactly perpendicular to the normal.
vec3 UniformSampleHemisphere(const vec3 normal, inout uint seed) {
   return RandomOnUnitHemisphere(normal, 0.0, seed);
}


float UniformPDFHemisphere(const vec3 normal, const vec3 direction) {
   return 1.0 / (2.0 * 3.1415926535897932384626433832795);
}


// Generate a unit vector on the hemisphere about specified normal,
// with probability proportional to the cosine of the angle between
// generated vector and the normal.
// Excludes vectors exactly perpendicular to the normal
// (these have cos(theta) = 0, and therefore probability = 0, anyway)
vec3 CosineSampleHemisphere(const vec3 normal, inout uint seed) {
   return RandomOnUnitHemisphere(normal, 1.0, seed);
}


float CosinePDFHemisphere(const vec3 normal, const vec3 direction) {
   const float cosTheta = dot(normal, direction);
   return cosTheta < 0.0? 0.0 : cosTheta / 3.1415926535897932384626433832795;
}
