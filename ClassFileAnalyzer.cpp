// ClassFileAnalyzer.cpp

#include "ClassFileAnalyzer.h"
#include "ClassFile.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

FILE* fopenPath(char* path);
bool mkdirPath(char* path);

FILE* fopenPath(char* path)
{
    char* end = rindex(path, '/');
    if (end)
    {
        *end = '\0';
        mkdirPath(path);
        *end = '/';
    }
    return fopen(path, "w");
}

bool mkdirPath(char* path)
{
    char* slashptr = path;
    DIR* dyr;

    while ((slashptr = strchr(slashptr + 1, '/')))
    {
        *slashptr = '\0';
        dyr = opendir(path);
        if (dyr)
            closedir(dyr);
        else if (mkdir(path, S_IRWXU) < 0)
        {
            *slashptr = '/';
            return true;
        }
        *slashptr = '/';
        if (slashptr[1] == '\0')
        {
            /* Just ignore a trailing slash */
            return false;
        }
    }
    if (mkdir(path, S_IRWXU) < 0)
        return true;
    else
        return false;
}

const string gDepFormat("d");
const string gTabFormat("tab");

ClassFileAnalyzer::ClassFileAnalyzer()
{
    mFormat.assign(gDepFormat);
}

void ClassFileAnalyzer::SetFormat(const string& format)
{
    if (format == gDepFormat)
        mFormat.assign(format);
    else if (format == gTabFormat)
        mFormat.assign(format);
    else
    {
        fprintf(stderr, "Format %s unrecognized.\n", format.c_str());
        exit(1);
    }
}

void ClassFileAnalyzer::WriteOutput() const
{
    const char* name = mPackageAndName.c_str();
    char outfilename[1000];
    snprintf(outfilename, sizeof(outfilename), "%s%s.%s", mDepRoot.c_str(), name, mFormat.c_str());
    FILE* outFile = fopenPath(outfilename);
    if (!outFile)
    {
        fprintf(stderr, "unable to open output file %s", outfilename);
        exit(1);
    }

    if (mFormat == gDepFormat)
        WriteDependencyFile(outFile);
    else if (mFormat == gTabFormat)
        WriteTabularOutput(outFile);

    fclose(outFile);
}

void ClassFileAnalyzer::WriteDependencyFile(FILE* outFile) const
{
    const char* name = mPackageAndName.c_str();
    fprintf(outFile, "%s%s.class: \\\n", mClassRoot.c_str(), name);
    for (StringSet::iterator it=mDeps.begin(); it!=mDeps.end(); ++it)
    {
        const char* dep = it->c_str();
        if (index(dep, '$') == NULL)
            fprintf(outFile, "  %s%s.java \\\n", mJavaRoot.c_str(), dep);
    }
    fprintf(outFile, "\n");
}

void ClassFileAnalyzer::WriteTabularOutput(FILE* outFile) const
{
    const char* name = mPackageAndName.c_str();
    for (StringSet::iterator it=mDeps.begin(); it!=mDeps.end(); ++it)
    {
        const char* dep = it->c_str();
        if (index(dep, '$') == NULL)
            fprintf(outFile, "%s\t%s\n", name, dep);
    }
}

bool ClassFileAnalyzer::addDep(const char* name)
{
    int before = mDeps.size();
    mDeps.insert(name);
    return before != mDeps.size();
}


string ClassFileAnalyzer::FullClassPathToPackageAndName(const string& fullClassPath) const
{
    // Initialize our result to the fullClassPath
    string packageAndName(fullClassPath);

    // We must have a class file, i.e. a .class suffix
    const string suffix(".class");
    const size_t sufLen = suffix.length();
    size_t packNameLen = packageAndName.length();
    assert(packageAndName.substr(packNameLen-sufLen) == suffix);

    // Now lop off the suffix
    packageAndName.resize(packNameLen-sufLen);

    // The fullClassPath must begin with the class root path
    const size_t rootLen = mClassRoot.length();
    assert(packageAndName.substr(0, rootLen) == mClassRoot);

    // Now remove the root path
    packageAndName = packageAndName.substr(rootLen);

    // packageAndName is now {packagepath}/{classname}, e.g:
    // com/redsealsys/srm/server/analysis/compactTree/CompactTreeTrafficFlow
    return packageAndName;
}

void ClassFileAnalyzer::analyzeClassFile(const string& fullClassPath)
{
    mDeps.clear();
    mPackageAndName = FullClassPathToPackageAndName(fullClassPath);
    findDeps(mPackageAndName);
}

string ClassFileAnalyzer::PackageToPath(const string& name)
{
    string pathName(name);
    std::replace(pathName.begin(), pathName.end(), '.', '/');
    if (pathName[pathName.size()-1] != '/')
        pathName += '/';
    return pathName;
}

void ClassFileAnalyzer::findDeps(const string& packageAndName)
{
    const char* name = packageAndName.c_str();
    fprintf(stderr, "Analyzing %s\n", name);
    char infilename[1000];
    snprintf(infilename, sizeof(infilename), "%s%s.class", mClassRoot.c_str(), name);
    ClassFile classFile(infilename);
    classFile.findDepsInFile(name, *this);
}

bool ClassFileAnalyzer::matchPackage(const string& name, const StringSet& packages)
{
    for (StringSet::iterator it=packages.begin(); it!=packages.end(); ++it)
    {
        size_t len = it->size();
        if (name.substr(0, len) == *it)
            return true;
    }
    return false;
}

bool ClassFileAnalyzer::isIncludedClass(const string& name) const
{
    if (matchPackage(name, mExcludedPackages))
        return false;
    if (mIncludedPackages.size() > 0)
        return matchPackage(name, mIncludedPackages);
    return true;
}

