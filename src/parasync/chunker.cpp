#include <assert.h>
#include <nmmintrin.h>
#include <chrono>
#include <isa-l/crc.h>
#include "chunker.h"

// Define thread-local variables
thread_local uint64_t chunker::offset;
thread_local uint64_t chunker::start_offset;
thread_local uint64_t chunker::end_offset;

thread_local uint64_t chunker::tls_fingerprint;
thread_local unsigned char* chunker::tls_map_t;
thread_local unsigned char* chunker::tls_pre;
thread_local uint64_t chunker::tls_FING_GEAR_08KB_64;
thread_local uint64_t chunker::tls_GEARv2_tmp[256];

struct bitmap* bitmap_create(uint64_t fs) {
    struct bitmap* bm = (struct bitmap*)mi_zalloc(sizeof(struct bitmap));
    assert(bm);
    bm->size = fs;
    bm->bits = (uint8_t*)mi_zalloc((fs + 7) / 8);
    assert(bm->bits);
    return bm;
}

void bitmap_set(struct bitmap* bm, uint64_t index) {
    if (!bm || !bm->bits || index >= bm->size) return;
    uint64_t byte_index = index >> 3;
    uint64_t bit_index = 7 - (index % 8);  // Reverse bit order within byte
    bm->bits[byte_index] |= (1U << bit_index);
}

void bitmap_clear(struct bitmap* bm, uint64_t index) {
    if (!bm || !bm->bits || index >= bm->size) return;
    uint64_t byte_index = index / 8;
    uint64_t bit_index = 7 - (index % 8);  // Reverse bit order within byte
    bm->bits[byte_index] &= ~(1U << bit_index);
}

uint8_t bitmap_test(struct bitmap* bm, uint64_t index) {
    if (!bm || !bm->bits || index >= bm->size) return 0;
    uint64_t byte_index = index / 8;
    uint64_t bit_index = 7 - (index % 8);  // Reverse bit order within byte
    return (bm->bits[byte_index] >> bit_index) & 1U;
}

void bitmap_destroy(struct bitmap* bm) {
    if (bm) {
        mi_free(bm->bits);
        mi_free(bm);
    }
}

void chunker_1::init(const char *map, uint64_t off_t, uint64_t length) {
    seg = (csegment *)mi_zalloc(sizeof(csegment));
    seg->offset = off_t;
    seg->length = length;
    seg->map = (char *)mi_zalloc(seg->length);
    memcpy(seg->map, map + seg->offset, seg->length);

    offset = seg->offset;
    start_offset = seg->offset;
    end_offset = seg->offset + seg->length;

    tls_fingerprint = 0;
    tls_map_t = (unsigned char*)seg->map;
    tls_pre = (unsigned char*)seg->map;
    tls_FING_GEAR_08KB_64 = 0x0000d93003530000ULL;
    for (int i = 0; i < 256; i++) {
        tls_GEARv2_tmp[i] = GEARv2[i];
    }
}

void chunker_1::run(struct bitmap* bm) {
    auto start_1 = std::chrono::high_resolution_clock::now();
    while (offset < end_offset) {
        tls_fingerprint = (tls_fingerprint << 1) + (tls_GEARv2_tmp[tls_map_t[0]]);
        if (!(tls_fingerprint & tls_FING_GEAR_08KB_64)) {
            // bitmap_set(seg->bm, offset - start_offset);
            bitmap_set(bm, offset);
        }
        offset += 1;
        tls_map_t += 1;
    }

    std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - start_1;
    printf("core used,%d\n", sched_getcpu());
    printf("chunker_1 time,%f\n", diff.count());
}

csegment* chunker_1::task(struct bitmap* bm) {
    this->run(bm);
    return seg;
}

void chunker_crc::init(const char *map, uint64_t off_t, uint64_t length) {
    seg = (csegment *)mi_zalloc(sizeof(csegment));
    seg->offset = off_t;
    seg->length = length;
    seg->map = (char *)mi_zalloc(seg->length);
    memcpy(seg->map, map + seg->offset, seg->length);
        
    seg->csums_queue = new DataQueue<one_cdc>();
        
    offset = seg->offset;
    start_offset = seg->offset;
    end_offset = seg->offset + seg->length;
    
    tls_fingerprint = 0;
    tls_map_t = (unsigned char*)seg->map;
    tls_pre = (unsigned char*)seg->map;
    tls_FING_GEAR_08KB_64 = 0x0000d93003530000ULL;
    for (int i = 0; i < 256; i++) {
        tls_GEARv2_tmp[i] = GEARv2[i];
    }
}

