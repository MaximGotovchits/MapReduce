#ifndef Task_Queue_task_queue_h
#define Task_Queue_task_queue_h

#include <future>
#include "thread_safe_queue.h"

template <typename ResultType>
using task_function_type = std::function<ResultType()>;
template <typename ResultType>
using task_input_channel_type = std::promise<ResultType>;
template <typename ResultType>
using task_output_channel_type = std::future<ResultType>;
template <typename ResultType>
using task_type = std::pair<std::function<ResultType()>, std::promise<ResultType>>;

template <typename ResultType>
class task_queue : public thread_safe_queue<ResultType> {
private:
    thread_safe_queue<task_type<ResultType>> pending_tasks;
    std::vector<std::thread> worker_threads;
    
    void push_tasks(const size_t core_amount) {
        for (size_t set_index = 0; set_index < core_amount; ++set_index) {
            worker_threads.push_back(std::thread(&task_queue::execute_tasks, this));
        }
    }
 
    void execute_tasks() {
        task_type<ResultType> next_task;
        while (pending_tasks.Pop(next_task)) {
            try {
                auto return_value = (next_task.first)();
                next_task.second.set_value(return_value);
            }
            catch(std::exception&) {
                next_task.second.set_exception(std::current_exception());
            }
        }
    }
    
public:
    task_queue() {
        unsigned core_amount;
        if (std::thread::hardware_concurrency()) {
            core_amount = std::thread::hardware_concurrency();
        } else {
            core_amount = 2; // Default value.
        }
        push_tasks(core_amount);
    }
    
    task_queue(unsigned core_amount) {
        for (size_t set_index = 0; set_index < core_amount; ++set_index) {
            worker_threads.push_back(std::thread(&task_queue::execute_tasks, this));
        }
    }
    
    task_output_channel_type<ResultType> Submit(task_function_type<ResultType> function_wrapper) {
        task_input_channel_type<ResultType> input_channel;
        task_output_channel_type<ResultType> output_channel = input_channel.get_future();
        task_type<ResultType> current_task(function_wrapper, std::move(input_channel));
        pending_tasks.Push(std::move(current_task));
        return output_channel;
    }
    
    void Shutdown() {
        pending_tasks.Shutdown(); // Теперь в очередь нельзя ничего положить. Ждем пока выполнятся все задания.
    }
    
    void ActiveWait(std::future<ResultType> async_result) {
        std::chrono::milliseconds span(0);
        //while (async_result.wait_for(span) != std::future_status::ready) {
        while (async_result.wait_for(span) == std::future_status::timeout) {
            task_type<ResultType> next_task;
            if (pending_tasks.TryPop(next_task)) {
                try {
                    auto return_value = (next_task.first)();
                    next_task.second.set_value(return_value);
                } catch(std::exception&) {
                    next_task.second.set_exception(std::current_exception());
                }
            } else {
                std::this_thread::yield();
            }
        }
    }
    
    ~task_queue() {
        Shutdown(); // Чтобы избавиться от тех потоков, которые ждут, если Shutdown() не был вызван пользователем.
        for (auto& thread_to_join : worker_threads) {
            thread_to_join.join();
        }
    }
};

#endif
