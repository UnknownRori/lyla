// Thank you tsoding:3

#include <rstb_common.h>
#include "assets/circle_frag.c"
#include "visualizer.h"
#include "utils.h"

#include <math.h>
#include <rlgl.h>

static Shader circle;
static int radius_loc = -1;
static int power_loc = -1;
static bool ready = false;

static f32 detach = 0.0f;

static void update_detach(bool detached, f32 dt)
{
    f32 target = detached ? 1.0f : 0.0f;
    f32 k = 1.0f - expf(-5.0f*dt);
    detach += (target - detach)*k;
    if (fabsf(target - detach) < 0.001f) detach = target;
}

bool visualizer_init(void)
{
    circle = LoadShaderFromMemory(NULL, (char*) __resources_circle330_frag_glsl);
    ready = IsShaderValid(circle);
    if (ready) {
        radius_loc = GetShaderLocation(circle, "radius");
        power_loc  = GetShaderLocation(circle, "power");
    }
    return ready;
}

void visualizer_shutdown(void)
{
    if (ready) UnloadShader(circle);
    ready = false;
}

#define MAX_PARTICLES 4096

typedef struct { Vector2 pos, vel; } Particle;

static Particle particles[MAX_PARTICLES];
static Vector2  homes[MAX_PARTICLES];
static f32      acc_x[MAX_PARTICLES], acc_y[MAX_PARTICLES];
static bool     free_flight = false;
static bool     prev_detached = false;

static inline f32 hash01(usize i, usize k) { return sinf(i*k)*0.5f + 0.5f; }

static void launch_particles(Rectangle b, usize m)
{
    for (usize i = 0; i < m; ++i) {
        particles[i].pos = homes[i];
        particles[i].vel.x = (hash01(i, 78.233f) - 0.5f)*b.width*0.25f;         // sideways scatter
        particles[i].vel.y = -b.height*(0.5f + 0.8f*hash01(i, 12.9898f));       // burst upward
    }
    free_flight = true;
}

static void step_particles(Rectangle b, usize m, bool detached, f32 dt)
{
    f32 range   = sqrtf(b.width*b.height/m);
    f32 repel   = range*20.0f*detach;
    f32 margin  = range*0.5f;
    f32 drag    = detached ? 3.5f : 12.0f;
    f32 home_k  = detached ? 0.0f : 60.0f;
    f32 time    = (f32)GetTime();

    for (usize i = 0; i < m; ++i) { acc_x[i] = 0; acc_y[i] = 0; }

    if (repel > 0.0f) {
        for (usize i = 0; i < m; ++i) {
            for (usize j = i + 1; j < m; ++j) {
                f32 dx = particles[j].pos.x - particles[i].pos.x;
                f32 dy = particles[j].pos.y - particles[i].pos.y;
                f32 d2 = dx*dx + dy*dy;
                if (d2 >= range*range) continue;
                f32 d = sqrtf(d2);
                if (d < 1e-3f) { dx = hash01(i, 3.7f) - 0.5f; dy = hash01(j, 5.1f) - 0.5f; d = 1.0f; }
                f32 f = repel*(1.0f - d/range);
                f32 nx = dx/d, ny = dy/d;
                acc_x[i] -= nx*f; acc_y[i] -= ny*f;
                acc_x[j] += nx*f; acc_y[j] += ny*f;
            }
        }
    }

    f32 damp = expf(-drag*dt);
    for (usize i = 0; i < m; ++i) {
        Particle *p = &particles[i];

        acc_x[i] += sinf(time*0.9f + i*1.3f)*40.0f*detach;
        acc_y[i] += cosf(time*1.1f + i*0.7f)*40.0f*detach;

        acc_x[i] += (homes[i].x - p->pos.x)*home_k;
        acc_y[i] += (homes[i].y - p->pos.y)*home_k;

        p->vel.x = (p->vel.x + acc_x[i]*dt)*damp;
        p->vel.y = (p->vel.y + acc_y[i]*dt)*damp;
        p->pos.x += p->vel.x*dt;
        p->pos.y += p->vel.y*dt;

        if (detached) {
            f32 lo_x = b.x + margin, hi_x = b.x + b.width  - margin;
            f32 lo_y = b.y + margin, hi_y = b.y + b.height - margin;
            if (p->pos.x < lo_x) { p->pos.x = lo_x; if (p->vel.x < 0) p->vel.x = 0; }
            if (p->pos.x > hi_x) { p->pos.x = hi_x; if (p->vel.x > 0) p->vel.x = 0; }
            if (p->pos.y < lo_y) { p->pos.y = lo_y; if (p->vel.y < 0) p->vel.y = 0; }
            if (p->pos.y > hi_y) { p->pos.y = hi_y; if (p->vel.y > 0) p->vel.y = 0; }
        }
    }
}

