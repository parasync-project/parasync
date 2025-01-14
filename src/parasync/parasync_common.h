#ifndef PARASYNC_COMMON_H
#define PARASYNC_COMMON_H

#include <mutex>
#include <condition_variable>
#include <string>
#include <queue>
#include <atomic>

template <typename T>
class DataQueue {
public:
    DataQueue() : done(false) {}

    ~DataQueue() {}

    void push(const T& data) {
        std::unique_lock<std::mutex> lock(mtx);
        queue.push(data);
        cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty() || done; });
        if (queue.empty()) {
            return T(); // Return a default-constructed T instead of throwing an exception
        }
        T data = queue.front();
        queue.pop();
        return data;
    }

    void setDone() {
        std::unique_lock<std::mutex> lock(mtx);
        done = true;
        cv.notify_all();
    }

    bool isDone() const {
        std::unique_lock<std::mutex> lock(mtx);
        return done && queue.empty();
    }
    
    size_t size() const {
        std::unique_lock<std::mutex> lock(mtx);
        return queue.size();
    }

    bool empty() const {
        std::unique_lock<std::mutex> lock(mtx);
        return queue.empty();
    }

    void init() {
        done = false;
        while (!queue.empty())
            queue.pop();
    }
private:
    std::queue<T> queue;
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done;
};

#endif // PARASYNC_COMMON_H