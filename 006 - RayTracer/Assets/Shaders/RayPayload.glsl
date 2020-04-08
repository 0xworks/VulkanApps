struct RayPayload
{
   vec4 attenuationAndDistance; // rgb,t
   vec4 emission;               // rgb,notused
   vec4 scatterDirection;       // xyz,isScattered
   uint randomSeed;
};
