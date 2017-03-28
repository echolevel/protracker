#include <stdint.h>
#include "pt_blep.h"
#include "pt_helpers.h"

static const uint32_t blepData[48] =
{
    0x3F7FE1F1, 0x3F7FD548, 0x3F7FD6A3, 0x3F7FD4E3, 0x3F7FAD85, 0x3F7F2152,
    0x3F7DBFAE, 0x3F7ACCDF, 0x3F752F1E, 0x3F6B7384, 0x3F5BFBCB, 0x3F455CF2,
    0x3F26E524, 0x3F0128C4, 0x3EACC7DC, 0x3E29E86B, 0x3C1C1D29, 0xBDE4BBE6,
    0xBE3AAE04, 0xBE48DEDD, 0xBE22AD7E, 0xBDB2309A, 0xBB82B620, 0x3D881411,
    0x3DDADBF3, 0x3DE2C81D, 0x3DAAA01F, 0x3D1E769A, 0xBBC116D7, 0xBD1402E8,
    0xBD38A069, 0xBD0C53BB, 0xBC3FFB8C, 0x3C465FD2, 0x3CEA5764, 0x3D0A51D6,
    0x3CEAE2D5, 0x3C92AC5A, 0x3BE4CBF7, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};

void blepAdd(blep_t *b, float offset, float amplitude)
{
    int8_t n;
    uint32_t i;
    const float *blepSrc;
    float f;

    // ad [22/2/15]: these assertions are better than a fall-through!
    //
    // TODO: test the error condition and remove the fall-through
    // after a bug-fix if required, then restore the assertions.
    //

    PT_ASSERT(offset >= 0.0f);
    PT_ASSERT(offset <= 1.0f);
    if ((offset < 0.0f) || (offset > 1.0f))
        return;

    i  = (uint32_t)(offset * BLEP_SP);
    blepSrc = (const float *)(blepData) + i + BLEP_OS;

    f = (offset * BLEP_SP) - i;
    i = b->index;
    n = BLEP_NS;

    while (n--)
    {
        PT_ASSERT((blepSrc < (const float *)(&blepData[47])) && (i <= BLEP_RNS));

        b->buffer[i] += (amplitude * LERP(blepSrc[0], blepSrc[1], f));
        blepSrc      += BLEP_SP;

        i++;
        i &= BLEP_RNS;
    }

    b->samplesLeft = BLEP_NS;
}

float blepRun(blep_t *b)
{
    float blepOutput;

    blepOutput = b->buffer[b->index];
    b->buffer[b->index] = 0.0f;

    b->index++;
    b->index &= BLEP_RNS;

    b->samplesLeft--;

    return (blepOutput);
}
