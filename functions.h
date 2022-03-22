#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#include <chrono>
#include <thread>
#include <limits>
#include <QEventLoop>

unsigned int factorial(int n)
{
    QEventLoop elp;
    elp.processEvents();
    int res = 1;
    for(auto i = n; i > 1; --i)
    {
        res *= i;
        if(res > std::numeric_limits<unsigned int>::max())
        {
            return 0;
        }
    }
    return res;
}

unsigned int sleep_function(int n)
{
    QEventLoop elp;
    elp.processEvents();
    std::this_thread::sleep_for(std::chrono::seconds(n));
    return 1;
}

unsigned int fibbonaci_sequence(int n)
{
    QEventLoop elp;
    elp.processEvents();
    if(n <= 1)
        return 1;
    else
        return fibbonaci_sequence(n-1) + fibbonaci_sequence(n-2);
}

unsigned int double_factorial(unsigned int n)
{
    QEventLoop elp;
    elp.processEvents();
    // замена на итеративную версию, т.к. нужно контролировать
    // выход за пределы диапазона
    /*
    if (n == 0 || n==1)
      return 1;
    return n * double_factorial(n-2);
    */
    unsigned int fact = 1;
    while (n > 1)
    {
        fact *= n;
        n -= 2;
        if(fact > std::numeric_limits<unsigned int>::max())
        {
            return 0;
        }
    }
    return fact;
}

#endif // FUNCTIONS_H
