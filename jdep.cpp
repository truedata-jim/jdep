/*
  jdep.cpp -- Java .class file dependency analyzer

  Derived from work by:
  Copyright 2009 Chip Morningstar


  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  Written by Chip Morningstar.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <string>
#include <set>

#include "BytesDecoder.h"
#include "FileReader.h"

using std::string;
using std::set;

#define FREE(p)    free(p)
#define ALLOC(n)   malloc(n)

#define TYPE_ALLOC(type)          ((type *) ALLOC(sizeof(type)))
#define TYPE_ALLOC_MULTI(type, n) ((type *) ALLOC(sizeof(type) * (n)))

#define USAGE "usage: jdep -a [-e PACKAGE] [-i PACKAGE] -h [-c CPATH] [-d DPATH] [-j JPATH] files...\n"

const char *ClassRoot = "";
const char *DepRoot = "";
const char *JavaRoot = "";

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

struct cp_info {
    int tag;            /* CONSTANT_xxxx */
};

struct attribute_info {
    attribute_info *next;
    uint16_t attribute_name_index;
    long attribute_length;
    uint8_t *info;
};

struct constant_class_info {
    int tag;            /* CONSTANT_Class */
    uint16_t name_index;
};

struct constant_utf8_info {
    int tag;            /* CONSTANT_Utf8 */
    char *str;
};

class ClassFileAnalyzer;

class ClassFile
{
public:
    ClassFile(const char* filename);

    const char* getString(int index) const
    {
        cp_info* cp = mConstantPool[index];
        cp = mConstantPool[index];
        if (cp->tag == CONSTANT_Utf8) {
            const char* name = ((constant_utf8_info *) cp)->str;
            return name;
        }
        return NULL;
    }

