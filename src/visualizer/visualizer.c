// Thank you tsoding:3

#include <rstb_common.h>
#include "visualizer.h"
#include "utils.h"

#if defined(PLATFORM_WEB)
#include "assets/circle100_frag.c"
#else
#include "assets/circle_frag.c"
#endif

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
#if defined(PLATFORM_WEB)
    circle = LoadShaderFromMemory(NULL, (char*) __resources_circle100_frag_glsl);
#else
    circle = LoadShaderFromMemory(NULL, (char*) __resources_circle330_frag_glsl);
#endif
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

#define MAX_PARTICLES     4096

#define BURST_SPEED_MIN   0.20f
#define BURST_SPEED_VAR   0.40f
#define BURST_JITTER      1.0f 
#define FLASH_DECAY       4.0f
#define FLASH_SIZE        0.5f

#define FLOAT_DRAG        1.8f
#define FLOAT_SPRING      1.5f
#define FLOW_ACCEL        0.18f
#define BOUNDS_SPRING     40.0f
#define TWINKLE           0.15f
#define RADIAL_ORBIT      0.25f

#define TRAIL_TIME        0.12f
#define TRAIL_MAX         0.30f
#define TRAIL_FADE_SPEED  0.30f
#define TRAIL_WIDTH       0.45f
#define TRAIL_REATTACH    0.5f

#define FLASH_BRIGHT      0.5f

typedef struct {
    Vector2 pos, vel;
    Vector2 start, target;
    f32     delay, age, flash;
    bool    attached;
} Particle;

static Particle particles[MAX_PARTICLES];
static Vector2  homes[MAX_PARTICLES];
static bool     prev_detached = false;
static Vector2  burst_c = {0};
static f32      orbit_w = 0.0f;

