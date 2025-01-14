#ifndef PARALLEL_CDC_H
#define PARALLEL_CDC_H

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
#include <vector>
#include <thread>
#include <pthread.h>
#include <barrier>
#include <omp.h>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <photon/photon.h>
#include <photon/thread/std-compat.h>
#include <photon/common/alog.h>
#include <photon/fs/localfs.h>
#include <photon/thread/workerpool.h>
#include <photon/thread/thread11.h>
#include <mimalloc-2.1/mimalloc.h>

#include "skysync_f.h"
#include "skysync_c.h"
#include "ring_buffer.h"
#include "chunker.h"

static uint8_t adj_core_set[32] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                              16, 17, 18, 19, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
static uint8_t cross_core_set[32] = {0, 40, 1, 41, 2, 42, 3, 43, 4, 44, 5, 45, 6, 46, 7, 47,
                              8, 48, 9, 49, 10, 50, 11, 51, 12, 52, 13, 53, 14, 54, 15, 55};
static uint8_t cross_cpu_set[32] = {0, 20, 1, 21, 2, 22, 3, 23, 4, 24, 5, 25, 6, 26, 7, 27,
                              8, 28, 9, 29, 10, 30, 11, 31, 12, 32, 13, 33, 14, 34, 15, 35};

void chunker_1_(csegment *seg);

void chunker_crc_(csegment *seg);

void chunker_crc_use_rb(csegment *seg);

struct bitmap* handle_segs_bm(csegment *seg_array, int thread_num);

void detector_1(std::unique_ptr<csegment*[]>& seg_array, char *map, int thread_num, uint64_t fs, struct bitmap *bm,  std::unique_ptr<DataQueue<one_cdc>>& csums_queue);

struct file_cdc* handle_segs_fc(csegment *seg_array, int thread_num);

void detector_crc(std::unique_ptr<csegment*[]>& seg_array, char *map, int thread_num, uint64_t fs, std::unique_ptr<DataQueue<one_cdc>>& csums_queue);

struct file_cdc* parallel_run_cdc(char *file_path, uint32_t thread_num,
                                std::vector<std::thread> &threads,
                                struct stats *st, int whichone);

#endif // PARALLEL_CDC_H