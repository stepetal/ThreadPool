#include "threadpool.h"

ThreadPool::ThreadPool(std::mutex& c_mutex, std::mutex &tasks_res_mutex, QObject *parent) : QObject(parent),
                                                                                           cout_mutex(c_mutex),
                                                                                           tasks_results_mutex(tasks_res_mutex)
{

}
