//
// Created by moltma on 10/21/25.
//

#ifndef SWARMULATOR_CPP_THREADSAFEQUEUE_H
#define SWARMULATOR_CPP_THREADSAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

namespace swarmulator {
    template<typename T>
    class ThreadsafeQueue {
    private:
        std::queue<T> queue_;
        std::mutex lock_;
        std::condition_variable not_full_, not_empty_;
        bool stopped_ = false;
        size_t max_size_ = 1024 * 1024 * 32; // the maximum number of tasks to queue before this becomes a blocking queue

    public:
        void push(T item) {
            std::unique_lock lock(lock_); // get the lock
            not_full_.wait(lock, [&] { // wait until there's room in the queue
                return stopped_ || max_size_ == 0 || queue_.size() < max_size_;
            });

            queue_.push(std::move(item)); // add the item
            not_empty_.notify_one(); // let others know the queue is not empty
        }

        bool pop(T& item) {
            std::unique_lock lock(lock_); // grab the lock
            not_empty_.wait(lock, [&] { return !queue_.empty() || stopped_; }); // wait until there's things on the queue, or we want to be done

            // if stopped and empty, return false
            if (queue_.empty()) {
                return false;
            }

            item = std::move(queue_.front());
            queue_.pop();
            not_full_.notify_one(); // let others know the queue is no longer full
            return true;
        }

        std::size_t size() {
            std::unique_lock lock(lock_);
            return queue_.size();
        }

        void stop() {
            std::lock_guard lock(lock_);
            stopped_ = true;
            not_full_.notify_all();
            not_empty_.notify_all();
        }
    };
} // namespace swarmulator

#endif // SWARMULATOR_CPP_THREADSAFEQUEUE_H
