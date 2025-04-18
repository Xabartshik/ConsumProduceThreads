#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <queue>
#include <vector>
using namespace std;

mutex mtx;
condition_variable cv;
atomic<bool> running(true);
queue<int> taskQueue;
int totalSum = 0;

void producer() {
    while (running) {
        int input;
        cin >> input;

        if (input == 0) {
            running = false;
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
        cv.wait(lock, [] { return !running || !taskQueue.empty(); });

        if (!running && taskQueue.empty()) break;

        if (!taskQueue.empty()) {
            int currentNumber = taskQueue.front();
            taskQueue.pop();
            lock.unlock();

            // Имитация обработки
            this_thread::sleep_for(chrono::seconds(2));

            // Обновление суммы
            lock.lock();
            totalSum += currentNumber;
            cout << "Consumer " << id << ": Added " << currentNumber
                << ", Total sum: " << totalSum << endl;
            lock.unlock();
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