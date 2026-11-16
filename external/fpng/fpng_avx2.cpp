// Optional AVX2 implementation for BGRA32 filtering.
// Compile this translation unit with AVX2 enabled (/arch:AVX2 on MSVC,
// -mavx2 on GCC/Clang). Keep it separate from fpng.cpp so the baseline
// encoder does not acquire an AVX2 compiler target.

#include <stdint.h>

#if (defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__))
#include <immintrin.h>

namespace fpng
{
    // Converts BGRA to RGBA. If pPrev is non-null, applies PNG Up filtering
    // (current row minus previous row) before channel reordering.
    // Processes eight pixels (32 bytes) per AVX2 iteration; scalar tail
    // handles widths not divisible by eight.
    void fpng_apply_bgra32_avx2(
        const uint8_t* pSrc,
        const uint8_t* pPrev,
        uint8_t* pDst,
        uint32_t width)
    {
        const __m256i shuffle_mask = _mm256_setr_epi8(
            2, 1, 0, 3,  6, 5, 4, 7,
            10, 9, 8, 11, 14, 13, 12, 15,
            2, 1, 0, 3,  6, 5, 4, 7,
            10, 9, 8, 11, 14, 13, 12, 15);

        uint32_t x = 0;
        for (; width - x >= 8; x += 8)
        {
            __m256i pixels = _mm256_loadu_si256(
                reinterpret_cast<const __m256i*>(pSrc + x * 4));

            if (pPrev)
            {
                const __m256i previous = _mm256_loadu_si256(
                    reinterpret_cast<const __m256i*>(pPrev + x * 4));
                pixels = _mm256_sub_epi8(pixels, previous);
            }

            pixels = _mm256_shuffle_epi8(pixels, shuffle_mask);

            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(pDst + x * 4), pixels);
        }

        // Scalar tail.
        for (; x < width; ++x)
        {
            const uint8_t* s = pSrc + x * 4;
            const uint8_t* p = pPrev ? (pPrev + x * 4) : nullptr;
            uint8_t* d = pDst + x * 4;

            if (p)
            {
                d[0] = static_cast<uint8_t>(s[2] - p[2]);
                d[1] = static_cast<uint8_t>(s[1] - p[1]);
                d[2] = static_cast<uint8_t>(s[0] - p[0]);
                d[3] = static_cast<uint8_t>(s[3] - p[3]);
            }
            else
            {
                d[0] = s[2];
                d[1] = s[1];
                d[2] = s[0];
                d[3] = s[3];
            }
        }

        // Avoid AVX-to-SSE transition penalties when returning to baseline code.
        _mm256_zeroupper();
    }
}
#else
// Non-x86 builds never reference this function from fpng.cpp.
#endif
