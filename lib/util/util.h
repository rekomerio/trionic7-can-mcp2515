#pragma once

namespace util
{
    template <typename T>
    T minVal(T a, T b)
    {
        return a < b ? a : b;
    }
    
    template <typename T>
    T maxVal(T a, T b)
    {
        return a > b ? a : b;
    }
}