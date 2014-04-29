// Classfile.cpp

#include "ClassFile.h"

#include "BytesDecoder.h"
#include "ClassFileAnalyzer.h"
#include "FileReader.h"

#include <stdlib.h>

ClassFile::ClassFile(const char* infilename)
    : mConstantPoolCount(0)
    , mConstantPool(0)
    , mAttributes(0)
{
    FileReader reader(infilename);

    reader.ReadLong(); // magic
    reader.ReadWord(); // minor_version
    reader.ReadWord(); // major_version
    mConstantPoolCount = reader.ReadWord();
    mConstantPool = readConstantPool(reader, infilename, mConstantPoolCount);
    reader.ReadWord(); // access_flags
    reader.ReadWord(); // this_class
    reader.ReadWord(); // super_class
    uint16_t interfaces_count = reader.ReadWord();
    skipWordArray(reader, interfaces_count); // interfaces
    uint16_t fields_count = reader.ReadWord();
    mAttributes = readFields(reader, fields_count, mAttributes); // fields
    uint16_t methods_count = reader.ReadWord();
    mAttributes = readMethods(reader, methods_count, mAttributes); // methods
    uint16_t attributes_count = reader.ReadWord();
    mAttributes = readAttributes(reader, attributes_count, mAttributes);
}

attribute_info* ClassFile::readAttributeInfo(FileReader& reader, attribute_info* atts)
{
    uint16_t attribute_name_index = reader.ReadWord();
    long attribute_length = reader.ReadLong();
    uint8_t* info = reader.ReadByteArray(attribute_length);

    return new attribute_info(attribute_name_index, attribute_length, info, atts);
}

attribute_info* ClassFile::readAttributes(FileReader& reader, int count, attribute_info* atts)
{
    for (int i=0; i<count; ++i)
        atts = readAttributeInfo(reader, atts);
    return atts;
}

void ClassFile::scanAnnotation(BytesDecoder& decoder, ClassFileAnalyzer& analyzer)
{
    int type_index = decoder.DecodeWord();
    const char* name = getClassName(type_index);
    if (analyzer.isIncludedClass(name))
        analyzer.addDep(name);
    int num_element_value_pairs = decoder.DecodeWord();
    for (int i = 0; i < num_element_value_pairs; ++i)
    {
        decoder.DecodeWord(); // skip over element_name_index
        scanElementValue(decoder, analyzer);
    }
}

void ClassFile::scanElementValue(BytesDecoder& decoder, ClassFileAnalyzer& analyzer)
{
    uint8_t tag = decoder.DecodeByte();
    switch (tag)
    {
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z':
        {
            decoder.DecodeWord(); // skip over const_value_index
            break;
        }
        case 's':
        {
            decoder.DecodeWord(); // skip over const_value_index
            break;
        }
        case 'c':
        {
            decoder.DecodeWord(); // skip over class_info_index
            break;
        }
        case 'e':
        {
            int type_name_index = decoder.DecodeWord();
            const char* name = getClassName(type_name_index);
            decoder.DecodeWord(); // skip over const_name_index
            if (analyzer.isIncludedClass(name))
                analyzer.addDep(name);
            break;
        }
        case '@':
        {
            scanAnnotation(decoder, analyzer);
            break;
        }
        case '[':
        {
            int num_values = decoder.DecodeWord();
            int i;
            for (i = 0; i < num_values; ++i)
                scanElementValue(decoder, analyzer);
            break;
        }
        default:
            break;
    }
}

void ClassFile::findDepsInFile(const char* target, ClassFileAnalyzer& analyzer)
{
    for (int i=0; i < mConstantPoolCount; ++i)
    {
        cp_info* cp = mConstantPool[i];
        if (cp && cp->tag == CONSTANT_Class)
        {
            const char* name = getClassName(i);
            if (analyzer.isIncludedClass(name))
            {
                if (name[0] != '[')   /* Skip array classes */
                {
                    char* dollar = index(name, '$');
                    if (dollar)
                    {
                        /* It's an inner class */
                        if (strncmp(name, target, dollar-name) == 0)
                        {
                            /* It's one of target's inner classes, so we
                               depend on whatever *it* depends on and thus
                               we need to recurse. */
                            bool added = analyzer.addDep(name);
                            if (added)
                            {
                                analyzer.findDeps(name);    // Recurses here!!
                            }
                        }
                        else
                        {
                            /* It's somebody else's inner class, so we
                               depend on its outer class source file */
                            *dollar = '\0';
                            analyzer.addDep(name);
                            *dollar = '$';
                        }
                    }
                    else
                    {
                        /* It's a regular class */
                        analyzer.addDep(name);
                    }
                }
            }
        }
    }

    attribute_info* att = mAttributes;
    while (att != NULL)
    {
        const char* name = getString(att->attribute_name_index);
        if (strcmp(name, "RuntimeVisibleAnnotations") == 0)
        {
            int i;
            BytesDecoder decoder(att->info, att->attribute_length);
            int num_annotations = decoder.DecodeWord();
            for (i = 0; i < num_annotations; ++i)
                scanAnnotation(decoder, analyzer);
        }
        att = att->next;
    }
}

