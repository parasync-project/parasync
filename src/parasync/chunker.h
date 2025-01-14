#ifndef CHUNKER_H
#define CHUNKER_H

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <zlib.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <atomic>
#include <thread>
#include <mimalloc-2.1/mimalloc.h>
#include "skysync_f.h"
#include "skysync_c.h"
#include "parasync_common.h"

struct bitmap {
    uint8_t *bits;
    uint64_t size;
};

struct bitmap* bitmap_create(uint64_t fs);

void bitmap_set(struct bitmap* bm, uint64_t index);

void bitmap_clear(struct bitmap* bm, uint64_t index);

uint8_t bitmap_test(struct bitmap* bm, uint64_t index);

void bitmap_destroy(struct bitmap* bm);

typedef struct {
    uint8_t tid;
    char *map;
    uint64_t offset;
    uint64_t length;
    struct file_cdc* fc; // deprecated
    struct bitmap* bm; // deprecated
    DataQueue<one_cdc> *csums_queue;
} csegment;

typedef struct {
    std::atomic<csegment *> seg;
    uint64_t offset;
    uint64_t length;
} thread_args;


class chunker
{
public:
    csegment *seg;
    thread_local static uint64_t offset;
    thread_local static uint64_t start_offset;
    thread_local static uint64_t end_offset;
    
    thread_local static uint64_t tls_fingerprint;
    thread_local static unsigned char *tls_map_t;
    thread_local static unsigned char *tls_pre;
    thread_local static uint64_t tls_FING_GEAR_08KB_64;
    thread_local static uint64_t tls_GEARv2_tmp[256];

public:
    explicit chunker() : seg(nullptr) {}
    virtual ~chunker()
    {
        // if (cargs) {
        //     mi_free(cargs);
        // }
    }
};

class chunker_1 : public chunker
{
public:
    using chunker::chunker;

    void init(const char *map, uint64_t off_t, uint64_t length);
    void run(struct bitmap* bm);
    csegment* task(struct bitmap* bm);
};

class chunker_crc : public chunker
{
public:
    using chunker::chunker;

    void init(const char *map, uint64_t off_t, uint64_t length);
    void run();
    csegment* task();
};

#endif // CHUNKER_H