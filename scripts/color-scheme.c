#!/usr/bin/env -S tcc -run
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOTIFY_TIMEOUT "5000"
const char* GSETTINGS_PATH = "/org/gnome/desktop/interface/color-scheme";
const char* DCONF_READ     = "dconf read %s";
const char* DCONF_WRITE    = "dconf write %s \"'%s'\"";

int main() {
    char buffer[128];
    char cmd_read[256];
    char cmd_write[256];
    char notify_cmd[256];
    char *new_mode_text;
    char *new_mode_val;
    FILE *fp;

    snprintf(cmd_read, sizeof(cmd_read), DCONF_READ, GSETTINGS_PATH);
    fp = popen(cmd_read, "r");
    if (fp == NULL) return 1;

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strstr(buffer, "prefer-dark") != NULL) {
            new_mode_val  = "prefer-light";
            new_mode_text = "Ljust läge";
        } else {
            new_mode_val  = "prefer-dark";
            new_mode_text = "Mörkt läge";
        }

        snprintf(cmd_write, sizeof(cmd_write), DCONF_WRITE, GSETTINGS_PATH, new_mode_val);
        system(cmd_write);

        snprintf(notify_cmd, sizeof(notify_cmd), "notify-send -t %s 'System' 'Ändrat till %s'", NOTIFY_TIMEOUT, new_mode_text);
        system(notify_cmd);
    }

    pclose(fp);
    return 0;
}
