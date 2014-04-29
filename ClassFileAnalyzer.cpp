// ClassFileAnalyzer.cpp

#include "ClassFileAnalyzer.h"
#include "ClassFile.h"

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

bool ClassFileAnalyzer::addDep(const char* name)
{
    int before = mDeps.size();
    mDeps.insert(name);
    return before != mDeps.size();
}

void ClassFileAnalyzer::analyzeClassFile(char* name)
{
    FILE* outfyle;
    char outfilename[1000];

    char* match = strstr(name, ".class");
    if (match && strlen(match) == 6 /* strlen(".class") */)
    {
        /* Chop off the trailing ".class" if it's there */
        *match = '\0';
    }

    if (mClassRoot != "")
    {
        const char* root = mClassRoot.c_str();
        if (strncmp(name, root, mClassRoot.length()))
        {
            fprintf(stderr, "%s.class does not match class root path %s\n",
                    name, root);
            exit(1);
        }
        name += mClassRoot.length();
    }

    findDeps(name);

    snprintf(outfilename, sizeof(outfilename), "%s%s.d", mDepRoot.c_str(), name);
    outfyle = fopenPath(outfilename);
    if (outfyle)
    {
        fprintf(outfyle, "%s%s.class: \\\n", mClassRoot.c_str(), name);
        for (StringSet::iterator it=mDeps.begin(); it!=mDeps.end(); ++it)
        {
            const char* dep = it->c_str();
            if (index(dep, '$') == NULL)
                fprintf(outfyle, "  %s%s.java \\\n", mJavaRoot.c_str(), dep);
        }
        fprintf(outfyle, "\n");
        fclose(outfyle);
    }
    else
        fprintf(stderr, "unable to open output file %s", outfilename);
}

string ClassFileAnalyzer::PackageToPath(const string& name)
{
    string pathName(name);
    std::replace(pathName.begin(), pathName.end(), '.', '/');
    if (pathName[pathName.size()-1] != '/')
        pathName += '/';
    return pathName;
}

void ClassFileAnalyzer::findDeps(const char* name)
{
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

