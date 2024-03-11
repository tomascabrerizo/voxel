#ifndef _COMMON_H_
#define _COMMON_H_

#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef float f32;
typedef double f64;
typedef u32 b32;

#define unused(variable) (void)(variable)
#define array_len(array) ((sizeof((array))) / sizeof((array)[0]))
#define offset_of(type, value) (&(((type *)0)->value))
#define is_power_of_two(value) (((value) & (value - 1)) == 0)
#define next_power_of_two(value)                                                                   \
    do {                                                                                           \
        value--;                                                                                   \
        value |= value >> 1;                                                                       \
        value |= value >> 2;                                                                       \
        value |= value >> 4;                                                                       \
        value |= value >> 8;                                                                       \
        value |= value >> 16;                                                                      \
        value++;                                                                                   \
    } while(0)

#define list_init(node) ((node)->prev = (node), (node)->next = (node))

#define list_insert_front(node0, node1)                                                            \
    (node1)->prev       = (node0);                                                                 \
    (node1)->next       = (node0)->next;                                                           \
    (node1)->prev->next = (node1);                                                                 \
    (node1)->next->prev = (node1);

#define list_insert_back(node0, node1)                                                             \
    (node1)->prev       = (node0)->prev;                                                           \
    (node1)->next       = (node0);                                                                 \
    (node1)->prev->next = (node1);                                                                 \
    (node1)->next->prev = (node1);

#define list_remove(node)                                                                          \
    (node)->prev->next = (node)->next;                                                             \
    (node)->next->prev = (node)->prev;

#define list_is_empty(dummy) ((dummy)->next == (dummy) && (dummy)->prev == (dummy))

#define list_is_end(dummy, node) ((node) == (dummy))

#define list_is_first(dummy, node) ((node)->prev == (dummy))

#define list_is_last(dummy, node) ((node)->next == (dummy))

#define list_get_back(dummy) (dummy)->prev

#define list_get_top(dummy) (dummy)->next

// NOTE: named link list

#define list_init_named(node, name) ((node)->prev_##name = (node), (node)->next_##name = (node))

#define list_insert_front_named(node0, node1, name)                                                \
    (node1)->prev_##name              = (node0);                                                   \
    (node1)->next_##name              = (node0)->next_##name;                                      \
    (node1)->prev_##name->next_##name = (node1);                                                   \
    (node1)->next_##name->prev_##name = (node1);

#define list_insert_back_named(node0, node1, name)                                                 \
    (node1)->prev_##name              = (node0)->prev_##name;                                      \
    (node1)->next_##name              = (node0);                                                   \
    (node1)->prev_##name->next_##name = (node1);                                                   \
    (node1)->next_##name->prev_##name = (node1);

#define list_remove_named(node, name)                                                              \
    (node)->prev_##name->next_##name = (node)->next_##name;                                        \
    (node)->next_##name->prev_##name = (node)->prev_##name;

#define list_is_empty_named(dummy, name)                                                           \
    ((dummy)->next_##name == (dummy) && (dummy)->prev_##name == (dummy))

#define list_is_end_named(dummy, node, name) ((node) == (dummy))

#define list_is_first_named(dummy, node, name) ((node)->prev_##name == (dummy))

#define list_is_last_named(dummy, node, name) ((node)->next_##name == (dummy))

#define list_get_back_named(dummy, name) (dummy)->prev_##name

#define list_get_top_named(dummy, name) (dummy)->next_##name

#endif //  _COMMON_H_
