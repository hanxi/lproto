#include <stdio.h>
#include <inttypes.h>

static uint64_t pack754(long double f, uint8_t bits, uint8_t expbits)
{
    long double fnorm;
    int shift;
    uint64_t sign;
    uint64_t exp;
    uint64_t significand;
    uint8_t significandbits = bits - expbits - 1;

    if (f == 0.0) {
        return 0;
    }

    if (f < 0) {
        sign = 1;
        fnorm = -f;
    } else {
        sign = 0;
        fnorm = f;
    }

    shift = 0;
    while (fnorm >= 2.0) {
        fnorm /= 2.0;
        shift++;
    }
    while (fnorm < 1.0) {
        fnorm *= 2.0;
        shift--;
    }
    fnorm = fnorm - 1.0;

    significand = fnorm * ((1LL<<significandbits) + 0.5f);
    // get the biased exponent
    exp = shift + ((1<<(expbits-1)) - 1); // shift + bias
    return (sign<<(bits-1)) | (exp<<significandbits) | significand;
}

static long double unpack754(uint64_t i, uint8_t bits, uint8_t expbits)
{
    long double result;
    uint64_t shift;
    unsigned bias;
    uint8_t significandbits = bits - expbits - 1;

    if (i == 0) {
        return 0.0;
    }
    // pull the significand
    result = (i&((1LL<<significandbits)-1)); // mask
    result /= (1LL<<significandbits);
    result += 1.0f;

    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits) & ((1LL<<expbits)-1)) - bias;
    while (shift > 0) {
        result *= 2.0;
        shift--;
    }
    while (shift < 0) {
        result /= 2.0;
        shift++;
    }
    result *= (i>>(bits-1))&1 ? -1.0 : 1.0;
    return result;
}

int main()
{
    float f = 3.1415926,f2;
    uint32_t fi;
    fi = pack754(f,32,8);
    f2 = unpack754(fi,32,8);
    printf("float before:%.7f\n",f);
    printf("float encoded: 0x%08" PRIx32 "\n",fi);
    printf("float after : %.7f\n\n",f2);
    return 0;
}
