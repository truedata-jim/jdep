// BytesDecoder.cpp

#include "BytesDecoder.h"
#include <assert.h>

BytesDecoder::BytesDecoder(const uint8_t* bytes, long length)
: mCursor(bytes)
, mLimit(bytes+length)
{
}

uint8_t BytesDecoder::DecodeByte()
{
    assert(mCursor < mLimit);
    uint8_t result = *mCursor++;
    assert(mCursor <= mLimit);
    return result;
}

uint16_t BytesDecoder::DecodeWord()
{
    assert(mCursor < mLimit);
    uint16_t result = (mCursor[0] << 8) | mCursor[1];
    mCursor += 2;
    assert(mCursor <= mLimit);
    return result;
}
