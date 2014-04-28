// FileReader.cpp

#include "FileReader.h"

#include <stdlib.h>

bool testEndianism()
{
    union {
        uint16_t asWord;
        uint8_t asBytes[2];
    } tester;

    tester.asWord = 1;
    return tester.asBytes[0];
}

void reverseBytes(char *data, int length)
{
    char temp;
    int i;

    for (i = 0; i<length/2; ++i) {
        temp = data[i];
        data[i] = data[length-1-i];
        data[length-1-i] = temp;
    }
}

static bool gLittleEndian = testEndianism();

FileReader::FileReader(const char* path)
: mFile(fopen(path, "rb"))
{
    if (!mFile)
    {
        fprintf(stderr, "unable to open class file %s\n", path);
        exit(1);
    }
}

FileReader::~FileReader()
{
    fclose(mFile);
}


uint32_t FileReader::ReadLong()
{
    uint32_t result;
    fread((char *)&result, 4, 1, mFile);
    if (gLittleEndian) {
        reverseBytes((char *)&result, 4);
    }
    return result;
}

uint16_t FileReader::ReadWord()
{
    uint16_t result;
    fread((char *)&result, 2, 1, mFile);
    if (gLittleEndian) {
        reverseBytes((char *)&result, 2);
    }
    return result;
}

uint8_t FileReader::ReadByte()
{
    uint8_t result;
    fread((char *)&result, 1, 1, mFile);
    return result;
}

uint8_t* FileReader::ReadByteArray(int length)
{
    uint8_t *result = (uint8_t*) malloc(length + 1);
    fread((char *)result, 1, length, mFile);
    result[length] = 0;
    return result;
}

