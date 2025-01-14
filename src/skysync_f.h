#ifndef SKYSYNC_F_H
#define SKYSYNC_F_H

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <zlib.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>

#include "uthash.h"
#include "crc32.h"
#include "fastcdc.h"
#include "ring_buffer.h"

/* ===============Checksums=============== */

// #define DEBUG 1
// #define IO_PRINT 1

struct one_csum {
	uint32_t chunk_num;
    uint32_t weak_csum;
	uint8_t csum[SHA256_DIGEST_LENGTH];
};

struct crr_csums {
    uint32_t f_size;
    uint64_t chunk_nums;
    struct one_csum *all_csums;
};

struct crr_ol {
    uint32_t offset;
    uint32_t length;
};

struct crr_matched_item {
    uint32_t item_nums;
    uint8_t hash[SHA256_DIGEST_LENGTH];
    struct crr_ol old_ol;
    uint32_t* new_chunk_nums;
};

struct crr_unmatched_item {
    struct crr_ol old_ol;
    struct crr_ol new_ol;
};

struct crr_matched_chunks {
    /* record all the matched chunks' offsets and lengths array, and then the sha1 */
    uint64_t matched_chunks_num;
    struct crr_ol *matched_item_array;
    /* record all the unmatched chunks' offsets and lengths array */
    uint64_t unmatched_chunks_num;
    struct crr_unmatched_item *unmatched_item_array;
};

struct crr_uthash {
    uint32_t weak_csum;
    struct one_csum *csum;
    uint64_t item_nums;
    uint8_t remalloc;
    UT_hash_handle hh;
};

void crr_build_uthash(struct crr_uthash **hash_table, struct crr_csums *new_csums,
                        uint64_t chunk_nums);

void crr_delete_uthash(struct crr_uthash **hash_table);

void crr_print_uthash(struct crr_uthash **hash_table);

/* ===============Calculate Checksums=============== */

uint8_t* crr_calc_sha256(uint8_t *buf, uint32_t len);

uint8_t* crr_calc_blake2bp(uint8_t *buf, uint32_t len);

uint8_t* crr_calc_blake3(uint8_t *buf, uint32_t len);

uint32_t crr_calc_crc32c(uint8_t *buf, uint32_t len);

struct crr_csums* crr_calc_csums_1(int fd, char *map);

struct crr_csums* crr_calc_csums_2(int fd, char *map);

/* ===============Write and Read Checksums=============== */

void crr_write_csums(char *file_path, struct crr_csums *csums, struct stats *st);

struct crr_csums* crr_read_csums_new(char *file_path, struct stats *st);

struct crr_csums* crr_read_csums_old(char *file_path, char *file_csums, struct stats *st);

/* ==================Compare Checksums================== */

void crr_direct_compare(struct crr_csums *old_csums, struct crr_csums *new_csums);

int find_uthash_item(struct crr_uthash *s, uint8_t *csum);

void crr_compare_csums_1(int new_fd, char *delta_file_path, struct crr_csums *old_csums, struct crr_csums *new_csums);

void crr_compare_csums_2(char *new_file_path, char *delta_file_path, struct crr_csums *old_csums, struct crr_csums *new_csums);

int crr_patch_delta(char *old_file_path, char* delta_file_path, char* out_file_path);

#endif // SKYSYNC_F_H
