// ClassFile.h

#pragma once

#include <stdint.h>

#define CONSTANT_Class                   7
#define CONSTANT_Double                  6
#define CONSTANT_Fieldref                9
#define CONSTANT_Float                   4
#define CONSTANT_Integer                 3
#define CONSTANT_InterfaceMethodref     11
#define CONSTANT_Long                    5
#define CONSTANT_Methodref              10
#define CONSTANT_NameAndType            12
#define CONSTANT_String                  8
#define CONSTANT_Utf8                    1

struct cp_info
{
    int tag;            /* CONSTANT_xxxx */

    cp_info(int _tag) : tag(_tag) {}
};

cp_info* const kLongTag = ((cp_info*) -1);

struct constant_class_info : public cp_info
{
    uint16_t name_index;

    constant_class_info(uint16_t index) : cp_info(CONSTANT_Class), name_index(index) {}
};

struct constant_utf8_info  : public cp_info
{
    char* str;

    constant_utf8_info(char* _str) : cp_info(CONSTANT_Utf8), str(_str) {}
};

struct attribute_info
{
    uint16_t attribute_name_index;
    long attribute_length;
    uint8_t* info;
    attribute_info* next;

    attribute_info(uint16_t _attribute_name_index, long _attribute_length
                   , uint8_t* _info, attribute_info* _next)
        : attribute_name_index(_attribute_name_index)
        , attribute_length(_attribute_length)
        , info(_info)
        , next(_next)
    {}
};

class BytesDecoder;
class FileReader;
class ClassFileAnalyzer;

class ClassFile
{
public:
    ClassFile(const char* filename);

    void findDepsInFile(const char* target, ClassFileAnalyzer& analyzer);

    void scanAnnotation(BytesDecoder& decoder, ClassFileAnalyzer& analyzer);
    void scanElementValue(BytesDecoder& decoder, ClassFileAnalyzer& analyzer);

private:

    const char* getString(int index) const;
    const char* getClassName(int index) const;
    cp_info** readConstantPool(FileReader& reader, const char* filename, int count);
    cp_info*  readConstantPoolInfo(FileReader& reader, const char* filename);
    attribute_info* readFields(FileReader& reader, int count, attribute_info* atts);
    attribute_info* readFieldInfo(FileReader& reader, attribute_info* atts);
    attribute_info* readMethods(FileReader& reader, int count, attribute_info* atts);
    attribute_info* readMethodInfo(FileReader& reader, attribute_info* atts);
    attribute_info* readAttributes(FileReader& reader, int count, attribute_info* atts);
    attribute_info* readAttributeInfo(FileReader& reader, attribute_info* atts);

    void skipWordArray(FileReader& reader, int length);

private:
    uint16_t mConstantPoolCount;
    cp_info** mConstantPool;
    attribute_info* mAttributes;
};

