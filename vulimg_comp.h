#ifndef VULIMG_COMPH
#define VULIMG_COMPH

#ifdef VULKAN
#pragma shader_stage(compute)

#define INT int
#define UINT uint
#define STRUCT

#else

#define INT int32_t
#define UINT uint32_t
#define U16 uint16_t
#define STRUCT struct
#pragma pack(push,1)

#endif

#define UGR 16
#define FLOAT float
#define TAIL 0xffffffff
#define EMPTY 0xfffffffe
#define DIDX 12

#ifdef VULKAN

struct VtlRect {
   INT left;
   INT top;
   UINT width;
   UINT height;
};

#endif

struct VigRect {
   UINT id;
   UINT link;
   UINT weight;
   UINT left, top, width, height;
};

struct VigCloud {
   UINT id;
   UINT link;
   UINT weight;
   FLOAT mx, my, dx, dy;
};

struct VigImgParam {
   UINT width;
   UINT height;
   UINT stride;
};

#ifdef VULKAN
struct VigTransform {
   FLOAT sx, ry, rx, sy, dx, dy;
};
#endif

struct VigCopyParams {
   STRUCT VigImgParam src;
   STRUCT VigImgParam dst;
   UINT sleft;
   UINT stop;
   UINT width;
   UINT height;
   UINT dleft;
   UINT dtop;
};

struct VigTransParams {
   STRUCT VigImgParam src;
   STRUCT VigImgParam dst;
   STRUCT VigTransform trans;
   INT compBits;
   INT compCount;
};

struct VigPyrParams {
   STRUCT VigImgParam src;
   STRUCT VigImgParam dst;
   INT compBits;
   INT compCount;
   UINT width;
   UINT height;
   UINT row;
};

struct VigWhiteParams {
   STRUCT VigImgParam img;
   FLOAT limit;
   FLOAT density;
   UINT minSize;
   UINT maxDist;
   UINT phase;
};

struct VigWCloudParams {
   STRUCT VigImgParam img;
   FLOAT maxDist;
   UINT phase;
};

struct VigJoinParams {
   UINT width;
   UINT height;
   UINT srcstride;
   UINT dststride;
   UINT index;
};

struct VigDSumParams {
   UINT mode;
   UINT astride;
   UINT bstride;
   UINT aleft;
   UINT atop;
   UINT bleft;
   UINT btop;
   UINT width;
   UINT height;
};

struct VigAddParams {
   STRUCT VigImgParam img;
   UINT compCount;
   UINT pixel;
   INT min;
   INT max;
};

struct VigRectParams {
   STRUCT VigImgParam img;
   STRUCT VtlRect rect;
   UINT pixSize;
   UINT pixVal;
};
   

#ifndef VULKAN
#pragma pack(pop)
#endif

#endif // VULIMG_COMPH
