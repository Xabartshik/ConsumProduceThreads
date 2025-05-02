#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <queue>
#include <vector>
using namespace std;
/*
The mutex class is a synchronization primitive that can be used to protect shared data from being simultaneously accessed by multiple threads.
mutex offers exclusive, non-recursive ownership semantics:
A calling thread owns a mutex from the time that it successfully calls either lock or try_lock until it calls unlock.
When a thread owns a mutex, all other threads will block (for calls to lock) or receive a false return value (for try_lock) if they attempt to claim ownership of the mutex.
A calling thread must not own the mutex prior to calling lock or try_lock.
*/
mutex mtx;
/*
std::condition_variable is a synchronization primitive used with a std::mutex
to block one or more threads until another thread both modifies a shared variable (the condition) and notifies the std::condition_variable.
*/
condition_variable cv;
//Атомарная переменная для проверки, работает ли поток
atomic<bool> running(true);
//Очередь задач
queue<int> taskQueue;
int totalSum = 0;

void producer() {
    while (running) {
        int input;
        cin >> input;

        if (input == 0) {
            running = false;
            //Оповещает все потоки. Используется для завершения работы
            cv.notify_all();
            break;
        }

        {
            unique_lock<mutex> lock(mtx);
            taskQueue.push(input);
            cv.notify_one();
        }
    }
}

void consumer(int id) {
    while (running || !taskQueue.empty()) {
        unique_lock<mutex> lock(mtx);
        //Переменная, блокирующая выполнение кода (мьютекс)
        /*
        std::condition_variable is a synchronization primitive used with a std::mutex to block one or more threads until another thread both modifies a shared variable (the condition) and notifies the std::condition_variable.

        The thread that intends to modify the shared variable must:

        Acquire a std::mutex (typically via std::lock_guard).
        Modify the shared variable while the lock is owned.
        Call notify_one or notify_all on the std::condition_variable (can be done after releasing the lock).
        Even if the shared variable is atomic, it must be modified while owning the mutex to correctly publish the modification to the waiting thread.

        https://en.cppreference.com/w/cpp/thread/condition_variable
        */
        //Потоки ожидают изменения состояния wait
        cv.wait(lock, [] { return !running || !taskQueue.empty(); });

        //Если задач нет и программа не работает - выходим
        if (!running && taskQueue.empty()) break;

        if (!taskQueue.empty()) {
            int currentNumber = taskQueue.front();
            taskQueue.pop();
            lock.unlock();

            // Имитация обработки
            this_thread::sleep_for(chrono::seconds(2));

            // Обновление суммы (используем мьютекс, чтобы заблокировать доступ)
            lock.lock();
            totalSum += currentNumber;
            cout << "Consumer " << id << ": Added " << currentNumber
                << ", Total sum: " << totalSum << endl;
            lock.unlock();
            //Разблокирует после выполнения кода потоком, позволяя другим потокам обновить значения
        }
    }
}

int main() {
    cout << "Enter numbers (0 to stop):" << endl;

    thread producerThread(producer);
    vector<thread> consumerThreads;
    int numConsumers = 3;

    for (int i = 0; i < numConsumers; ++i) {
        consumerThreads.push_back(thread(consumer, i));
    }

    producerThread.join();
    for (auto& thread : consumerThreads) {
        thread.join();
    }

    cout << "Final sum: " << totalSum << endl;
    return 0;
}