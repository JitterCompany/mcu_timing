#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "unity.h"

#include "profile.h"

int count = 0;
int delay = 100;
uint64_t delay_get_timestamp()
{
    return count++ *delay;
}

void do_func()
{
    int fcount = 0;
    while(fcount++ < 20000000){}
    return;
}

void test_profile()
{
    Profile prof;
    profile_init(&prof);

    delay = 100;
    profile_start(&prof);
    do_func();
    profile_end(&prof);

    TEST_ASSERT_EQUAL(1, prof.call_count);
    printf("Hello World!\n"); // TODO RM
    TEST_ASSERT_EQUAL(100, prof.av_ticks);
    printf("this: %d should be 101\n", 101);


    delay = 300;
    profile_start(&prof);
    do_func();
    profile_end(&prof);

    TEST_ASSERT_EQUAL(2, prof.call_count);
    TEST_ASSERT_EQUAL(200, prof.av_ticks);
    TEST_ASSERT_EQUAL(1, profile_list_size());

    Profile prof2;
    profile_init(&prof2);
    Profile **list = NULL;
    int n = profile_get_data(&list);
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_NOT_EQUAL(0, list);
    TEST_ASSERT_EQUAL(2, list[0]->call_count);
    TEST_ASSERT_EQUAL(0, list[1]->call_count);
    


}

void test_dummy(void)
{
    printf("Hi from test_dummy()\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_profile);
    RUN_TEST(test_dummy);
    UNITY_END();
    return 0;
}
