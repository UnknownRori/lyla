#include "ui.h"
#include "text.h"
#include "common.h"
#include "utils.h"

#define HUD_SWEEP_TIME 1.2f
#define HUD_END_OFFSET 0.5f

static void hud_draw_sweep_line(f32 line_x, f32 cy, f32 line_w, f32 line_h, f32 scale)
{
    if (line_h <= 0.0f) return;

    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 6; i >= 1; i--) {
        f32 spread = i * 3.0f * scale;
        f32 bw = line_w + spread * 2.0f;
        f32 bh = line_h + spread;
        unsigned char a = (unsigned char)(10 + (7 - i) * 3);
        DrawRectangleRounded(
            (Rectangle){ line_x - bw / 2.0f, cy - bh / 2.0f, bw, bh },
            1.0f,
            8,
            (Color){ 255, 255, 255, a }
        );
    }
    EndBlendMode();

    DrawRectangle(
        (int)(line_x - line_w / 2.0f),
        (int)(cy - line_h / 2.0f),
        (int)line_w,
        (int)line_h,
        WHITE
    );
}

static int hud_track_sweep(f32 elapsed, f32 duration, f32* sweep, f32* line_scale)
{
    f32 eff_duration = duration - HUD_END_OFFSET;
    if (duration > 0.0f && eff_duration < 0.0f) eff_duration = 0.0f;

    f32 remaining = eff_duration - elapsed;
    int mode = 0;
    f32 t = 1.0f;

    if (elapsed < HUD_SWEEP_TIME) {
        mode = 1;
        t = elapsed / HUD_SWEEP_TIME;
    } else if (duration > 0.0f && remaining < HUD_SWEEP_TIME) {
        mode = 2;
        t = 1.0f - remaining / HUD_SWEEP_TIME;
    } else {
        return 0;
    }

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    if (t < 0.15f) {
        *sweep = 0.0f;
        *line_scale = t / 0.15f;
    } else if (t < 0.85f) {
        f32 p = (t - 0.15f) / 0.7f;
        *sweep = p * p * (3.0f - 2.0f * p);
        *line_scale = 1.0f;
    } else {
        *sweep = 1.0f;
        *line_scale = 1.0f - (t - 0.85f) / 0.15f;
    }

    return mode;
}

static void hud_draw_track_thumbnail(Track* track, f32 group_cx, f32 thumb_size, f32 padding, f32* current_y)
{
    Rectangle dest = { group_cx - thumb_size / 2.0f, *current_y, thumb_size, thumb_size };
    DrawTexturePro(*track->thumbnail, (Rectangle){ 0, 0, track->thumbnail->width, track->thumbnail->height }, 
                   dest, (Vector2){0, 0}, 0.0f, WHITE);
    *current_y += thumb_size + padding;
}

static void hud_draw_track_title(f32 group_cx, f32 text_size, f32 scale, f32* current_y)
{
    const char *text = TextFormat("%s", player_name());
    f32 actual_text_size = text_size < 12.0f ? 12.0f : text_size; 
    f32 text_w = text_width(text, actual_text_size);
    f32 text_x = group_cx - text_w / 2.0f;

    Color text_col = { 255, 255, 255, player_paused() ? 200 : 255 };
    hud_draw_text_with_bg(
        text, 
        text_x, 
        *current_y, 
        actual_text_size, 
        text_col, 
        BLACK, 
        10.0f * scale, 
        9.0f * scale, 
        0.5f
    );
    *current_y += actual_text_size;
}

static void hud_draw_track_metadata(Track* track, f32 group_cx, f32 meta_text_size, f32 padding, f32 scale, f32* current_y)
{
    *current_y += (padding * 0.75f);

    const char* meta_text;
    if (track->artist && track->album) {
        meta_text = TextFormat("%s by %s", track->album, track->artist);
    } else if (track->artist) {
        meta_text = TextFormat("%s", track->artist);
    } else {
        meta_text = track->album;
    }

    f32 actual_meta_size = meta_text_size < 9.0f ? 9.0f : meta_text_size;
    f32 meta_w = text_width(meta_text, actual_meta_size);
    f32 meta_x = group_cx - meta_w / 2.0f;

    hud_draw_text_with_bg(
        meta_text, 
        meta_x, 
        *current_y, 
        actual_meta_size, 
        LIGHTGRAY, 
        BLACK, 
        6.0f * scale, 
        5.0f * scale, 
        0.5f
    );
    *current_y += actual_meta_size;
}

static f32 hud_track_content_width(Track* track, bool has_thumb, bool has_qr, bool has_artist_or_album, f32 thumb_size, f32 qr_size, f32 text_size, f32 meta_text_size, f32 bc_text_size)
{
    f32 max_w = text_width(TextFormat("%s", player_name()), text_size < 12.0f ? 12.0f : text_size);

    if (has_thumb && thumb_size > max_w) max_w = thumb_size;
    if (has_qr && qr_size > max_w) max_w = qr_size;

    if (has_artist_or_album) {
        const char* meta_text;
        if (track->artist && track->album) {
            meta_text = TextFormat("%s by %s", track->album, track->artist);
        } else if (track->artist) {
            meta_text = TextFormat("%s", track->artist);
        } else {
            meta_text = track->album;
        }
        f32 meta_w = text_width(meta_text, meta_text_size < 9.0f ? 9.0f : meta_text_size);
        if (meta_w > max_w) max_w = meta_w;
    }

    if (has_qr) {
        f32 bc_w = text_width("bandcamp", bc_text_size < 8.0f ? 8.0f : bc_text_size);
        if (bc_w > max_w) max_w = bc_w;
    }

    return max_w;
}

