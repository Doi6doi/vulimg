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

struct Vig_Rect {
   UINT id;
   UINT link;
   UINT weight;
   UINT left, top, width, height;
};

struct Vig_Cloud {
   UINT id;
   UINT link;
   UINT weight;
   FLOAT mx, my, dx, dy;
};

struct Vig_ImgParam {
   UINT width;
   UINT height;
   UINT stride;
};

#ifdef VULKAN

struct Vyt_FTrans2 {
   FLOAT sx, ry, rx, sy, dx, dy;
};
#endif

struct Vig_CopyParams {
   STRUCT Vig_ImgParam src;
   STRUCT Vig_ImgParam dst;
   UINT sleft;
   UINT stop;
   UINT width;
   UINT height;
   UINT dleft;
   UINT dtop;
};

struct Vig_TransParams {
   STRUCT Vig_ImgParam src;
   STRUCT Vig_ImgParam dst;
   STRUCT Vyt_FTrans2 trans;
   INT compBits;
   INT compCount;
};

struct Vig_PyrParams {
   STRUCT Vig_ImgParam src;
   STRUCT Vig_ImgParam dst;
   INT compBits;
   INT compCount;
   UINT width;
   UINT height;
   UINT row;
};

struct Vig_WhiteParams {
   STRUCT Vig_ImgParam img;
   FLOAT limit;
   FLOAT density;
   UINT minSize;
   UINT maxDist;
   UINT phase;
};

struct Vig_WCloudParams {
   STRUCT Vig_ImgParam img;
   FLOAT maxDist;
   UINT phase;
};

struct Vig_JoinParams {
   UINT width;
   UINT height;
   UINT srcstride;
   UINT dststride;
   UINT index;
};

struct Vig_DiffParams {
   STRUCT Vig_ImgParam img;
   UINT alpha;
};

struct Vig_DSumParams {
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

struct Vig_AddParams {
   STRUCT Vig_ImgParam img;
   UINT compCount;
   UINT pixel;
   INT min;
   INT max;
};

struct Vig_RectParams {
   STRUCT Vig_ImgParam img;
   UINT left;
   UINT top;
   UINT width;
   UINT height;
   UINT pixSize;
   UINT pixVal;
};
   
struct Vig_DeltaParams {
   STRUCT Vig_ImgParam img;
   UINT comps;
   UINT top;
   INT dx;
   INT dy;
};


#ifndef VULKAN
#pragma pack(pop)
#endif

#endif // VULIMG_COMPH
