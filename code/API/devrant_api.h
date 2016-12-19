#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <curl/curl.h>

#include "json/json.c"

typedef i8 int8_t;
typedef i16 int16_t;
typedef i32 int32_t;
typedef i64 int64_t;

typedef u8 uint8_t;
typedef u16 uint16_t;
typedef u32 uint32_t;
typedef u64 uint64_t;

typedef f32 float;
typedef f64 double;

typedef enum sort_types {
    ALGO,
    RECENT,
    TOP,
}

typedef struct rant_s {
    i32 id;
    char *user;
    i32 upvotes;
    i32 downvotes;
    i32 score;
    char *content;
} rant
