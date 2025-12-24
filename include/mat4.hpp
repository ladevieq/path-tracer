#ifndef __MAT4_HPP_
#define __MAT4_HPP_

#include <xmmintrin.h>

class mat4 {
    static mat4 projection();

    __m128 r1, r2, r3, r4;
};

#endif // __MAT4_HPP_
