#pragma once
#include <raylib.h>

void text_init();
void text_shutdown(void);

void text_prepare(const char *utf8);
void text_collect(const char *utf8);
void text_flush(void);

void  text_draw(const char *utf8, float x, float y, float size, Color color);
float text_width(const char *utf8, float size);
