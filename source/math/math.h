#pragma once

static inline int math_min(int a, int b)
{
    return a < b ? a : b;
}

static inline int math_max(int a, int b)
{
    return a > b ? a : b;
}

static inline int math_clamp(int a, int min, int max)
{
    a = math_max(a,min);
    a = math_min(a,max);
    return a;
}

static inline int math_absInline(int a)
{
    return a > 0 ? a : -a;
}