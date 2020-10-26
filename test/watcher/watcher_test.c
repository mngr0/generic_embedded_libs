#include <string.h>
#include "unity.h"
#include "watcher/watcher.h"
#include "gel_conf.h"

char      var1 = 0;
int       var2 = 0;
long long var3 = 0;
struct {
    char      a;
    int       b;
    long      c;
    long long d;
} var4             = {0};
uint64_t array[10] = {0};

static int cbtest = 0;

void setUp(void) {
    cbtest = 0;
}

void tearDown(void) {}

void callback(void *var, void *data) {
    (void)var;
    TEST_ASSERT_EQUAL(data, NULL);
    cbtest++;
}

void test_watcher() {
    watcher_t list[] = {
        WATCHER(&var1, callback, NULL), WATCHER(&var2, callback, NULL),           WATCHER(&var3, callback, NULL),
        WATCHER(&var4, callback, NULL), WATCHER_ARRAY(array, 10, callback, NULL),
    };

    watcher_list_init(list, sizeof(list) / sizeof(list[0]));

    TEST_ASSERT(!WATCHER_PROCESS_CHANGES(list, 0));
    var2++;
    TEST_ASSERT(WATCHER_PROCESS_CHANGES(list, 0));
    TEST_ASSERT_EQUAL(1, cbtest);
}


void test_watcher_delayed() {
    watcher_t list[] = {
        WATCHER_DELAYED(&var1, callback, NULL, 5000),
    };

    watcher_list_init(list, sizeof(list) / sizeof(list[0]));

    var1++;
    TEST_ASSERT(WATCHER_PROCESS_CHANGES(list, 0));
    TEST_ASSERT(!WATCHER_PROCESS_CHANGES(list, 1000));
    TEST_ASSERT(!WATCHER_PROCESS_CHANGES(list, 4000));
    var1++;
    TEST_ASSERT(WATCHER_PROCESS_CHANGES(list, 6000));
    TEST_ASSERT_EQUAL(0, cbtest);
    TEST_ASSERT(!WATCHER_PROCESS_CHANGES(list, 11000));
    TEST_ASSERT_EQUAL(1, cbtest);
}