cp_info** ClassFile::readConstantPool(FileReader& reader, const char* filename, int count)
{
    cp_info** result = new cp_info*[count];
    int i;
    result[0] = NULL;
    for (i=1; i<count; ++i)
    {
        result[i] = readConstantPoolInfo(reader, filename);
        if (result[i] == kLongTag)
        {
            result[i] = NULL;
            result[++i] = NULL;
        }
    }
    return result;
}

cp_info* ClassFile::readConstantPoolInfo(FileReader& reader, const char* filename)
{
    uint8_t tag = reader.ReadByte();
    switch (tag)
    {
        case CONSTANT_Class:
        {
            uint16_t name_index = reader.ReadWord();
            return new constant_class_info(name_index);
        }
        case CONSTANT_Fieldref:
        {
            reader.ReadWord(); // class_index
            reader.ReadWord(); // name_and_type_index
            return NULL;
        }
        case CONSTANT_Methodref:
        {
            reader.ReadWord(); // class_index
            reader.ReadWord(); // name_and_type_index
            return NULL;
        }
        case CONSTANT_InterfaceMethodref:
        {
            reader.ReadWord(); // class_index
            reader.ReadWord(); // name_and_type_index
            return NULL;
        }
        case CONSTANT_String:
        {
            reader.ReadWord(); // string_index
            return NULL;
        }
        case CONSTANT_Integer:
        {
            reader.ReadLong(); // bytes
            return NULL;
        }
        case CONSTANT_Float:
        {
            reader.ReadLong(); // bytes
            return NULL;
        }
        case CONSTANT_Long:
        {
            reader.ReadLong(); // high_bytes
            reader.ReadLong(); // low_bytes
            return kLongTag;
        }
        case CONSTANT_Double:
        {
            reader.ReadLong(); // high_bytes
            reader.ReadLong(); // low_bytes
            return kLongTag;
        }
        case CONSTANT_NameAndType:
        {
            reader.ReadWord(); // name_index
            reader.ReadWord(); // descriptor_index
            return NULL;
        }
        case CONSTANT_Utf8:
        {
            uint16_t length = reader.ReadWord();
            char* str = (char*) reader.ReadByteArray(length);
            return new constant_utf8_info(str);
        }
        default:
        {
            fprintf(stderr, "invalid constant pool tag %d in %s\n", tag,
                    filename);
            exit(1);
        }
    }
    return NULL;
}

attribute_info* ClassFile::readFieldInfo(FileReader& reader, attribute_info* atts)
{
    reader.ReadWord(); // access_flags
    reader.ReadWord(); // name_index
    reader.ReadWord(); // descriptor_index
    uint16_t attributes_count = reader.ReadWord();
    return readAttributes(reader, attributes_count, atts);
}

attribute_info* ClassFile::readFields(FileReader& reader, int count, attribute_info* atts)
{
    int i;
    for (i=0; i<count; ++i)
        atts = readFieldInfo(reader, atts);
    return atts;
}

attribute_info* ClassFile::readMethodInfo(FileReader& reader, attribute_info* atts)
{
    reader.ReadWord(); // access_flags
    reader.ReadWord(); // name_index
    reader.ReadWord(); // descriptor_index
    uint16_t attributes_count = reader.ReadWord();
    return readAttributes(reader, attributes_count, atts);
}

attribute_info* ClassFile::readMethods(FileReader& reader, int count, attribute_info* atts)
{
    for (int i = 0; i < count; ++i)
        atts = readMethodInfo(reader, atts);
    return atts;
}

void ClassFile::skipWordArray(FileReader& reader, int length)
{
    for (int i = 0; i < length; ++i)
        reader.ReadWord();
}

const char* ClassFile::getString(int index) const
{
    cp_info* cp = mConstantPool[index];
    cp = mConstantPool[index];
    if (cp->tag == CONSTANT_Utf8)
    {
        const char* name = ((constant_utf8_info*) cp)->str;
        return name;
    }
    return NULL;
}

const char* ClassFile::getClassName(int index) const
{
    cp_info* cp = mConstantPool[index];
    if (cp == NULL)
        return NULL;
    else if (cp->tag == CONSTANT_Class)
    {
        constant_class_info* classInfo = (constant_class_info*) cp;
        return getString(classInfo->name_index);
    }
    else if (cp->tag == CONSTANT_Utf8)
    {
        char* name = ((constant_utf8_info*) cp)->str;
        if (name[0] == 'L')
        {
            char* result = strdup(name + 1);
            char* s = result;
            while (*s)
            {
                if (*s == ';')
                {
                    *s = '\0';
                    break;
                }
                ++s;
            }
            return result;
        }
    }
    return NULL;
}