void chunker_crc::run() {
    auto start_1 = std::chrono::high_resolution_clock::now();
    DataQueue<one_cdc> *csums_queue = seg->csums_queue;
    uint64_t pre_offset = offset;

    while (offset < end_offset) {
        tls_fingerprint = (tls_fingerprint << 1) + (tls_GEARv2_tmp[tls_map_t[0]]);
        if (!(tls_fingerprint & tls_FING_GEAR_08KB_64)) {
            struct one_cdc cdc = {
                .offset = pre_offset,
                .length = offset - pre_offset,
                .weak_hash = crc32_isal((uint8_t *)tls_pre, offset - pre_offset, 0)
            };
            tls_pre = tls_map_t;
            pre_offset = offset;
            csums_queue->push(cdc);
        }
        offset += 1;
        tls_map_t += 1;
    }

    if (pre_offset < end_offset) {
        struct one_cdc cdc = {
            .offset = pre_offset,
            .length = end_offset - pre_offset,
            .weak_hash = crc32_isal((uint8_t *)tls_pre, end_offset - pre_offset, 0)
        };
        csums_queue->push(cdc);
    }
    csums_queue->setDone();
    
    std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - start_1;
    printf("core used,%d\n", sched_getcpu());
    printf("chunker_crc time,%f\n", diff.count());
}

csegment* chunker_crc::task() {
    this->run();
    return seg;
}

void chunker_1_(csegment *seg) {
    uint64_t offset = seg->offset;
    uint64_t end_offset = seg->offset + seg->length;

    uint64_t fingerprint = 0;
    // char *map_t = (char *)seg->map + offset;
    char *map_t = seg->map;
    uint64_t FING_GEAR_08KB_64 = 0x0000d93003530000;
    uint64_t GEARv2_tmp[256];
    memcpy(GEARv2_tmp, GEARv2, sizeof(GEARv2));

    bitmap *bm = seg->bm;
    uint64_t start_offset = offset;

    clock_t start = 0, end = 0, total = 0;

    while (offset < end_offset) {
        uint8_t *p = (uint8_t *) map_t;
        fingerprint = (fingerprint << 1) + (GEARv2_tmp[p[0]]);
        if ((!(fingerprint & FING_GEAR_08KB_64))) {
            bitmap_set(bm, offset - start_offset);
        }
        offset += 1;
        map_t += 1;
    }
}

void chunker_crc_(csegment *seg) {
    uint64_t offset = seg->offset;
    struct file_cdc *fc = seg->fc;

    uint64_t chunk_num = 0;
    uint64_t end_offset = seg->offset + seg->length;
    uint64_t fingerprint = 0;
    clock_t start = 0, end = 0, total = 0;

    fc->cdc_array[0].offset = offset;
    char* map_t = seg->map;
    char* pre = map_t;
    uint64_t FING_GEAR_08KB_64 = 0x0000d93003530000;
    uint64_t GEARv2_tmp[256];
    memcpy(GEARv2_tmp, GEARv2, sizeof(GEARv2));
    
    while (offset < end_offset) {
        // calculate the fingerprint
        uint8_t *p = (uint8_t *) map_t;
        fingerprint = (fingerprint << 1) + (GEARv2_tmp[p[0]]);
    
        if ((!(fingerprint & FING_GEAR_08KB_64))) {
            // bitmap_set(seg->bm, offset);
            fc->cdc_array[chunk_num].length = offset - fc->cdc_array[chunk_num].offset;
            assert(pre + fc->cdc_array[chunk_num].length == map_t);
            fc->cdc_array[chunk_num].weak_hash = crc32_16bytes((uint8_t *)pre, fc->cdc_array[chunk_num].length, 0);
            
            pre = map_t;
            chunk_num += 1;
            fc->cdc_array[chunk_num].offset = offset;
        }
        offset += 1;
        map_t += 1;
    }

    if (fc->cdc_array[chunk_num].offset < end_offset) {
        // bitmap_set(seg->bm, offset);
        assert(offset == end_offset);
        fc->cdc_array[chunk_num].length = offset - fc->cdc_array[chunk_num].offset;
        assert(pre + fc->cdc_array[chunk_num].length == map_t);
        fc->cdc_array[chunk_num].weak_hash = crc32_16bytes((uint8_t *)pre, fc->cdc_array[chunk_num].length, 0);
        chunk_num += 1;
    }
    fc->chunk_num = chunk_num;
}

