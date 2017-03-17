#ifndef PROFILE_H
#define PROFILE_H

#include <stdint.h>

#define MAX_PROFILES 100

typedef struct {
    uint64_t call_count;
    uint64_t ticks;
    uint64_t max_ticks;
    uint64_t timestamp;
    const char *label;
} Profile;

void profile_init(Profile *prof, const char *label);
void profile_reset(Profile *prof);
void profile_start(Profile *prof);
void profile_end(Profile *prof);
void profile_end_ptr(Profile **prof);
uint64_t profile_get_average(Profile *prof);
uint64_t profile_get_max(Profile *prof);
int profile_list_size(void);

/*
 * Get the list of all profiles and the number of profiles in that list
 * list = pointer to array of Profile pointers
 * Returns profile_list_size
 */
int profile_get_data(Profile **list[MAX_PROFILES]);

#if PROFILE_ENABLED == 1
#define PROFILE \
    static Profile prof = { \
        .call_count = 0, \
        .ticks = 0, \
        .max_ticks = 0, \
        .timestamp = 0, \
        .label = 0 \
    }; \
    Profile *prof_ptr __attribute__ ((__cleanup__(profile_end_ptr))) = &prof; \
    if (!prof.label) { \
        profile_init(&prof, __func__); \
    } \
    profile_start(&prof);
#else
#define PROFILE
#endif

#endif