static void hud_draw_track_qrcode(Track* track, f32 group_cx, f32 qr_size, f32 bc_text_size, f32 padding, f32 stroke, f32 scale, f32* current_y)
{
    Rectangle dest = { group_cx - qr_size / 2.0f, *current_y, qr_size, qr_size };
    DrawRectangle(
        dest.x - stroke, 
        dest.y - stroke, 
        dest.width + (stroke * 2), 
        dest.height + (stroke * 2), BLACK
    ); 
    DrawTexturePro(
        *track->qrcode,
        RECT(0, 0, track->qrcode->width, track->qrcode->height),
        dest, 
        VEC2(0, 0), 
        0.0f, 
        WHITE
    );
    
    *current_y += qr_size + (padding / 2.0f);
    
    const char* bc_text = "bandcamp";
    f32 actual_bc_size = bc_text_size < 8.0f ? 8.0f : bc_text_size;
    f32 bc_w = text_width(bc_text, actual_bc_size);
    f32 bc_x = group_cx - bc_w / 2.0f;
    
    hud_draw_text_with_bg(
        bc_text, 
        bc_x, 
        *current_y, 
        actual_bc_size, 
        LIGHTGRAY, 
        BLACK, 
        6.0f * scale, 
        4.0f * scale, 
        0.5f
    );
}

void hud_draw_track_info(Track* track, int w, int h, f32 elapsed, f32 duration)
{
    if (!track) return;

    bool has_thumb = track->thumbnail != NULL;
    bool has_qr = track->qrcode != NULL;
    bool has_artist_or_album = (track->artist && track->artist[0] != '\0') || (track->album && track->album[0] != '\0');

    f32 base_thumb_size = 256.0f;
    f32 base_qr_size = 128.0f;
    f32 base_text_size = 22.0f; 
    f32 base_meta_text_size = 13.0f;
    f32 base_bc_text_size = 14.0f;
    f32 base_padding = 20.0f;

    f32 total_height = base_text_size;
    if (has_thumb) total_height += base_thumb_size + base_padding;
    if (has_artist_or_album) total_height += (base_padding * 0.75f) + base_meta_text_size;
    if (has_qr) {
        total_height += base_padding + base_qr_size;
        total_height += (base_padding / 2.0f) + base_bc_text_size;
    }

    f32 max_allowed_height = (h - HUD_TIMELINE_HEIGHT) * 0.85f;
    f32 max_allowed_width = w * 0.40f;

    f32 scale = 1.0f;
    if (total_height > max_allowed_height) {
        scale = max_allowed_height / total_height;
    }
    if (base_thumb_size * scale > max_allowed_width) {
        scale = max_allowed_width / base_thumb_size;
    }
    
    f32 thumb_size = base_thumb_size * scale;
    f32 qr_size = base_qr_size * scale;
    f32 text_size = base_text_size * scale;
    f32 meta_text_size = base_meta_text_size * scale;
    f32 bc_text_size = base_bc_text_size * scale;
    f32 padding = base_padding * scale;
    f32 stroke = 4.0f * scale;

    f32 scaled_total_height = text_size;
    if (has_thumb) scaled_total_height += thumb_size + padding;
    if (has_artist_or_album) scaled_total_height += (padding * 0.75f) + meta_text_size;
    if (has_qr) scaled_total_height += padding + qr_size + (padding / 2.0f) + bc_text_size;

    f32 group_cx = w * 0.75f; 
    f32 group_cy = (h - HUD_TIMELINE_HEIGHT) * 0.5f;
    f32 current_y = group_cy - (scaled_total_height / 2.0f);

    f32 content_w = hud_track_content_width(track, has_thumb, has_qr, has_artist_or_album, thumb_size, qr_size, text_size, meta_text_size, bc_text_size);
    f32 region_w = content_w + padding * 2.0f;
    f32 region_left = group_cx - region_w / 2.0f;
    f32 sweep = 0.0f;
    f32 line_scale = 0.0f;
    int mode = hud_track_sweep(elapsed, duration, &sweep, &line_scale);
    f32 line_x = region_left + region_w * sweep;

    if (mode == 1) {
        BeginScissorMode(0, 0, (int)line_x, h);
    } else if (mode == 2) {
        BeginScissorMode((int)line_x, 0, w - (int)line_x, h);
    }

    if (has_thumb) {
        hud_draw_track_thumbnail(track, group_cx, thumb_size, padding, &current_y);
    }

    hud_draw_track_title(group_cx, text_size, scale, &current_y);

    if (has_artist_or_album) {
        hud_draw_track_metadata(track, group_cx, meta_text_size, padding, scale, &current_y);
    }
    
    current_y += padding;

    if (has_qr) {
        hud_draw_track_qrcode(track, group_cx, qr_size, bc_text_size, padding, stroke, scale, &current_y);
    }

    if (mode != 0) {
        EndScissorMode();

        f32 line_w = 3.0f * scale < 2.0f ? 2.0f : 3.0f * scale;
        f32 line_h = (scaled_total_height + padding * 2.0f) * line_scale;
        hud_draw_sweep_line(line_x, group_cy, line_w, line_h, scale);
    }
}
