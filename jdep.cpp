/*
  jdep.cpp -- Java .class file dependency analyzer

  Derived from work by:
  Copyright 2009 Chip Morningstar

  Ported to C++ and extensively revised by Jim Lloyd


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
#include <unistd.h>

#include "ClassFileAnalyzer.h"

void Usage()
{
    const char* usage =
        "usage: jdep -a [-e PACKAGE] [-i PACKAGE] -h [-c CPATH] [-d DPATH] [-j JPATH] files...\n";
    printf("%s", usage);
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
}

void ParseArgs(int& argc, char**& argv, ClassFileAnalyzer& analyzer)
{
    bool excludeLibraryPackages = true;
    while (true)
    {
        int c = getopt(argc, argv, "ae:i:c:d:j:f:m");
        if (c == -1)
            break;

        switch (c)
        {
            case 'a':
            {
                excludeLibraryPackages = false;
                break;
            }
            case 'e':
            {
                analyzer.excludePackage(optarg);
                break;
            }
            case 'i':
            {
                analyzer.includePackage(optarg);
                break;
            }
            case 'c':
            {
                analyzer.SetClassRoot(optarg);
                break;
            }
            case 'd':
            {
                analyzer.SetDepRoot(optarg);
                break;
            }
            case 'j':
            {
                analyzer.SetJavaRoot(optarg);
                break;
            }
            case 'f':
            {
                analyzer.SetFormat(optarg);
                break;
            }
            case 'm':
            {
                analyzer.MergeOutput();
                break;
            }
            default:
            {
                Usage();
                break;
            }
        }
    }

    if (excludeLibraryPackages)
    {
        analyzer.excludePackage("java");
        analyzer.excludePackage("javax");
        analyzer.excludePackage("com.sun");
    }

    argc -= optind;
    argv += optind;
}

int main(int argc, char* argv[])
{
    ClassFileAnalyzer analyzer;

    ParseArgs(argc, argv, analyzer);

    for (int i = 0; i < argc; ++i)
    {
        analyzer.analyzeClassFile(argv[i]);
        analyzer.WriteOutput();
    }

    exit(0);
}
