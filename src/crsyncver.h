/*
The MIT License (MIT)

Copyright (c) 2015 chenqi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __CRSYNCVER_H
#define __CRSYNCVER_H

/* This is the global package copyright */
#define CRSYNC_COPYRIGHT "2015 chenqi, <9468305@qq.com>."

/* This is the version number of the crsync package from which this header file origins: */
#define CRSYNC_VERSION "0.1.0"

/* The numeric version number is also available "in parts" by using these defines: */
#define CRSYNC_VERSION_MAJOR 0
#define CRSYNC_VERSION_MINOR 1
#define CRSYNC_VERSION_PATCH 0

/* This is the numeric version of the crsync version number, meant for easier
   parsing and comparions by programs. The CRSYNC_VERSION_NUM define will
   always follow this syntax:

         0xXXYYZZ

   Where XX, YY and ZZ are the main version, release and patch numbers in
   hexadecimal (using 8 bits each). All three numbers are always represented
   using two digits.  1.2.3 would appear as "0x010203".

   This 6-digit (24 bits) hexadecimal number does not show pre-release number,
   and it is always a greater number in a more recent release. It makes
   comparisons with greater than and less than work.
*/
#define CRSYNC_VERSION_NUM 0x000100

#endif //__CRSYNCVER_H
