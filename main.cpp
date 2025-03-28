#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <atomic>

// Structure to hold each traffic data entry
struct TrafficData {
    std::string timestamp;
    std::string lightID;
    int carsPassed;
};

// Thread-safe bounded queue
template <typename T>
class BoundedQueue {
private:
    std::queue<T> q;
    size_t capacity;
    std::mutex mtx;
    std::condition_variable not_full, not_empty;

public:
    BoundedQueue(size_t cap) : capacity(cap) {}

    void push(const T &item) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [&]() { return q.size() < capacity; });
        q.push(item);
        not_empty.notify_one();
    }

    bool pop(T &item) {
        std::unique_lock<std::mutex> lock(mtx);
        if (!not_empty.wait_for(lock, std::chrono::seconds(2), [&]() { return !q.empty(); })) {
            return false;
        }
        item = q.front();
        q.pop();
        not_full.notify_one();
        return true;
    }
};

// Parse CSV line into TrafficData
bool parseLine(const std::string &line, TrafficData &data) {
    std::stringstream ss(line);
    if (!std::getline(ss, data.timestamp, ',')) return false;
    if (!std::getline(ss, data.lightID, ',')) return false;
    std::string cars;
    if (!std::getline(ss, cars)) return false;

    try {
        data.carsPassed = std::stoi(cars);
    } catch (...) {
        return false;
    }
    return true;
}

// Extract hour (YYYY-MM-DD HH) from timestamp
std::string extractHour(const std::string &timestamp) {
    if (timestamp.size() < 13) return timestamp;
    return timestamp.substr(0, 13);
}

// Producer thread function
void producer(BoundedQueue<TrafficData> &queue, const std::string &filepath) {
    std::ifstream file(filepath);
    std::string line;
    while (std::getline(file, line)) {
        TrafficData data;
        if (parseLine(line, data)) {
            queue.push(data);
        }
    }
}

// Header for each hour's report
void printHeader(const std::string &hour) {
    std::cout << "\n📊 Hourly Traffic Congestion Report — " << hour << ":00\n";
    std::cout << "--------------------------------------------------------\n";
    std::cout << "   Rank  | Traffic Light | Cars Passed\n";
    std::cout << "--------------------------------------------------------\n";
}

// Print each top light entry
void printEntry(int rank, const std::string &light, int cars) {
    std::cout << std::setw(7) << rank << " | "
              << std::setw(13) << light << " | "
              << std::setw(11) << cars << " cars\n";
}

// Footer separator
void printFooter() {
    std::cout << "--------------------------------------------------------\n";
}

// Consumer thread function
void consumer(BoundedQueue<TrafficData> &queue, int topN, std::atomic<bool> &allDone) {
    std::map<std::pair<std::string, std::string>, int> trafficCounts;
    std::string currentHour;

    while (true) {
        TrafficData data;
        bool gotItem = queue.pop(data);

        if (!gotItem && allDone.load()) break;
        if (!gotItem) continue;

        std::string hour = extractHour(data.timestamp);
        if (currentHour.empty()) currentHour = hour;

        if (hour != currentHour) {
            std::vector<std::pair<std::string, int>> results;
            for (auto &entry : trafficCounts) {
                if (entry.first.second == currentHour) {
                    results.push_back({entry.first.first, entry.second});
                }
            }

            std::sort(results.begin(), results.end(),
                      [](auto &a, auto &b) { return a.second > b.second; });

            printHeader(currentHour);
            for (int i = 0; i < topN && i < (int)results.size(); ++i) {
                printEntry(i + 1, results[i].first, results[i].second);
            }
            printFooter();

            currentHour = hour;
        }

        trafficCounts[{data.lightID, hour}] += data.carsPassed;
    }

    // Final flush of last hour
    if (!currentHour.empty()) {
        std::vector<std::pair<std::string, int>> results;
        for (auto &entry : trafficCounts) {
            if (entry.first.second == currentHour) {
                results.push_back({entry.first.first, entry.second});
            }
        }
        std::sort(results.begin(), results.end(),
                  [](auto &a, auto &b) { return a.second > b.second; });

        printHeader(currentHour);
        for (int i = 0; i < topN && i < (int)results.size(); ++i) {
            printEntry(i + 1, results[i].first, results[i].second);
        }
        printFooter();
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <numProducers> <numConsumers> <dataFile>\n";
        return 1;
    }

    int numProducers = std::stoi(argv[1]);
    int numConsumers = std::stoi(argv[2]);
    std::string dataFile = argv[3];
    int topN = 3;

    BoundedQueue<TrafficData> queue(200);
    std::atomic<bool> allDone(false);

    std::vector<std::thread> producers;
    for (int i = 0; i < numProducers; ++i) {
        producers.emplace_back(producer, std::ref(queue), dataFile);
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < numConsumers; ++i) {
        consumers.emplace_back(consumer, std::ref(queue), topN, std::ref(allDone));
    }

    for (auto &p : producers) p.join();

    std::this_thread::sleep_for(std::chrono::seconds(2));
    allDone = true;

    for (auto &c : consumers) c.join();

    std::cout << "\n✅ Traffic simulation completed successfully!\n";
    return 0;
}
