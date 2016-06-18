#ifndef quick_sort_parallel_parallel_sort_h
#define quick_sort_parallel_parallel_sort_h

#include <thread>

const size_t SEQUENTIAL_SORT_SIZE_THRESHOLD = 100000; // Длина подмассива, для которого необходимо вызвать последовательный алгоритм.

template <typename T>
class sort_task {
private:
    std::vector<T>* data;
    task_queue<std::vector<T>>* tasks;
    const size_t decomposition_level;
    
    size_t partition(size_t left_index, size_t right_index) {
        T x = (*data)[left_index];
        size_t i = left_index;
        for (size_t j = left_index + 1; j < right_index; ++j) {
            if((*data)[j] <= x) {
                ++i;
                std::swap((*data)[i], (*data)[j]);
            }
        }
        std::swap((*data)[i], (*data)[left_index]);
        return i;
    }
       
public:
    sort_task(std::vector<T>& data_set, task_queue<std::vector<T>>& task_set) : decomposition_level(SEQUENTIAL_SORT_SIZE_THRESHOLD) {
        data = &data_set;
        tasks = &task_set;
    }
    
    std::vector<T>& operator() (size_t left_index, size_t right_index) {
        size_t pivot = 0;
        if (left_index < right_index) {
            if (right_index - left_index <= decomposition_level) {
                sort(data->begin() + left_index, data->begin() + right_index);
                return *data;
            }
            pivot = partition(left_index, right_index);
            //usleep(100);
            std::function<std::vector<T>()> left_task = std::bind(*this, left_index, pivot);
            std::future<std::vector<T>> left_future = tasks->Submit(left_task);
            
            (*this)(pivot + 1, right_index);
            tasks->ActiveWait(std::move(left_future));

        }
        return *data;
    }
};

#endif
