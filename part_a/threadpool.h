#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>

#include <sstream>
#include <iostream>

#define DEBUG 0

class ThreadTask {
    public:
        virtual ~ThreadTask() = default;
        virtual void run() = 0;
        virtual int id() = 0;
};


class SingleQueueThreadPool {
    public:
        SingleQueueThreadPool(size_t numThreads = std::thread::hardware_concurrency()) {
            if (numThreads > std::thread::hardware_concurrency()){
                numThreads = std::thread::hardware_concurrency();
            }
            // printf("Creating a thread pool of size %u...\n", numThreads);

            // Create `numThreads` threads with an infinite pool and a stopp signal
            for (int i = 0 ; i < numThreads; i++){
                pool.emplace_back(std::thread([this]{

                    std::ostringstream oss;
                    oss << std::this_thread::get_id();
                    std::string threadid = oss.str();

                    for(;;) {
                        std::unique_lock<std::mutex> _ulock(this->thread_mtx);
                        _cond.wait(_ulock, [this]{ return !this->task_queue.empty() || this->stopped; });

                        if (this->stopped && task_queue.empty())
                            return;

                        auto task = task_queue.front();
                        if (DEBUG)
                            printf("[Thread-%s] wake up and run task %d\n", threadid.c_str(), task->id());

                        task_queue.erase(task_queue.begin());
                        task->run();
                    }                    
                }));
            }
        };

        ~SingleQueueThreadPool() {
            {
                std::lock_guard<std::mutex> _lock(thread_mtx);
                stopped = true;                
            }

            _cond.notify_all();

            for (std::thread& t: pool){
                t.join();
            }            
            if (DEBUG)
                printf("thread pool deleted\n");
        }

         void enqueue(std::shared_ptr<ThreadTask> task){
            // Schedule a task on one of the pool threads ()
            {
                std::lock_guard<std::mutex> _lock(thread_mtx);
                task_queue.push_back(task);
            }
            _cond.notify_all();
        };

       
    private:
        std::vector<std::thread> pool;

        std::vector<std::shared_ptr<ThreadTask>> task_queue; // Shared queue across all workers. 
        std::mutex thread_mtx; // protects `task_queue`
        std::condition_variable _cond; // message passing and wake up from other thread

        bool stopped;
};



// class PoolThreadwithQueue {
//     public:
//         PoolThreadwithQueue(){};

//         ~PoolThreadwithQueue() {
//             std::lock_guard<std::mutex> _lock(thread_mtx);
//             queue.clear();
//             stopped = true;
//         }

//     void run(){
//         std::thread t = std::thread([this]{             
//                     using namespace std::chrono_literals;
        
//                     while (this->stopped != true){
//                         std::unique_lock<std::mutex> _ulock(this->thread_mtx);
//                         _cond.wait(_ulock, [this]{ return !this->queue.empty(); });


//                         ThreadTask task = queue.front();
//                         printf("[Thread-%d] wake up and run task %d", task.id());
//                         task.run();
//                         queue.erase(queue.begin());
//                     }

//                 });

//         t.detach();        
//     }

//     void enqueue(ThreadTask task){
//             // Schedule a task on one of the pool threads ()
//             std::lock_guard<std::mutex> _lock(thread_mtx);
//             queue.push_back(task);
//             _cond.notify_one();
//         };

//     void stop(){
//         this->stopped = true;
//     }

//     int size(){
//         std::lock_guard<std::mutex> _lock(thread_mtx);
//         return queue.size();
//     }

//     private:
//         std::vector<ThreadTask> queue;
//         std::mutex thread_mtx; // protects `queue`
//         std::condition_variable _cond; // message passing and wake up from other thread

//         bool stopped;
// };


// class MultiQueueThreadPool {
//     public:
//         MultiQueueThreadPool(unsigned int numThreads): numThreads(std::min(numThreads, std::thread::hardware_concurrency())){
//             // Create `numThreads` threads with an infinite pool and a stopp signal
//             for (int i = 0 ; i < numThreads; i++){
//                 PoolThreadwithQueue t = PoolThreadwithQueue();
//                 pool.emplace_back(t);
//                 t.run();
//             }
//         };

//         ~MultiQueueThreadPool() {}
//         void enqueue(IRunnable* task){
//             // pick a pool and assign work
            

//         };

//         void terminate() {
//             for (int i = 0 ; i < numThreads; i++){ 
//               pool.at(i).stop();
//             }
//         }

//     private:
//         std::vector<PoolThreadwithQueue> pool;
//         const unsigned int numThreads;
// };

#endif //_THREADPOOL_H