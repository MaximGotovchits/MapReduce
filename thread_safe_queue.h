#ifndef Task_Queue_thread_safe_queue_h
#define Task_Queue_thread_safe_queue_h

#include <queue>
#include <thread>
#include <atomic>

template <typename T>
class thread_safe_queue
{
private:
    std::mutex queue_mtx;
    std::queue<T> local_queue;
    bool shutdown;
    std::condition_variable not_empty_cond_var;
public:
    thread_safe_queue(): shutdown(false) { }
    
    bool Empty() {
        return local_queue.empty();
    }
    
    void Push(T value)
    {
        std::lock_guard<std::mutex> guard(queue_mtx);
        if(!shutdown) { // Нельзя класть, если позвали Shutdown().
            local_queue.push(std::move(value));
            not_empty_cond_var.notify_one(); // Если очередь была пуста, то есть ждущие потоки. Разбудим один их них при добавлении элемента.
        } else {
            throw std::runtime_error("Unable to add new element. Queue is blocked.\n"); // Бросим runtime_error, если попытаться добавить элемент в заблокированнную очередь.
        }
    }
    
    bool Pop(T& value) {
        std::unique_lock<std::mutex> pop_lock(queue_mtx);
        while (local_queue.empty()) {
            if (shutdown) { // Чтобы не ждать вечно.
                return false;
            }
            not_empty_cond_var.wait(pop_lock);
        }
        value = std::move(local_queue.front());
        local_queue.pop();
        return true;
    }
    
    bool Pop() {
        std::unique_lock<std::mutex> pop_lock(queue_mtx);
        while (local_queue.empty()) { // Можно извлекать, даже если вызвали Shutdown().
            if (shutdown) { // Чтобы не ждать вечно.
                return false;
            }
            not_empty_cond_var.wait(pop_lock);
        }
        local_queue.pop();
        return true;
    }
    
    bool TryPop() {
        std::unique_lock<std::mutex> pop_lock(queue_mtx);
        if (!local_queue.empty()) {
            local_queue.pop();
            return true;
        }
        return false;
    }
    
    bool TryPop(T& value) {
        std::unique_lock<std::mutex> pop_lock(queue_mtx);
        if (!local_queue.empty()) {
            value = std::move(local_queue.front());
            local_queue.pop();
            return true;
        }
        return false;
    }
    
    void Shutdown() {
        std::lock_guard<std::mutex> shut_down_lock(queue_mtx);
        shutdown = true;
        not_empty_cond_var.notify_all();
    }
};

#endif