#include <assert.h>
#include <isa-l/crc.h>
#include "parallel_cdc.h"

off_t file_size(int fd);
void *map_file(int fd);
void unmap_file(int fd, void *map);

void detector_1(std::unique_ptr<csegment*[]>& seg_array, char *map, int thread_num, uint64_t fs, struct bitmap *bm, std::unique_ptr<DataQueue<one_cdc>>& csums_queue) {

    uint64_t chunk_num = 0;
    uint64_t offset = 0;
    uint64_t i = MinSize;

    char *map_t = map;
    
    while (i < fs) {
            if (bitmap_test(bm, i) && (i - offset) >= MinSize) {
                struct one_cdc cdc = {
                    .offset = offset,
                    .length = i - offset,
                    .weak_hash = crc32_isal((uint8_t *)map_t + offset, i - offset, 0)
                };

                // printf("offset: %ld, length: %ld\n", offset, i - offset);
                csums_queue->push(cdc);
                chunk_num += 1;
                offset = i;
                i += MinSize;
            }
            else
                i += 1;
    }

    if (offset < fs) {
        struct one_cdc cdc = {
            .offset = offset,
            .length = fs - offset,
            .weak_hash = crc32_isal((uint8_t *)map_t + offset, fs - offset, 0)
        };
        csums_queue->push(cdc);
        chunk_num += 1;
    }

    csums_queue->setDone();
}

void detector_crc(std::unique_ptr<csegment*[]>& seg_array, char *map, int thread_num, uint64_t fs, std::unique_ptr<DataQueue<one_cdc>>& total_csums_queue) {
    uint64_t chunk_num = 0;
    uint32_t combined_hash = 0;
    uint64_t total_length = 0;
    uint64_t start_offset = 0;
    bool first = true;

    for (int i = 0; i < thread_num; i++) {
        DataQueue<one_cdc> *csums_queue = seg_array[i]->csums_queue;
        
        while (!csums_queue->isDone()) {
            struct one_cdc curr_cdc = csums_queue->pop();

            if (first) {
                start_offset = curr_cdc.offset;
                first = false;
            }

            total_length += curr_cdc.length;
            combined_hash = crc32_comb(combined_hash, curr_cdc.weak_hash, curr_cdc.length);
            
            // Push combined chunk only when size threshold met
            if (total_length >= MinSize) {
                struct one_cdc combined = {
                    .offset = start_offset,
                    .length = total_length,
                    .weak_hash = combined_hash
                };
                total_csums_queue->push(combined);
                chunk_num += 1;

                // Reset for the next chunk
                total_length = 0;
                combined_hash = 0;
                first = true;
            }
        }

        delete csums_queue;
    }

    // Handle any remaining data after all segments processed
    if (total_length > 0) {
        struct one_cdc combined = {
            .offset = start_offset,
            .length = total_length,
            .weak_hash = combined_hash
        };
        total_csums_queue->push(combined);
        chunk_num += 1;
    }

    total_csums_queue->setDone();
}

struct file_cdc* parallel_run_cdc(char *file_path, uint32_t thread_num,
                                std::vector<std::thread> &threads,
                                struct stats *st, int whichone) {

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        printf("open file %s failed\n", file_path);
        exit(1);
    }

    std::unique_ptr<DataQueue<one_cdc>> csums_queue(new DataQueue<one_cdc>());
    std::unique_ptr<thread_args[]> targs((thread_args*)mi_zalloc(sizeof(thread_args) * thread_num));
    std::unique_ptr<csegment*[]> seg_array((csegment**)mi_zalloc(sizeof(csegment*) * thread_num));

    char *map = (char *)map_file(fd);

    fastCDC_init();

    st->cdc_stage_1 = 0;
    st->cdc_stage_2 = 0;

    uint64_t fs = file_size(fd);
    uint64_t seg_size = 0;
    uint64_t last_seg_size = 0;

    if (thread_num == 1) {
        seg_size = fs;
        last_seg_size = fs;
    }
    else if (thread_num > 1) {
        seg_size = fs / thread_num;
        /* the last thread may read more data */
        last_seg_size = seg_size + fs % thread_num;
    }
    else {
        printf("thread_num should be greater than 0\n");
        close(fd);
        exit(1);
    }

    for (uint32_t i = 0; i < thread_num; i++) {
        targs[i].offset = i * seg_size;
        targs[i].length = (i == thread_num - 1) ? last_seg_size : seg_size;
    }

    struct bitmap* bm = bitmap_create(fs);

    auto start = std::chrono::high_resolution_clock::now();

    /*=========================== std::thread ===========================*/
    cpu_set_t cpuset;
    for (uint32_t i = 0; i < thread_num; i++) {
        CPU_ZERO(&cpuset);
        CPU_SET(adj_core_set[i], &cpuset);
        // CPU_SET(cross_core_set[i], &cpuset);
        // CPU_SET(cross_cpu_set[i], &cpuset);

        if (whichone == 1) {
            threads[i] = std::thread([&targs, i, map, bm]() {
                chunker_1 *c1 = new chunker_1();
                c1->init(map, targs[i].offset, targs[i].length);
                csegment* tmp_cseg = c1->task(bm);
                targs[i].seg.store(tmp_cseg);
                delete c1;
            });
        }
        else if (whichone == 2) {
            threads[i] = std::thread([&targs, i, map]() {
                chunker_crc *ccrc = new chunker_crc();
                ccrc->init(map, targs[i].offset, targs[i].length);
                csegment* tmp_cseg = ccrc->task();
                targs[i].seg.store(tmp_cseg);
                delete ccrc;
            });
        }
        else {
            printf("Invalid chunker function\n");
            exit(1);
        }
        int rc = pthread_setaffinity_np(threads[i].native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            printf("Error calling pthread_setaffinity_np: %d\n", rc);
        }
    }

    for (auto &t : threads) {
        t.join();
    }

    for (int i = 0; i < thread_num; i++) {
        seg_array[i] = targs[i].seg.load();
    }

    /*=========================== std::thread ===========================*/

    std::chrono::duration<double> diff_1 = std::chrono::high_resolution_clock::now() - start;
    printf("Parallel CDC Stage 1,%f\n", diff_1.count());

    start = std::chrono::high_resolution_clock::now();
    if (whichone == 1) {
        detector_1(seg_array, (char*) map, thread_num, fs, bm, csums_queue);
    }
    else if (whichone == 2) {
        detector_crc(seg_array, (char*) map, thread_num, fs, csums_queue);
    }
    
    std::chrono::duration<double> diff_2 = std::chrono::high_resolution_clock::now() - start;
    printf("Serial CDC Stage 2,%f\n\n", diff_2.count());

    for(int i = 0; i < thread_num; i++) {
        if (seg_array[i])
            mi_free(seg_array[i]->map);
    }

    bitmap_destroy(bm);
    unmap_file(fd, map);
    close(fd);
    return nullptr;
}