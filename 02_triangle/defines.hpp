#pragma once

#include <stdio.h>
#include <assert.h>

#define LOG(fmt, ...)                    \
    fprintf(stdout, fmt, ##__VA_ARGS__); \
    fflush(stdout);

#define ASSERT(val, fmt, ...)                    \
    do                                           \
    {                                            \
        if (!(val))                                \
        {                                        \
            fprintf(stdout, fmt, ##__VA_ARGS__); \
            fflush(stdout);                      \
            assert(false);                       \
        }                                        \
    } while(0)                                   \

#define EXIT(fmt, ...)                       \
    do                                       \
    {                                        \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fflush(stderr);                      \
        assert(false);                       \
    } while (0)