void visualizer_render(Rectangle boundary, const f32* smooth, const f32* smear,
                       usize m, bool detached, f32 beat, f32 dt)
{
    RORI_ASSERT(smooth != NULL && "dummy dumb dumb");
    RORI_ASSERT(smear != NULL && "dummy dumb dumb");

    update_detach(detached, dt);
    if (m > MAX_PARTICLES) m = MAX_PARTICLES;
    if (m == 0) return;

    f32 cell_width = boundary.width/m;
    f32 saturation = 0.75f, value = 1.0f;

    // Bars and smears shrink to the baseline as the circles detach.
    f32 bar_scale = 1.0f - detach;

    // Bars
    for (usize i = 0; i < m; ++i) {
        f32 t = smooth[i]*bar_scale;
        Color color = ColorFromHSV((f32)i/m*360, saturation, value);
        Vector2 start = { boundary.x + i*cell_width + cell_width/2,
                          boundary.y + boundary.height - boundary.height*2/3*t };
        Vector2 end   = { start.x, boundary.y + boundary.height };
        DrawLineEx(start, end, cell_width/3*sqrtf(t)*(1.0f + 0.3f*beat), color);
    }

    if (!ready) return;

    Texture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };

    // Smears
    SetShaderValue(circle, radius_loc, (f32[1]){ 0.3f }, SHADER_UNIFORM_FLOAT);
    SetShaderValue(circle, power_loc,  (f32[1]){ 3.0f }, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(circle);
    for (usize i = 0; i < m; ++i) {
        f32 start = smear[i]*bar_scale, end = smooth[i]*bar_scale;
        Color color = ColorFromHSV((f32)i/m*360, saturation, value);
        Vector2 sp = { boundary.x + i*cell_width + cell_width/2,
                       boundary.y + boundary.height - boundary.height*2/3*start };
        Vector2 ep = { sp.x, boundary.y + boundary.height - boundary.height*2/3*end };
        f32 radius = cell_width*3*sqrtf(end);
        Vector2 origin = {0};
        if (ep.y >= sp.y) {
            Rectangle dest = RECT(sp.x - radius/2, sp.y, radius, ep.y - sp.y);
            Rectangle source = { 0, 0, 1, 0.5f };
            DrawTexturePro(texture, source, dest, origin, 0, color);
        } else {
            Rectangle dest = { ep.x - radius/2, ep.y, radius, sp.y - ep.y };
            Rectangle source = { 0, 0.5f, 1, 0.5f };
            DrawTexturePro(texture, source, dest, origin, 0, color);
        }
    }
    EndShaderMode();

    SetShaderValue(circle, radius_loc, (f32[1]){ 0.07f }, SHADER_UNIFORM_FLOAT);
    SetShaderValue(circle, power_loc,  (f32[1]){ 5.0f },  SHADER_UNIFORM_FLOAT);

    for (usize i = 0; i < m; ++i) {
        homes[i] = VEC2(
            boundary.x + i*cell_width + cell_width/2,
            boundary.y + boundary.height - boundary.height*2/3*smooth[i]
        );
    }

    if (detached && !prev_detached && !free_flight) launch_particles(boundary, m);
    prev_detached = detached;
    if (!detached && detach == 0.0f) free_flight = false;

    if (free_flight) {
        // TODO : step is delegate at the start
        // f32 step_dt = fminf(dt, 1.0f/20.0f)/2;
        // step_particles(boundary, m, detached, step_dt);
        // step_particles(boundary, m, detached, step_dt);
        step_particles(boundary, m, detached, dt);
        step_particles(boundary, m, detached, dt);
    }

    BeginShaderMode(circle);
    for (usize i = 0; i < m; ++i) {
        f32 t = smooth[i];
        Vector2 center = free_flight ? particles[i].pos : homes[i];
        f32 radius = cell_width*6*sqrtf(t)*(1.0f + 0.6f*detach)*(1.0f + 0.35f*beat);
        
        Color color = ColorFromHSV((f32)i/m*360, saturation, value);
        color = ColorBrightness(color, 0.25f*beat);
        
        Vector2 pos = { center.x - radius, center.y - radius };
        DrawTextureEx(texture, pos, 0, 2*radius, color);
    }
    EndShaderMode();
}

void visualizer_draw_corner_glow(f32 beat, int w, int h)
{
    if (beat < 0.01f) return;

    f32 m = fminf((f32)w, (f32)h);
    int t = (int)(m*(0.04f + 0.01f*beat));
    int lw = (int)(w*0.30f);
    int lh = (int)(h*0.30f);

    f32 hue = fmodf((f32)GetTime()*30.0f, 360.0f);
    Color c = ColorAlpha(ColorFromHSV(hue, 0.45f, 1.0f), 0.18f*beat);
    Color n = ColorAlpha(c, 0.0f);

    BeginBlendMode(BLEND_ADDITIVE);
    // horizontal
    DrawRectangleGradientEx((Rectangle){0,      0,     lw, t}, c, n, n, n);
    DrawRectangleGradientEx((Rectangle){w - lw, 0,     lw, t}, n, n, n, c);
    DrawRectangleGradientEx((Rectangle){0,      h - t, lw, t}, n, c, n, n);
    DrawRectangleGradientEx((Rectangle){w - lw, h - t, lw, t}, n, n, c, n);
    // vertical
    DrawRectangleGradientEx((Rectangle){0,      0,      t, lh}, c, n, n, n);
    DrawRectangleGradientEx((Rectangle){w - t,  0,      t, lh}, n, n, n, c);
    DrawRectangleGradientEx((Rectangle){0,      h - lh, t, lh}, n, c, n, n);
    DrawRectangleGradientEx((Rectangle){w - t,  h - lh, t, lh}, n, n, c, n);
    EndBlendMode();
}

