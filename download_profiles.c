#include <stdio.h>
#include <string.h>
#include "download_profiles.h"

static const DownloadProfile s_profiles[] = {
    {
        "1",
        "Video (MP4, Best quality - Auto 4K/2K/1080p, Embedded Subs & Cover)",
        "videos",
        "%(title)s [%(id)s].%(ext)s",
        false,
        false
    },
    {
        "2",
        "Video (MP4, 1080p Max)",
        "videos",
        "%(title)s [%(id)s].%(ext)s",
        false,
        false
    },
    {
        "3",
        "Video (MP4, 720p Max)",
        "videos",
        "%(title)s [%(id)s].%(ext)s",
        false,
        false
    },
    {
        "4",
        "Audio only (MP3 320k, with Cover Art)",
        "videos/audio/mp3",
        "%(title)s [%(id)s].%(ext)s",
        false,
        true
    },
    {
        "5",
        "Audio only (Best M4A, with Cover Art)",
        "videos/audio/m4a",
        "%(title)s [%(id)s].%(ext)s",
        false,
        true
    },
    {
        "6",
        "Entire Playlist (MP4, organized in playlist folder)",
        "videos/playlists",
        "%(playlist_title|Unknown)s/%(playlist_index|0)s - %(title)s [%(id)s].%(ext)s",
        true,
        false
    }
};

const DownloadProfile *profile_get(const char *choice_id) {
    if (choice_id && *choice_id >= '1' && *choice_id <= '6') {
        int idx = *choice_id - '1';
        return &s_profiles[idx];
    }
    return &s_profiles[0]; // 預設 1 號 Best Quality
}

void profile_print_menu(void) {
    printf("Select download format:\n");
    for (int i = 0; i < 6; ++i) {
        printf("   [%s] %s%s\n", s_profiles[i].choice_id, s_profiles[i].name, (i == 0) ? " [Default]" : "");
    }
    printf("\n");
}