    const char* getClassName(int index) const
    {
        cp_info *cp = mConstantPool[index];
        if (cp == NULL) {
            return NULL;
        } else if (cp->tag == CONSTANT_Class) {
            constant_class_info *classInfo = (constant_class_info *) cp;
            return getString(classInfo->name_index);
        } else if (cp->tag == CONSTANT_Utf8) {
            char *name = ((constant_utf8_info *) cp)->str;
            if (name[0] == 'L') {
                char *result = strdup(name + 1);
                char *s = result;
                while (*s) {
                    if (*s == ';') {
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

    void findDepsInFile(const char* target, ClassFileAnalyzer& analyzer);

    void scanAnnotation(BytesDecoder& decoder, ClassFileAnalyzer& analyzer);
    void scanElementValue(BytesDecoder& decoder, ClassFileAnalyzer& analyzer);

private:
    uint16_t mConstantPoolCount;
    cp_info** mConstantPool;
    attribute_info* mAttributes;
};

#define LONG_TAG ((cp_info *) -1)


FILE *fopenPath(char *path);
bool mkdirPath(char *path);
cp_info **readConstantPool(FileReader& reader, const char *filename, int count);
cp_info *readConstantPoolInfo(FileReader& reader, const char *filename);
attribute_info *readFields(FileReader& reader, int count, attribute_info *atts);
attribute_info *readMethods(FileReader& reader, int count, attribute_info *atts);
void skipWordArray(FileReader& reader, int length);

class ClassFileAnalyzer
{
public:
    ClassFileAnalyzer() {}

    void analyzeClassFile(char *name);

    void includePackage(const string& name) {mIncludedPackages.insert(PackageToPath(name)); }

    void excludePackage(const string& name) {mExcludedPackages.insert(PackageToPath(name)); }

    bool addDep(const char *name);
        // Adds name to the set of known dependencies.
        // Returns true if this is a new dependency.

    bool isIncludedClass(const string& name) const;

    void findDeps(const char *name);

private:
    typedef set<string> StringSet;

    static string PackageToPath(const string& name);

    static bool matchPackage(const string& name, const StringSet& packages);

private:
    StringSet mDeps;
    StringSet mExcludedPackages;
    StringSet mIncludedPackages;
};

bool ClassFileAnalyzer::addDep(const char *name)
{
    int before = mDeps.size();
    mDeps.insert(name);
    return before != mDeps.size();
}

void ClassFileAnalyzer::analyzeClassFile(char *name)
{
    FILE *outfyle;
    char outfilename[1000];
    int i;

    char *match = strstr(name, ".class");
    if (match && strlen(match) == 6 /* strlen(".class") */) {
        /* Chop off the trailing ".class" if it's there */
        *match = '\0';
    }

    if (ClassRoot[0]) {
        /* Strip leading class root path */
        if (strncmp(name, ClassRoot, strlen(ClassRoot))) {
            fprintf(stderr, "%s.class does not match class root path %s\n",
                    name, ClassRoot);
            exit(1);
        }
        name += strlen(ClassRoot);
    }

    findDeps(name);

    snprintf(outfilename, sizeof(outfilename), "%s%s.d", DepRoot, name);
    outfyle = fopenPath(outfilename);
    if (outfyle) {
        fprintf(outfyle, "%s%s.class: \\\n", ClassRoot, name);
        for (StringSet::iterator it=mDeps.begin(); it!=mDeps.end(); ++it)
        {
            const char* dep = it->c_str();
            if (index(dep, '$') == NULL) {
                fprintf(outfyle, "  %s%s.java\\\n", JavaRoot, dep);
            }
        }
        fprintf(outfyle, "\n");
        fclose(outfyle);
    } else {
        fprintf(stderr, "unable to open output file %s", outfilename);
    }
}

attribute_info * build_attribute_info(uint16_t attribute_name_index, long attribute_length,
                     uint8_t *info, attribute_info *next)
{
    attribute_info *result = TYPE_ALLOC(attribute_info);
    result->next = next;
    result->attribute_name_index = attribute_name_index;
    result->attribute_length = attribute_length;
    result->info = info;
    return result;
}

constant_class_info * build_constant_class_info(uint16_t name_index)
{
    constant_class_info *result = TYPE_ALLOC(constant_class_info);
    result->tag = CONSTANT_Class;
    result->name_index = name_index;
    return result;
}

constant_utf8_info * build_constant_utf8_info(char *str)
{
    constant_utf8_info *result = TYPE_ALLOC(constant_utf8_info);
    result->tag = CONSTANT_Utf8;
    result->str = str;
    return result;
}

string ClassFileAnalyzer::PackageToPath(const string& name)
{
    string pathName(name);
    std::replace(pathName.begin(), pathName.end(), '.', '/');
    if (pathName[pathName.size()-1] != '/')
        pathName += '/';
    return pathName;
}

void ClassFileAnalyzer::findDeps(const char *name)
{
    FILE *infyle;
    char infilename[1000];

    snprintf(infilename, sizeof(infilename), "%s%s.class", ClassRoot, name);
    ClassFile classFile(infilename);
    classFile.findDepsInFile(name, *this);

}

bool ClassFileAnalyzer::matchPackage(const string& name, const StringSet& packages)
{
    return packages.find(name) != packages.end();
}

bool ClassFileAnalyzer::isIncludedClass(const string& name) const
{
    if (matchPackage(name, mExcludedPackages))
        return false;
    if (mIncludedPackages.size() > 0)
        return matchPackage(name, mIncludedPackages);
    return true;
}

void ClassFile::scanAnnotation(BytesDecoder& decoder, ClassFileAnalyzer& analyzer)
{
    int i;

    int type_index = decoder.DecodeWord();

    const char *name = getClassName(type_index);
    if (analyzer.isIncludedClass(name)) {
        analyzer.addDep(name);
    }
    int num_element_value_pairs = decoder.DecodeWord();
    for (i = 0; i < num_element_value_pairs; ++i) {
        int element_name_index = decoder.DecodeWord();
        scanElementValue(decoder, analyzer);
    }
}

void ClassFile::scanElementValue(BytesDecoder& decoder, ClassFileAnalyzer& analyzer)
{
    uint8_t tag = decoder.DecodeByte();
    switch (tag) {
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z': {
            int const_value_index = decoder.DecodeWord();
            break;
        }
        case 's': {
            int const_value_index = decoder.DecodeWord();
            break;
        }
        case 'c': {
            int class_info_index = decoder.DecodeWord();
            break;
        }
        case 'e': {
            int type_name_index = decoder.DecodeWord();
            const char *name = getClassName(type_name_index);
            int const_name_index = decoder.DecodeWord();
            if (analyzer.isIncludedClass(name)) {
                analyzer.addDep(name);
            }
            break;
        }
        case '@': {
            scanAnnotation(decoder, analyzer);
            break;
        }
        case '[': {
            int num_values = decoder.DecodeWord();
            int i;
            for (i = 0; i < num_values; ++i) {
                scanElementValue(decoder, analyzer);
            }
            break;
        }
        default:
            break;
    }
}

void ClassFile::findDepsInFile(const char* target, ClassFileAnalyzer& analyzer)
{
    for (int i=0; i < mConstantPoolCount; ++i) {
        cp_info* cp = mConstantPool[i];
        if (cp && cp->tag == CONSTANT_Class) {
            const char* name = getClassName(i);
            if (analyzer.isIncludedClass(name)) {
                if (name[0] != '[') { /* Skip array classes */
                    char *dollar = index(name, '$');
                    if (dollar) {
                        /* It's an inner class */
                        if (strncmp(name, target, dollar-name) == 0) {
                                /* It's one of target's inner classes, so we
                                   depend on whatever *it* depends on and thus
                                   we need to recurse. */
                            bool added = analyzer.addDep(name);
                            if (added) {
                                analyzer.findDeps(name);
                            }
                        } else {
                                /* It's somebody else's inner class, so we
                                   depend on its outer class source file */
                            *dollar = '\0';
                            analyzer.addDep(name);
                            *dollar = '$';
                        }
                    } else {
                        /* It's a regular class */
                        analyzer.addDep(name);
                    }
                }
            }
        }
    }

    attribute_info *att = mAttributes;
    while (att != NULL) {
        const char *name = getString(att->attribute_name_index);
        if (strcmp(name, "RuntimeVisibleAnnotations") == 0) {
            int i;
            BytesDecoder decoder(att->info, att->attribute_length);
            int num_annotations = decoder.DecodeWord();
            for (i = 0; i < num_annotations; ++i) {
                scanAnnotation(decoder, analyzer);
            }
        }
        att = att->next;
    }
}

FILE * fopenPath(char *path)
{
    char *end = rindex(path, '/');
    if (end) {
        *end = '\0';
        mkdirPath(path);
        *end = '/';
    }
    return fopen(path, "w");
}

bool mkdirPath(char *path)
{
    char *slashptr = path;
    DIR *dyr;

    while ((slashptr = strchr(slashptr + 1, '/'))) {
        *slashptr = '\0';
        dyr = opendir(path);
        if (dyr) {
            closedir(dyr);
        } else if (mkdir(path, S_IRWXU) < 0) {
            *slashptr = '/';
            return true;
        }
        *slashptr = '/';
        if (slashptr[1] == '\0') {
            /* Just ignore a trailing slash */
            return false;
        }
    }
    if (mkdir(path, S_IRWXU) < 0) {
        return true;
    } else {
        return false;
    }
}

attribute_info * readAttributeInfo(FileReader& reader, attribute_info *atts)
{
    uint16_t attribute_name_index = reader.ReadWord();
    long attribute_length = reader.ReadLong();
    uint8_t *info = reader.ReadByteArray(attribute_length);

    return build_attribute_info(attribute_name_index, attribute_length, info,
                                atts);
}

attribute_info * readAttributes(FileReader& reader, int count, attribute_info *atts)
{
    int i;
    for (i = 0; i < count; ++i) {
        atts = readAttributeInfo(reader, atts);
    }
    return atts;
}

ClassFile::ClassFile(const char* infilename)
: mConstantPoolCount(0)
, mConstantPool(0)
, mAttributes(0)
{
    FileReader reader(infilename);

    reader.ReadLong(); /* magic */
    reader.ReadWord(); /* minor_version */
    reader.ReadWord(); /* major_version */
    mConstantPoolCount = reader.ReadWord();

    // TODO: There may be a bug here with infilename.
    // In my refactoring I think I collapsed two different versions of the filename
    // down to one version, but they needed to stay separate.
    mConstantPool = readConstantPool(reader, infilename, mConstantPoolCount);
    reader.ReadWord(); /* access_flags */
    reader.ReadWord(); /* this_class */
    reader.ReadWord(); /* super_class */
    uint16_t interfaces_count = reader.ReadWord();
    skipWordArray(reader, interfaces_count); /* interfaces */
    uint16_t fields_count = reader.ReadWord();
    mAttributes = readFields(reader, fields_count, mAttributes); /* fields */
    uint16_t methods_count = reader.ReadWord();
    mAttributes = readMethods(reader, methods_count, mAttributes); /* methods */
    uint16_t attributes_count = reader.ReadWord();
    mAttributes = readAttributes(reader, attributes_count, mAttributes);
}

cp_info** readConstantPool(FileReader& reader, const char *filename, int count)
{
    cp_info **result = TYPE_ALLOC_MULTI(cp_info *, count);
    int i;
    result[0] = NULL;
    for (i=1; i<count; ++i) {
        result[i] = readConstantPoolInfo(reader, filename);
        if (result[i] == LONG_TAG) {
            result[i] = NULL;
            result[++i] = NULL;
        }
    }
    return result;
}

cp_info * readConstantPoolInfo(FileReader& reader, const char *filename)
{
    uint8_t tag = reader.ReadByte();
    switch (tag) {
        case CONSTANT_Class:{
            uint16_t name_index = reader.ReadWord();
            return (cp_info *) build_constant_class_info(name_index);
        }
        case CONSTANT_Fieldref:{
            reader.ReadWord(); /* class_index */
            reader.ReadWord(); /* name_and_type_index */
            return NULL;
        }
        case CONSTANT_Methodref:{
            reader.ReadWord(); /* class_index */
            reader.ReadWord(); /* name_and_type_index */
            return NULL;
        }
        case CONSTANT_InterfaceMethodref:{
            reader.ReadWord(); /* class_index */
            reader.ReadWord(); /* name_and_type_index */
            return NULL;
        }
        case CONSTANT_String:{
            reader.ReadWord(); /* string_index */
            return NULL;
        }
        case CONSTANT_Integer:{
            reader.ReadLong(); /* bytes */
            return NULL;
        }
        case CONSTANT_Float:{
            reader.ReadLong(); /* bytes */
            return NULL;
        }
        case CONSTANT_Long:{
            reader.ReadLong(); /* high_bytes */
            reader.ReadLong(); /* low_bytes */
            return LONG_TAG;
        }
        case CONSTANT_Double:{
            reader.ReadLong(); /* high_bytes */
            reader.ReadLong(); /* low_bytes */
            return LONG_TAG;
        }
        case CONSTANT_NameAndType:{
            reader.ReadWord(); /* name_index */
            reader.ReadWord(); /* descriptor_index */
            return NULL;
        }
        case CONSTANT_Utf8:{
            uint16_t length = reader.ReadWord();
            char *str = (char *) reader.ReadByteArray(length);
            return (cp_info *) build_constant_utf8_info(str);
        }
        default:
            fprintf(stderr, "invalid constant pool tag %d in %s\n", tag,
                    filename);
            exit(1);
    }
    return NULL;
}

char * savePath(char *path)
{
    int len = strlen(path);
    if (path[len-1] == '/') {
        return strdup(path);
    } else {
        char *result = TYPE_ALLOC_MULTI(char, len+2);
        strncpy(result, path, len+1);
        result[len] = '/';
        result[len+1] = '\0';
        return result;
    }
}

attribute_info * readFieldInfo(FileReader& reader, attribute_info *atts)
{
    reader.ReadWord(); /* access_flags */
    reader.ReadWord(); /* name_index */
    reader.ReadWord(); /* descriptor_index */
    uint16_t attributes_count = reader.ReadWord();
    return readAttributes(reader, attributes_count, atts);
}

attribute_info * readFields(FileReader& reader, int count, attribute_info *atts)
{
    int i;
    for (i=0; i<count; ++i) {
        atts = readFieldInfo(reader, atts);
    }
    return atts;
}

attribute_info * readMethodInfo(FileReader& reader, attribute_info *atts)
{
    reader.ReadWord(); /* access_flags */
    reader.ReadWord(); /* name_index */
    reader.ReadWord(); /* descriptor_index */
    uint16_t attributes_count = reader.ReadWord();
    return readAttributes(reader, attributes_count, atts);
}

attribute_info * readMethods(FileReader& reader, int count, attribute_info *atts)
{
    int i;
    for (i = 0; i < count; ++i) {
        atts = readMethodInfo(reader, atts);
    }
    return atts;
}

void skipWordArray(FileReader& reader, int length)
{
    int i;
    for (i = 0; i < length; ++i) {
        reader.ReadWord();
    }
}

int main(int argc, char *argv[])
{
    int i;
    char *p;
    bool excludeLibraryPackages = true;

    ClassFileAnalyzer analyzer;

    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'a':
                    excludeLibraryPackages = false;
                    break;
                case 'c':
                    if (argv[i][2]) {
                        p = &argv[i][2];
                    } else {
                        ++i;
                        p = argv[i];
                    }
                    ClassRoot = savePath(p);
                    break;
                case 'd':
                    if (argv[i][2]) {
                        p = &argv[i][2];
                    } else {
                        ++i;
                        p = argv[i];
                    }
                    DepRoot = savePath(p);
                    break;
                case 'e':
                    if (argv[i][2]) {
                        p = &argv[i][2];
                    } else {
                        ++i;
                        p = argv[i];
                    }
                    analyzer.excludePackage(p);
                    break;
                case 'i':
                    if (argv[i][2]) {
                        p = &argv[i][2];
                    } else {
                        ++i;
                        p = argv[i];
                    }
                    analyzer.includePackage(p);
                    break;
                case 'j':
                    if (argv[i][2]) {
                        p = &argv[i][2];
                    } else {
                        ++i;
                        p = argv[i];
                    }
                    JavaRoot = savePath(p);
                    break;
                case 'h':
                    printf("%s", USAGE);
                    printf("options:\n");
                    printf("-a          Include java.* packages in dependencies\n");
                    printf("-e PACKAGE  Exclude PACKAGE from dependencies\n");
                    printf("-i PACKAGE  Include PACKAGE in dependencies\n");
                    printf("-h          Print this helpful help message\n");
                    printf("-d DPATH    Use DPATH as base directory for output .d files\n");
                    printf("-c CPATH    Use CPATH as base directory for .class files\n");
                    printf("-j JPATH    Use JPATH as base directory for .java files in dependency lines\n");
                    printf("file        Name of a class file to examine\n");
                    exit(0);
                default:
                    fprintf(stderr, "%s", USAGE);
                    exit(1);
            }
        } else {
            if (excludeLibraryPackages) {
                analyzer.excludePackage("java");
                analyzer.excludePackage("javax");
                analyzer.excludePackage("com.sun");
                excludeLibraryPackages = false;
            }
            analyzer.analyzeClassFile(argv[i]);
        }
    }
    exit(0);
}