static inline f32 hash01(usize i, usize k) { return sinf(i*k)*0.5f + 0.5f; }
static inline f32 saturate(f32 x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
static inline f32 smooth01(f32 x) { x = saturate(x); return x*x*(3.0f - 2.0f*x); }

static Vector2 burst_velocity(usize i, Vector2 origin, f32 ref)
{
    f32 dx = origin.x - burst_c.x, dy = origin.y - burst_c.y;
    f32 d  = sqrtf(dx*dx + dy*dy);
    if (d < 1e-3f) { dx = (hash01(i, 11.0f) - 0.5f); dy = -1.0f; d = sqrtf(dx*dx + dy*dy); }
    f32 nx = dx/d, ny = dy/d;

    f32 a  = (hash01(i, 78.233f) - 0.5f)*BURST_JITTER;
    f32 ca = cosf(a), sa = sinf(a);
    f32 rx = nx*ca - ny*sa, ry = nx*sa + ny*ca;

    f32 speed = ref*(BURST_SPEED_MIN + BURST_SPEED_VAR*hash01(i, 91.17f));
    return (Vector2){ rx*speed, ry*speed };
}

static void step_particles(Rectangle b, usize m, bool detached, f32 dt, f32 ref)
{
    f32 time   = (f32)GetTime();
    f32 margin = ref*0.04f;
    f32 lo_x = b.x + margin, hi_x = b.x + b.width  - margin;
    f32 lo_y = b.y + margin, hi_y = b.y + b.height - margin;
    f32 snap_dist_sq = (ref*0.015f)*(ref*0.015f);

    for (usize i = 0; i < m; ++i) {
        Particle *p = &particles[i];

        if (p->attached) {
            p->pos = homes[i];
            p->vel = (Vector2){0};
            p->flash = 0.0f;
            continue;
        }

        p->age += dt;

        if (!detached) {
            f32 dx = homes[i].x - p->pos.x;
            f32 dy = homes[i].y - p->pos.y;
            f32 dist_sq = dx*dx + dy*dy;

            if (dist_sq <= snap_dist_sq) {
                p->attached = true;
                p->pos = homes[i];
                p->vel = (Vector2){0};
                p->flash = 0.0f;
                continue;
            }

            f32 ax = dx*60.0f;
            f32 ay = dy*60.0f;
            f32 damp = expf(-12.0f*dt);
            p->vel.x = (p->vel.x + ax*dt)*damp;
            p->vel.y = (p->vel.y + ay*dt)*damp;
            p->pos.x += p->vel.x*dt;
            p->pos.y += p->vel.y*dt;
            continue;
        }

        if (orbit_w != 0.0f) {
            f32 dx = p->target.x - burst_c.x, dy = p->target.y - burst_c.y;
            f32 ca = cosf(orbit_w*dt), sa = sinf(orbit_w*dt);
            p->target.x = burst_c.x + dx*ca - dy*sa;
            p->target.y = burst_c.y + dx*sa + dy*ca;
        }

        f32 flow = ref*FLOW_ACCEL;
        f32 k    = 6.0f/ref;
        f32 ax = sinf(time*0.7f + p->pos.y*k + i*0.9f)*flow;
        f32 ay = cosf(time*0.8f + p->pos.x*k + i*0.5f)*flow;

        ax += (p->target.x - p->pos.x)*FLOAT_SPRING;
        ay += (p->target.y - p->pos.y)*FLOAT_SPRING;

        if (p->pos.x < lo_x) ax += (lo_x - p->pos.x)*BOUNDS_SPRING;
        if (p->pos.x > hi_x) ax += (hi_x - p->pos.x)*BOUNDS_SPRING;
        if (p->pos.y < lo_y) ay += (lo_y - p->pos.y)*BOUNDS_SPRING;
        if (p->pos.y > hi_y) ay += (hi_y - p->pos.y)*BOUNDS_SPRING;

        f32 damp = expf(-FLOAT_DRAG*dt);
        p->vel.x = (p->vel.x + ax*dt)*damp;
        p->vel.y = (p->vel.y + ay*dt)*damp;
        p->pos.x += p->vel.x*dt;
        p->pos.y += p->vel.y*dt;

        p->flash *= expf(-FLASH_DECAY*dt);
    }
}

static void simulate(Rectangle b, usize m, bool detached, f32 dt, f32 ref)
{
    dt = fminf(dt, 1.0f/60.0f)/2;
    step_particles(b, m, detached, dt, ref);
    step_particles(b, m, detached, dt, ref);
}

static inline f32 ball_scale(const Particle *p, usize i, f32 time)
{
    f32 tw = !p->attached ? 1.0f + TWINKLE*sinf(time*6.0f + i*2.1f)*detach : 1.0f;
    return (1.0f + FLASH_SIZE*p->flash)*tw;
}

static void draw_trail(Texture2D tex, const Particle *p, f32 ball_radius, f32 ref,
                       Color color, f32 strength)
{
    if (p->attached || ball_radius < 0.1f) return;
    f32 vx = p->vel.x, vy = p->vel.y;
    f32 sp = sqrtf(vx*vx + vy*vy);
    f32 fade = smooth01(sp/(ref*TRAIL_FADE_SPEED))*strength*strength;
    if (fade <= 0.0f) return;

    f32 len_k = TRAIL_REATTACH + (1.0f - TRAIL_REATTACH)*strength;
    f32 len   = fminf(sp*TRAIL_TIME, ref*TRAIL_MAX)*len_k;

    f32 width = ball_radius*TRAIL_WIDTH*(0.6f + 0.4f*strength);
    if (len < 0.5f || width < 0.5f) return;

    f32 angle = atan2f(-vx, vy)*RAD2DEG;
    Rectangle dest   = { p->pos.x, p->pos.y, width, len };
    Vector2   origin = { width*0.5f, len };
    Rectangle source = { 0, 0, 1, 0.5f };
    DrawTexturePro(tex, source, dest, origin, angle, ColorAlpha(color, fade));
}

static void launch_particles(Rectangle b, usize m, const f32* smooth)
{
    (void)smooth;
    f32 H    = b.height;
    f32 base = b.y + H;
    f32 cx   = b.x + b.width*0.5f;

    burst_c = (Vector2){ cx, base - H*0.30f };
    orbit_w = 0.0f;

    for (usize i = 0; i < m; ++i) {
        Particle *p = &particles[i];
        p->attached = false;
        p->pos = homes[i];
        p->start = p->pos;
        p->flash = 1.0f;

        p->vel = burst_velocity(i, p->pos, H);
        p->vel.y -= H * (0.50f + 0.10f * hash01(i, 51.7f));

        f32 hx = hash01(i, 23.1f), hy = hash01(i, 47.3f);
        f32 tx = p->start.x + (p->start.x - cx)*0.35f + (hx - 0.5f)*b.width*0.2f;
        p->target = (Vector2){ fminf(fmaxf(tx, b.x + b.width*0.04f), b.x + b.width*0.96f),
                               b.y + H*(0.15f + 0.5f*hy) };

        p->age = 0.0f;
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
    f32 time = (f32)GetTime();

    f32 bar_scale = 1.0f - detach;

    for (usize i = 0; i < m; ++i) {
        homes[i] = VEC2(
            boundary.x + i*cell_width + cell_width/2,
            boundary.y + boundary.height - boundary.height*2/3*smooth[i]
        );
    }

    if (detached && !prev_detached) launch_particles(boundary, m, smooth);
    prev_detached = detached;

    // Run physics step
    simulate(boundary, m, detached, dt, boundary.height);

    // Render Bars
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

    // Render Smears & Trails
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
    for (usize i = 0; i < m; ++i) {
        const Particle *p = &particles[i];
        f32 t = smooth[i];
        f32 radius = cell_width*6*sqrtf(t)*(1.0f + 0.6f*detach)*(1.0f + 0.35f*beat)*ball_scale(p, i, time);
        Color color = ColorFromHSV((f32)i/m*360, saturation, value);
        draw_trail(texture, p, radius, boundary.height, color, detach);
    }
    EndShaderMode();

    SetShaderValue(circle, radius_loc, (f32[1]){ 0.07f }, SHADER_UNIFORM_FLOAT);
    SetShaderValue(circle, power_loc,  (f32[1]){ 5.0f },  SHADER_UNIFORM_FLOAT);

    // Render Particle Circles
    BeginShaderMode(circle);
    for (usize i = 0; i < m; ++i) {
        const Particle *p = &particles[i];
        f32 t = smooth[i];
        if (t < 0.001f) continue;

        // Position is determined individually per particle
        Vector2 center = (!p->attached) ? p->pos : homes[i];
        f32 sc = (!p->attached) ? ball_scale(p, i, time) : 1.0f;
        f32 radius = cell_width*6*sqrtf(t)*(1.0f + 0.6f*detach)*(1.0f + 0.35f*beat)*sc;

        Color color = ColorFromHSV((f32)i/m*360, saturation, value);
        f32 flash = (!p->attached) ? p->flash : 0.0f;
        color = ColorBrightness(color, 0.25f*beat + FLASH_BRIGHT*flash);

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
    DrawRectangleGradientEx((Rectangle){0,      0,      lw, t}, c, n, n, n);
    DrawRectangleGradientEx((Rectangle){w - lw, 0,      lw, t}, n, n, n, c);
    DrawRectangleGradientEx((Rectangle){0,      h - t, lw, t}, n, c, n, n);
    DrawRectangleGradientEx((Rectangle){w - lw, h - t, lw, t}, n, n, c, n);

    DrawRectangleGradientEx((Rectangle){0,      0,      t, lh}, c, n, n, n);
    DrawRectangleGradientEx((Rectangle){w - t,  0,      t, lh}, n, n, n, c);
    DrawRectangleGradientEx((Rectangle){0,      h - lh, t, lh}, n, c, n, n);
    DrawRectangleGradientEx((Rectangle){w - t,  h - lh, t, lh}, n, n, c, n);
    EndBlendMode();
}

#define RADIAL_MIRROR 1
#define RADIAL_RIGHT_RESERVE 0.45f

static inline Vector2 polar(Vector2 c, f32 r, f32 a)
{
    return (Vector2){ c.x + cosf(a)*r, c.y + sinf(a)*r };
}

static inline f32 radial_angle(usize i, usize s, usize m, f32 sweep, f32 spin)
{
    return -PI/2 + spin + (s ? -1.0f : 1.0f)*((f32)i/m)*sweep;
}

static void launch_particles_radial(Rectangle b, Vector2 c, usize n, usize m, const f32* smooth)
{
    (void)smooth;
    f32 half = fminf(b.width, b.height)*0.5f;

    burst_c = c;
    orbit_w = RADIAL_ORBIT;

    for (usize k = 0; k < n; ++k) {
        Particle *p = &particles[k];
        p->attached = false;
        p->pos = homes[k];
        p->start = p->pos;
        p->flash = 1.0f;

        p->vel = burst_velocity(k, p->pos, half);

        f32 r   = half*(0.35f + 0.6f*hash01(k, 23.1f));
        f32 dx  = p->pos.x - c.x, dy = p->pos.y - c.y;
        f32 ang = atan2f(dy, dx) + (hash01(k, 47.3f) - 0.5f)*1.2f;
        p->target = polar(c, r, ang);

        p->age = 0.0f;
    }
}

void visualizer_render_radial(Rectangle boundary, const f32* smooth, const f32* smear,
                              usize m, bool detached, f32 beat, f32 dt)
{
    RORI_ASSERT(smooth != NULL && "dummy dumb dumb");
    RORI_ASSERT(smear != NULL && "dummy dumb dumb");

    update_detach(detached, dt);

    boundary.width *= (1.0f - RADIAL_RIGHT_RESERVE);

    usize sides = RADIAL_MIRROR ? 2 : 1;
    if (m > MAX_PARTICLES/sides) m = MAX_PARTICLES/sides;
    if (m == 0) return;
    usize n = m*sides;

    static f32 spin = 0.0f;
    spin += dt*0.15f*(1.0f - detach);

    f32 half   = fminf(boundary.width, boundary.height)*0.5f;
    f32 r0     = half*0.35f*(1.0f + 0.05f*beat);
    f32 maxlen = half*0.55f;
    Vector2 c  = { boundary.x + boundary.width*0.5f, boundary.y + boundary.height*0.5f };

    f32 sweep      = RADIAL_MIRROR ? PI : 2*PI;
    f32 cell_width = (2*PI*r0)/n;
    f32 saturation = 0.75f, value = 1.0f;
    f32 time = (f32)GetTime();

    f32 bar_scale = 1.0f - detach;

    for (usize s = 0; s < sides; ++s) {
        for (usize i = 0; i < m; ++i) {
            f32 a = radial_angle(i, s, m, sweep, spin);
            homes[s*m + i] = polar(c, r0 + maxlen*smooth[i], a);
        }
    }

    if (detached && !prev_detached) launch_particles_radial(boundary, c, n, m, smooth);
    prev_detached = detached;

    simulate(boundary, n, detached, dt, half*2.0f);

    DrawRing(c, r0 - cell_width*0.6f, r0, 0, 360, 96,
             ColorAlpha(WHITE, (0.10f + 0.25f*beat)*(0.4f + 0.6f*bar_scale)));

    // Render Bars
    for (usize s = 0; s < sides; ++s) {
        for (usize i = 0; i < m; ++i) {
            f32 t = smooth[i]*bar_scale;
            f32 a = radial_angle(i, s, m, sweep, spin);
            Color color = ColorFromHSV((f32)i/m*360, saturation, value);
            DrawLineEx(polar(c, r0, a), polar(c, r0 + maxlen*t, a),
                       cell_width*0.6f*sqrtf(t)*(1.0f + 0.3f*beat), color);
        }
    }

    if (!ready) return;
    Texture2D texture = { rlGetTextureIdDefault(), 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };

    // Render Smears & Trails
    SetShaderValue(circle, radius_loc, (f32[1]){ 0.3f }, SHADER_UNIFORM_FLOAT);
    SetShaderValue(circle, power_loc,  (f32[1]){ 3.0f }, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(circle);
    for (usize s = 0; s < sides; ++s) {
        for (usize i = 0; i < m; ++i) {
            f32 a  = radial_angle(i, s, m, sweep, spin);
            f32 rs = r0 + maxlen*smear[i]*bar_scale;
            f32 re = r0 + maxlen*smooth[i]*bar_scale;
            f32 outer = fmaxf(rs, re), inner = fminf(rs, re);
            if (outer - inner < 0.5f) continue;

            f32 width = cell_width*2.0f*sqrtf(smooth[i]*bar_scale);
            Color color = ColorFromHSV((f32)i/m*360, saturation, value);

            Vector2 p = polar(c, outer, a);
            Rectangle dest = { p.x, p.y, width, outer - inner };
            Vector2 origin = { width/2, 0 };
            Rectangle source = (re <= rs) ? (Rectangle){ 0, 0.0f, 1, 0.5f }
                                          : (Rectangle){ 0, 0.5f, 1, 0.5f };
            DrawTexturePro(texture, source, dest, origin, a*RAD2DEG + 90.0f, color);
        }
    }
    for (usize s = 0; s < sides; ++s) {
        for (usize i = 0; i < m; ++i) {
            usize k = s*m + i;
            const Particle *p = &particles[k];
            f32 t = smooth[i];
            f32 radius = cell_width*3.0f*sqrtf(t)*(1.0f + 1.2f*detach)*(1.0f + 0.35f*beat)*ball_scale(p, k, time);
            Color color = ColorFromHSV((f32)i/m*360, saturation, value);
            draw_trail(texture, p, radius, half*2.0f, color, detach);
        }
    }
    EndShaderMode();

    SetShaderValue(circle, radius_loc, (f32[1]){ 0.07f }, SHADER_UNIFORM_FLOAT);
    SetShaderValue(circle, power_loc,  (f32[1]){ 5.0f },  SHADER_UNIFORM_FLOAT);

    // Render Particles
    BeginShaderMode(circle);
    for (usize s = 0; s < sides; ++s) {
        for (usize i = 0; i < m; ++i) {
            usize k = s*m + i;
            const Particle *p = &particles[k];

            f32 t = smooth[i];
            if (t < 0.001f) continue;

            Vector2 center = (!p->attached) ? p->pos : homes[k];
            f32 sc = (!p->attached) ? ball_scale(p, k, time) : 1.0f;
            f32 radius = cell_width*3.0f*sqrtf(t)*(1.0f + 1.2f*detach)*(1.0f + 0.35f*beat)*sc;
            Color color = ColorFromHSV((f32)i/m*360, saturation, value);
            f32 flash = (!p->attached) ? p->flash : 0.0f;
            color = ColorBrightness(color, 0.25f*beat + FLASH_BRIGHT*flash);
            Vector2 pos = { center.x - radius, center.y - radius };
            DrawTextureEx(texture, pos, 0, 2*radius, color);
        }
    }
    EndShaderMode();
}
