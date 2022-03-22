#ifndef THREADPOOL_H
#define THREADPOOL_H

/*
 * https://riptutorial.com/cplusplus/example/15806/create-a-simple-thread-pool
 */

#include <deque>
#include <vector>
#include <mutex>
#include <future>
#include <functional>
#include <thread>
#include <type_traits>
#include <memory>
#include <iostream>
#include <QObject>
#include <tuple>
#include "globals.h"


class ThreadPool : public QObject
{
    Q_OBJECT
    std::mutex m;
    std::mutex& cout_mutex;
    std::mutex& tasks_results_mutex;
    std::condition_variable v;
    std::deque<std::tuple<int,std::packaged_task<unsigned int()>>> work;
    std::vector<std::future<void>> finished;

    void threadTask()
    {
        while(true)
        {
            std::packaged_task<unsigned int()> f;
            int cur_task_id;
            {
              std::unique_lock<std::mutex> l(m);
              if (work.empty()){
                v.wait(l,[&]{return !work.empty();});
              }
              f = std::move(std::get<1>(work.front()));
              cur_task_id = std::get<0>(work.front());
              work.pop_front();
              std::cout << "Thread with id starts its work" << std::this_thread::get_id() << std::endl;
              std::cout << "Work queue size is: " << work.size() << std::endl;

            }
            if (!f.valid())
            {
                {
                    std::lock_guard<std::mutex> lg(cout_mutex);
                    std::cout << "Function is invalid" << std::endl;
                    std::cout << "Non valid function: Work queue size is: " << work.size() << std::endl;
                }
                return;
            }
            emit updateStatusOfTask(std::make_tuple(cur_task_id,SYS::enum_status::RUNNING));
            auto start = std::chrono::system_clock::now();
            f();
            auto diff = std::chrono::system_clock::now() - start;
            emit updateResultOfTask(cur_task_id,std::chrono::duration<double,std::milli>(diff).count());
//            //отправка сообщения об успешном завершении всех задач
//            {
//                std::unique_lock<std::mutex> l(m);
//                if(work.size() == 0)
//                {
//                    {
//                        std::lock_guard<std::mutex> lg(cout_mutex);
//                        std::cout << "All tasks completed" << std::endl;
//                    }
//                    emit allTasksHaveBeenCompleted();
//                }
//            }
        }
    }

public:
    ThreadPool(std::mutex& c_mutex, std::mutex& tasks_res_mutex, QObject* parent = nullptr);
    ~ThreadPool(){ finishTasks(); }

     int getWorkQueueSize()
     {
         std::unique_lock<std::mutex> l(m);
         return work.size();
     }

     bool taskInQueue(int task_id)
     {
         std::unique_lock<std::mutex> l(m);
         for(auto itr = work.begin();itr != work.end();++itr)
         {
             auto t_id = std::get<0>(*itr);
             if(task_id == t_id)
             {
                 {
                    std::unique_lock<std::mutex> ul(cout_mutex);
                    std::cout << "Queue contains task id is: " << task_id;
                 }
                 v.notify_all();
                 return true;
             }
         }
         v.notify_all();
         return false;
     }

     void cancelTask(int task_id)
     {
         std::unique_lock<std::mutex> l(m);
         for(auto itr = work.begin();itr != work.end();)
         {
             auto t_id = std::get<0>(*itr);
             if(task_id == t_id)
             {
                 {
                    std::unique_lock<std::mutex> ul(cout_mutex);
                    std::cout << "Cancelled task id is: " << task_id;
                 }
                 itr = work.erase(itr);
                 emit updateStatusOfTask(std::make_tuple(task_id,SYS::enum_status::CANCELLED));
             }
             else
             {
                 ++itr;
             }
         }
         v.notify_all();
     }

    template<class T>
    std::future<unsigned int> addTask(T&& f)
    {
      auto task = std::get<1>(f);
      std::packaged_task<unsigned int()> p(std::move(task));
      auto r=p.get_future();
      {
        std::unique_lock<std::mutex> l(m);
        work.emplace_back(std::make_tuple(std::get<0>(f),std::move(p)));
      }
      v.notify_one();
      return r;
    }

    // Запуск потоков с выбранными задачами. Каждый поток - своя задача
    void runThreads(std::size_t N=1)
    {
      for (std::size_t i = 0; i < N; ++i)
      {
        finished.push_back(
          std::async(
            std::launch::async,
            [this]{ threadTask(); }
          )
        );
      }
    }

    void cancelNonStartedTasks()
    {
      std::unique_lock<std::mutex> l(m);
      for(auto&& cancelled_task : work)
      {
          auto task_id = std::get<0>(cancelled_task);
          std::cout << "Cancelled task id is: " << task_id;
          emit updateStatusOfTask(std::make_tuple(task_id,SYS::enum_status::CANCELLED));
      }
      work.clear();
    }

    void finishTasks()
    {
      {
        std::unique_lock<std::mutex> l(m);
        for(auto&&unused:finished){
          work.push_back({});
        }
        {
            std::lock_guard<std::mutex> lg(cout_mutex);
            std::cout << "Finish tasks: Work queue size is: " << work.size() << std::endl;
        }
        v.notify_all();
      }
      finished.clear();
    }

    void abortTasks()
    {
      cancelNonStartedTasks();
      finishTasks();
    }
signals:
    void updateStatusOfTask(std::tuple<int,SYS::enum_status>);
    void updateResultOfTask(int,double);
    void allTasksHaveBeenCompleted();

};


#endif // THREADPOOL_H

