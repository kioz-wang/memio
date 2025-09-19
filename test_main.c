/**
 * @file test_main.c
 * @author kioz.wang
 * @brief
 * @version 1.0
 * @date 2025-09-19
 *
 * @copyright MIT License
 *
 *  Copyright (c) 2025 kioz.wang
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include "position.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define key_str(key) #key
#define key_id(key)  prm_id_##key

enum {
    key_id(version) = 0,
    key_id(length),
    key_id(major),
    key_id(minor),
    key_id(author),
    key_id(time),
    key_id(debug),
    key_id(phase),
    key_id(flag),
    key_id(crc),
    key_id(null) = LST_SEARCH_ID_END,
};

typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint16_t length;
    uint8_t  major : 4;
    uint8_t  minor : 4;
    uint8_t  author[10];
    uint32_t time;
    uint16_t debug : 1;
    uint16_t phase : 3;
    uint16_t flag  : 12;
    uint32_t crc;
} memory_struct_t;

memory_struct_t memory_instance = {};

pos_t memory_layout[] = {
    POS_ITEM(key_id(version), key_str(version), 0, 1, ~0),
    POS_ITEM(key_id(length), key_str(length), 1, 2, ~0),
    POS_ITEM(key_id(major), key_str(major), 3, 1, 0x0000000f),
    POS_ITEM(key_id(minor), key_str(minor), 3, 1, 0x000000f0),
    POS_ITEM(key_id(author), key_str(author), 4, 10, 0),
    POS_ITEM(key_id(time), key_str(time), 14, 4, ~0),
    POS_ITEM(key_id(debug), key_str(debug), 18, 2, 0x00000001),
    POS_ITEM(key_id(phase), key_str(phase), 18, 2, 0x0000000e),
    POS_ITEM(key_id(flag), key_str(flag), 18, 2, 0x0000fff0),
    POS_ITEM(key_id(null), NULL, 0, 0, 0),
};

#define assert_eq(memory, layout, member)                                                                              \
    {                                                                                                                  \
        uint32_t value = 0;                                                                                            \
        assert(!pos_read(pos_search(layout, key_id(member)), memory, &value, sizeof(value)));                          \
        assert(value == (memory)->member);                                                                             \
        printf("    assert_eq %s\n", key_str(member));                                                                 \
    }

void do_assert_eq(void) {
    assert_eq(&memory_instance, memory_layout, version);
    assert_eq(&memory_instance, memory_layout, length);
    assert_eq(&memory_instance, memory_layout, major);
    assert_eq(&memory_instance, memory_layout, minor);
    assert_eq(&memory_instance, memory_layout, time);
    assert_eq(&memory_instance, memory_layout, debug);
    assert_eq(&memory_instance, memory_layout, phase);
    assert_eq(&memory_instance, memory_layout, flag);
}

#define assert_eq_data(memory, layout, member)                                                                         \
    {                                                                                                                  \
        uint8_t data[64] = {};                                                                                         \
        assert(!pos_read(pos_search(layout, key_id(member)), memory, data, sizeof(data)));                             \
        assert(!memcmp(data, (memory)->member, sizeof((memory)->member)));                                             \
        printf("    assert_eq_data %s\n", key_str(member));                                                            \
    }

void do_assert_eq_data(void) { assert_eq_data(&memory_instance, memory_layout, author); }

void TEST_search(void) {
    printf("Begin %s\n", __func__);

    puts("\n search");
    assert(!pos_search(memory_layout, key_id(crc)));
    printf("    notfound %s\n", key_str(crc));
    assert(pos_search(memory_layout, key_id(time)) == pos_search_by_name(memory_layout, key_str(time)));
    printf("    search %s\n", key_str(time));

    printf("\nEnd %s\n", __func__);
}

void random_memory(void *base, uint32_t length) {
    for (int i = 0; i < length; i++) {
        ((uint8_t *)base)[i] = (uint32_t)rand() & UINT8_MAX;
    }
}

void TEST_map(void) {
    printf("Begin %s\n", __func__);
    random_memory(&memory_instance, sizeof(memory_instance));

    puts("\n assert_eq");
    do_assert_eq();

    puts("\n do_assert_eq_data");
    do_assert_eq_data();

    printf("\nEnd %s\n", __func__);
}

void TEST_write(void) {
    printf("Begin %s\n", __func__);
    const pos_t *pos      = NULL;
    uint8_t      zero[64] = {};
    uint8_t      data[64] = {};
    uint64_t     u64      = 0;
    uint32_t     u32      = 0;
    uint16_t     u16      = 0;
    uint8_t      u_8      = 0;

    puts("\n assert_write_data");
    pos = pos_search(memory_layout, key_id(author));

    assert(!pos_write(pos, &memory_instance, "hello world", 1));
    assert(!pos_read(pos, &memory_instance, data, sizeof(data)));
    assert(data[0] == 'h' && !memcmp(zero, &data[1], pos->length - 1));

    assert(!pos_write(pos, &memory_instance, "hello world", 6));
    assert(!pos_read(pos, &memory_instance, data, sizeof(data)));
    assert(!memcmp("hello world", data, 6) && !memcmp(zero, &data[6], pos->length - 6));

    assert(-ENOBUFS == pos_write(pos, &memory_instance, "hello world", 100));
    assert(-ENOBUFS == pos_read(pos, &memory_instance, data, 1));

    puts("\n assert_write");
    pos = pos_search(memory_layout, key_id(flag));

    assert(-ENOBUFS == pos_write(pos, &memory_instance, &u64, sizeof(u64)));
    u32 = UINT32_MAX;
    assert(-ENOBUFS == pos_write(pos, &memory_instance, &u32, sizeof(u32)));
    u32 = UINT8_MAX;
    assert(!pos_write(pos, &memory_instance, &u32, sizeof(u32)));
    assert(-ENOBUFS == pos_read(pos, &memory_instance, &u_8, sizeof(u_8)));
    assert(!pos_read(pos, &memory_instance, &u16, sizeof(u16)));
    assert(u16 == u32);

    printf("\nEnd %s\n", __func__);
}

int main(int argc, char const *argv[]) {
    srand(time(NULL));

    TEST_search();
    for (int i = 0; i < 10; i++) {
        TEST_map();
    }
    TEST_write();

    return 0;
}
