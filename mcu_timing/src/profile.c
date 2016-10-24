#include "profile.h"
#include "delay.h"

static Profile *profile_list[MAX_PROFILES];
static int num_profiles = 0;

void profile_init(Profile *prof, const char *label)
{
    prof->call_count = 0;
    prof->av_ticks = 0;
    prof->label = label;

    if (num_profiles < MAX_PROFILES) {
        profile_list[num_profiles++] = prof;
    }
}

void profile_start(Profile *prof)
{
    prof->timestamp = delay_get_timestamp();
}

void profile_end(Profile *prof)
{
    uint64_t end = delay_get_timestamp();
    uint64_t d = end - prof->timestamp;

    prof->call_count++;
    
    prof->av_ticks -= (prof->av_ticks / prof->call_count);
    prof->av_ticks += (d / prof->call_count);
}

int profile_list_size(void)
{
    return num_profiles;
}

int profile_get_data(Profile **list[MAX_PROFILES])
{
    *list = (profile_list);
    return profile_list_size();
}
