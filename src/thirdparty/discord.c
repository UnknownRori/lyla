#include "raylib.h"
#ifndef  NO_DISCORD
#include "discord.h"
#include "music/player.h"

#include <discord_rpc.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char* last_path = NULL;
static bool last_paused = false;
static bool force_update = true;

static void handle_ready(const DiscordUser* connectedUser) 
{
    // printf("\nDiscord: Connected to user %s#%s - %s\n", 
    //        connectedUser->username, 
    //        connectedUser->discriminator, 
    //        connectedUser->userId);
}

static void handle_disconnected(int errcode, const char* message) 
{
    // printf("\nDiscord: Disconnected (%d: %s)\n", errcode, message);
}

static void handle_error(int errcode, const char* message) 
{
    printf("\nDiscord: Error (%d: %s)\n", errcode, message);
}

void discord_init(const char* client_id) 
{
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handle_ready;
    handlers.disconnected = handle_disconnected;
    handlers.errored = handle_error;
    
    Discord_Initialize(client_id, &handlers, 1, NULL);
}

void discord_update_presence(Track* track, bool paused) 
{
    const char* current_path = track ? track->path : NULL;

    if (!force_update && current_path == last_path && paused == last_paused) return;

    last_path = current_path;
    last_paused = paused;
    force_update = false;

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    static char state_buffer[256];
    if (track) {
        discordPresence.details = track->title ? track->title : GetFileNameWithoutExt(track->path); 

        if (track->artist) {
            snprintf(state_buffer, sizeof(state_buffer), "by %s%s", 
                     track->artist, 
                     paused ? " [Paused]" : "");
        } else {
            snprintf(state_buffer, sizeof(state_buffer), "%s", paused ? "Paused" : "Playing");
        }
        discordPresence.state = state_buffer;

        if (!paused) {
            time_t now = time(NULL);
            discordPresence.startTimestamp = now - (time_t)player_time();
            discordPresence.endTimestamp = 0; 
        }
    } else {
        discordPresence.state = "Idling";
        discordPresence.details = "Waiting for music...";
    }

    discordPresence.largeImageKey = "logo"; 
    discordPresence.largeImageText = track && track->title ? track->title : "Music Visualizer";

    Discord_UpdatePresence(&discordPresence);
}

void discord_update(void) 
{
    Discord_RunCallbacks();
}

void discord_shutdown(void) 
{
    Discord_Shutdown();
}
#else

#include "discord.h"
void discord_init(const char* client_id) {}

void discord_update_presence(Track* track, bool paused) {}

void discord_update(void) {}

void discord_shutdown(void) {}

#endif
