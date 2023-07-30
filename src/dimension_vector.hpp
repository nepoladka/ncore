#pragma once

#define DEFAULT_VEC_BEGIN(NUM)      template <class _t> union vec##NUM

#define DEFAULT_VEC_END(NUM)        typedef vec##NUM<__int8> vec##NUM##i8;              \
                                    typedef vec##NUM<unsigned __int8> vec##NUM##ui8;    \
                                    typedef vec##NUM<__int16> vec##NUM##i16;            \
                                    typedef vec##NUM<unsigned __int16> vec##NUM##ui16;  \
                                    typedef vec##NUM<__int32> vec##NUM##i32;            \
                                    typedef vec##NUM<unsigned __int32> vec##NUM##ui32;  \
                                    typedef vec##NUM<__int64> vec##NUM##i64;            \
                                    typedef vec##NUM<unsigned __int64> vec##NUM##ui64;  \
                                    typedef vec##NUM<float> vec##NUM##f;                \
                                    typedef vec##NUM<long float> vec##NUM##lf;

#define DEFAULT_VEC_HEADER(NUM)     struct { _t VEC_##NUM##_AXIS; }axis; _t array[NUM];                                                                 \
                                    VEC_##NUM##_CONSTRUCTOR                                                                                             \
                                    __forceinline vec##NUM(_t array[NUM]) { for(unsigned long long i = 0; i < NUM; i++) this->array[i] = array[i]; };

#define DEFAULT_VEC_OPERATORS(NUM)  __forceinline vec##NUM* data() { return this; }                                             \
                                    __forceinline _t& operator[](unsigned long long idx) noexcept { return array[idx]; }        \
                                    __forceinline _t* operator&() { return array; }                                             \
                                    __forceinline bool operator==(const vec##NUM& right) { for(unsigned long long i=0; i< sizeof(_t) * NUM; i++) if(this->array[i] != right.array[i]) return false; return true; } \
                                    __forceinline bool operator!=(const vec##NUM& right) { return !(*this == right); }

#define IMP_VEC(NUM)                DEFAULT_VEC_BEGIN(NUM) { DEFAULT_VEC_HEADER(NUM); DEFAULT_VEC_OPERATORS(NUM); VEC_##NUM##_OPERATOR(+); VEC_##NUM##_OPERATOR(-); VEC_##NUM##_OPERATOR(*); VEC_##NUM##_OPERATOR(/ ); VEC_##NUM##_OPERATOR(+= ); VEC_##NUM##_OPERATOR(-= ); VEC_##NUM##_OPERATOR(*= ); VEC_##NUM##_OPERATOR(/= ); VEC_##NUM##_BOOL_OPERATOR(>= ); VEC_##NUM##_BOOL_OPERATOR(<= ); VEC_##NUM##_BOOL_OPERATOR(> ); VEC_##NUM##_BOOL_OPERATOR(< );  }; DEFAULT_VEC_END(NUM);


#define VEC_2_AXIS                  x, y
#define VEC_3_AXIS                  x, y, z
#define VEC_4_AXIS                  x, y, z, w

#define VEC_2_CONSTRUCTOR           __forceinline vec2(_t x = _t(), _t y = _t()) noexcept                           { axis.x = x; axis.y = y; }
#define VEC_3_CONSTRUCTOR           __forceinline vec3(_t x = _t(), _t y = _t(), _t z = _t()) noexcept              { axis.x = x; axis.y = y; axis.z = z; }
#define VEC_4_CONSTRUCTOR           __forceinline vec4(_t x = _t(), _t y = _t(), _t z = _t(), _t w = _t()) noexcept { axis.x = x; axis.y = y; axis.z = z; axis.w = w; }

#define VEC_2_OPERATOR(OPERAND)     __forceinline vec2 operator OPERAND (vec2 right) { return vec2(axis.x OPERAND right.axis.x, axis.y OPERAND right.axis.y); }
#define VEC_3_OPERATOR(OPERAND)     __forceinline vec3 operator OPERAND (vec3 right) { return vec3(axis.x OPERAND right.axis.x, axis.y OPERAND right.axis.y, axis.z OPERAND right.axis.z); }
#define VEC_4_OPERATOR(OPERAND)     __forceinline vec4 operator OPERAND (vec4 right) { return vec4(axis.x OPERAND right.axis.x, axis.y OPERAND right.axis.y, axis.z OPERAND right.axis.z, axis.w OPERAND right.axis.w); }

#define VEC_2_BOOL_OPERATOR(OPERAND)    __forceinline bool operator OPERAND (vec2 right) { return axis.x OPERAND right.axis.x && axis.y OPERAND right.axis.y; }
#define VEC_3_BOOL_OPERATOR(OPERAND)    __forceinline bool operator OPERAND (vec3 right) { return axis.x OPERAND right.axis.x && axis.y OPERAND right.axis.y && axis.z OPERAND right.axis.z; }
#define VEC_4_BOOL_OPERATOR(OPERAND)    __forceinline bool operator OPERAND (vec4 right) { return axis.x OPERAND right.axis.x && axis.y OPERAND right.axis.y && axis.z OPERAND right.axis.z && axis.w OPERAND right.axis.w; }


namespace ncore {
    IMP_VEC(2); //vec2
    IMP_VEC(3); //vec3
    IMP_VEC(4); //vec4

    typedef union col4 {
        struct {
            unsigned char red, green, blue, alpha;
        }chanel;
        unsigned ui32;

        __forceinline col4(unsigned ui32 = 0x0) {
            *this = *(col4*)&ui32;
        }

        __forceinline col4(unsigned char* chanel) {
            *this = *(col4*)chanel;
        }

        __forceinline col4(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
            chanel.red = red;
            chanel.green = green;
            chanel.blue = blue;
            chanel.alpha = alpha;
        }

        __forceinline bool operator==(const col4& right) {
            return ui32 == right.ui32; 
        }

        __forceinline bool operator!=(const col4& right) { 
            return !(*this == right);
        }

        __forceinline unsigned* operator&() {
            return (unsigned*)this; 
        }

        __forceinline col4 operator+(const col4& right) {
            return col4(
                (chanel.red + right.chanel.red) / 2, 
                (chanel.green + right.chanel.green) / 2, 
                (chanel.blue + right.chanel.blue) / 2, 
                (chanel.alpha + right.chanel.alpha) / 2); 
        }
    }rgba;

    typedef struct vrgba {
        rgba color;
        bool visible;
    }exrgba;
}