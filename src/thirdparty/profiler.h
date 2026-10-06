#pragma once


#ifdef WITH_PROFILER
#   include <prof.h>
#   define PROFILER_REG(X)   Prof_Region(X)
#   define PROFILER_BEGIN(X) Prof_Begin(X)
#   define PROFILER_END      Prof_End
#   define PROFILER_DEF(X)   Prof_Define(X)
#else
#   define PROFILER_BEGIN(X)
#   define PROFILER_END
#   define PROFILER_DEF(X)
#endif

void profiler_init(void);
void profiler_draw(int w, int h);
void profiler_update();
