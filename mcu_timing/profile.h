#ifndef PROFILE_H
#define PROFILE_H

#include <stdint.h>

#define MAX_PROFILES 10

typedef struct {
    uint64_t call_count;
    uint64_t av_ticks;
    uint64_t timestamp;
    const char *label;
} Profile;

void profile_init(Profile *prof, const char *label);
void profile_reset(Profile *prof);
void profile_start(Profile *prof);
void profile_end(Profile *prof);
int profile_list_size(void);

/*
 * Get the list of all profiles and the number of profiles in that list
 * list = pointer to array of Profile pointers
 * Returns profile_list_size
 */
int profile_get_data(Profile **list[MAX_PROFILES]);

#endif