void chunker_crc_use_rb(csegment *seg) {
    uint64_t offset = seg->offset;
    struct ringbuf_t *rb = ringbuf_create(BUFFER_SIZE);
    struct file_cdc *fc = seg->fc;

    fc->cdc_array = (struct one_cdc *)mi_zalloc(sizeof(struct one_cdc) * (seg->length / MinSize + 2));
    assert(fc->cdc_array != NULL);
    memset(fc->cdc_array, 0, sizeof(struct one_cdc) * (seg->length / MinSize + 2));

    uint64_t chunk_num = 0;
    uint64_t end_offset = seg->offset + seg->length;
    uint64_t fingerprint = 0;

    fc->cdc_array[0].offset = offset;
    uint8_t *p = (uint8_t *) ringbuf_head(rb);
    char *map_t = NULL;
    
    while (offset < end_offset) {
        uint64_t to_read = (end_offset - offset) > BUFFER_SIZE ? BUFFER_SIZE : (end_offset - offset);
        map_t = (char *)seg->map + offset;
        // uint64_t bytes_read = ringbuf_read_from_fd(seg->fd, rb, to_read);
        uint64_t bytes_read = ringbuf_read_from_map(map_t, rb, to_read);

        if (bytes_read <= 0) {
            break;
        }

        // uint8_t *p = (uint8_t *) ringbuf_head(rb);
        while (ringbuf_used(rb) > 0) {
            fingerprint = (fingerprint << 1) + (GEARv2[p[0]]);
            if ((!(fingerprint & FING_GEAR_08KB_64))) {
                bitmap_set(seg->bm, offset);
                fc->cdc_array[chunk_num].length = offset - fc->cdc_array[chunk_num].offset;

                uint8_t tmp_buf[fc->cdc_array[chunk_num].length];
                uint64_t tmp_len = (uint8_t *)ringbuf_end(rb) - (uint8_t *)ringbuf_head(rb);
                if (tmp_len >= fc->cdc_array[chunk_num].length) {
                    memcpy(tmp_buf, ringbuf_head(rb), fc->cdc_array[chunk_num].length);
                } else {
                    memcpy(tmp_buf, ringbuf_head(rb), tmp_len);
                    memcpy(tmp_buf + tmp_len, ringbuf_start(rb), fc->cdc_array[chunk_num].length - tmp_len);
                }
                fc->cdc_array[chunk_num].weak_hash = crc32_16bytes(tmp_buf, fc->cdc_array[chunk_num].length, 0);

                ringbuf_remove(rb, fc->cdc_array[chunk_num].length);
                chunk_num += 1;
                fc->cdc_array[chunk_num].offset = offset;
                // offset += 1;
                
                p = (uint8_t *)ringbuf_head(rb);
                continue;
            }
            offset += 1;
            // ringbuf_remove(rb, 1);
            p += 1;
            if (p == (uint8_t *)ringbuf_end(rb))
                p = (uint8_t *)ringbuf_start(rb);
            if (p == (uint8_t *)ringbuf_tail(rb)) {
                if (offset == end_offset) {
                    // bitmap_set(seg->bm, offset);
                    fc->cdc_array[chunk_num].length = offset - fc->cdc_array[chunk_num].offset;
                    uint8_t tmp_buf[fc->cdc_array[chunk_num].length];
                    uint64_t tmp_len = (uint8_t *)ringbuf_end(rb) - (uint8_t *)ringbuf_head(rb);
                    if (tmp_len >= fc->cdc_array[chunk_num].length) {
                        memcpy(tmp_buf, ringbuf_head(rb), fc->cdc_array[chunk_num].length);
                    } else {
                        memcpy(tmp_buf, ringbuf_head(rb), tmp_len);
                        memcpy(tmp_buf + tmp_len, ringbuf_start(rb), fc->cdc_array[chunk_num].length - tmp_len);
                    }
                    fc->cdc_array[chunk_num].weak_hash = crc32_16bytes(tmp_buf, fc->cdc_array[chunk_num].length, 0);
                    chunk_num += 1;
                }
                break;
            }
        }
    }
}