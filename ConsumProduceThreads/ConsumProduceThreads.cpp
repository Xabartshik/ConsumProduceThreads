#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

using Matrix = std::vector<std::vector<int>>;

std::mutex mtx;
std::condition_variable cv;
Matrix matrixA;
int matrixCount = 1;
std::atomic<bool> running(true);
bool newMatrixNeeded = false;

// Функция для создания матрицы, заполненной заданным числом
Matrix generateMatrixWithNumber(int rows, int cols, int number) {
    Matrix matrix(rows, std::vector<int>(cols));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix[i][j] = number;
        }
    }
    return matrix;
}

// Функция producer - отвечает за чтение ввода пользователя и создание новых матриц
void producer() {
    while (running) {
        char input;
        std::cin >> input;

        if (input == '0') {
            running = false;
            cv.notify_all(); // Уведомляем всех consumers
            break;
        }

        {
            std::unique_lock<std::mutex> lock(mtx);
            matrixCount++;
            newMatrixNeeded = true;
            cv.notify_one(); // Уведомляем одного consumer
        }
    }
}

// Функция consumer - выполняет умножение матриц
void consumer(int id) {
    while (running) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !running || newMatrixNeeded; });

        if (!running) break;

        int currentMatrixCount = matrixCount;
        Matrix matrixB = generateMatrixWithNumber(matrixA[0].size(), matrixA.size(), currentMatrixCount);
        newMatrixNeeded = false; // Сбрасываем флаг после взятия задачи
        lock.unlock();

        // Умножение матриц matrixA и matrixB
        Matrix result(matrixA.size(), std::vector<int>(matrixB[0].size(), 0));
        for (size_t i = 0; i < matrixA.size(); ++i) {
            for (size_t j = 0; j < matrixB[0].size(); ++j) {
                for (size_t k = 0; k < matrixA[0].size(); ++k) {
                    result[i][j] += matrixA[i][k] * matrixB[k][j];
                }
            }
        }

        // Вывод результирующей матрицы с идентификатором consumer
        std::cout << "Consumer " << id << ": Result Matrix (Multiplication " << currentMatrixCount - 1 << "):\n";
        for (const auto& row : result) {
            for (int val : row) {
                std::cout << val << " ";
            }
            std::cout << "\n";
        }

        // Обновляем matrixA. Критическая секция!
        lock.lock();
        matrixA = result;
        lock.unlock();
    }
}

int main() {
    int rows, cols;
    std::cout << "Введите количество строк и столбцов матрицы: ";
    std::cin >> rows >> cols;

    matrixA = generateMatrixWithNumber(rows, cols, 1);

    std::thread producerThread(producer);
    std::vector<std::thread> consumerThreads;
    int numConsumers = 3; // Количество consumer потоков

    for (int i = 0; i < numConsumers; ++i) {
        consumerThreads.push_back(std::thread(consumer, i));
    }

    producerThread.join();
    for (auto& thread : consumerThreads) {
        thread.join();
    }

    return 0;
}