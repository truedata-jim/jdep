// FileReader.h

#pragma once

#include <stdio.h>
#include <stdint.h>

class FileReader
{
public:
    FileReader(const char* path);
    ~FileReader();

    uint8_t ReadByte();
    uint32_t ReadLong();
    uint16_t ReadWord();
    uint8_t* ReadByteArray(int length);

private:
    FILE* mFile;
};
