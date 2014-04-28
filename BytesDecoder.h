// BytesDecoder.h

#pragma once

#include <stdint.h>

class BytesDecoder
{
public:
    BytesDecoder(const uint8_t* bytes, long length);

    uint8_t DecodeByte();

    uint16_t DecodeWord();

private:
    const uint8_t* mCursor;
    const uint8_t* mLimit;
};

