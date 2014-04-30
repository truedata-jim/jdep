// ClassFileAnalyzer.h

#pragma once

#include <set>
#include <string>

using std::set;
using std::string;

class ClassFileAnalyzer
{
public:
    ClassFileAnalyzer();

    void analyzeClassFile(const string& fullClassPath);

    void WriteOutput() const;

    void includePackage(const string& name)
    {
        mIncludedPackages.insert(PackageToPath(name));
    }

    void excludePackage(const string& name)
    {
        mExcludedPackages.insert(PackageToPath(name));
    }

    bool addDep(const char* name);
    // Adds name to the set of known dependencies.
    // Returns true if this is a new dependency.

    bool isIncludedClass(const string& name) const;

    void findDeps(const string& packageAndName);

    void SetJavaRoot(const string& root)
    {
        mJavaRoot = SavePath(root);
    }
    void SetClassRoot(const string& root)
    {
        mClassRoot = SavePath(root);
    }
    void SetDepRoot(const string& root)
    {
        mDepRoot = SavePath(root);
    }

    void SetFormat(const string& format);

    void MergeOutput() { mMergeOutput = true; }

private:

    void WriteDependencyFile(FILE* outFile) const;
    void WriteTabularOutput(FILE* outFile) const;

    string FullClassPathToPackageAndName(const string& fullClassPath) const;


private:
    typedef set<string> StringSet;

    static string PackageToPath(const string& name);

    static bool matchPackage(const string& name, const StringSet& packages);

    string SavePath(const string& _path)
    {
        string path(_path);
        int len = path.size();
        if (path[len-1] != '/')
            path += '/';
        return path;
    }

private:
    StringSet mExcludedPackages;
    StringSet mIncludedPackages;

    string mJavaRoot;
    string mClassRoot;
    string mDepRoot;

    string mFormat;
    bool   mMergeOutput;

    string    mPackageAndName;
    StringSet mDeps;
};

