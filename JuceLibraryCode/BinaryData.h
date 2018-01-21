/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   ledcirclegreymd_png;
    const int            ledcirclegreymd_pngSize = 15237;

    extern const char*   ledcircleredmd_png;
    const int            ledcircleredmd_pngSize = 29113;

    extern const char*   logo_transparent_png;
    const int            logo_transparent_pngSize = 129116;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 3;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}
