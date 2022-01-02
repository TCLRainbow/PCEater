#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <future>
#include <cmath>
#include <omp.h>
#include "flags.h"
#include <boost/compute/core.hpp>
#include <boost/compute/algorithm/transform.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/algorithm/iota.hpp>
#include <boost/compute/algorithm/adjacent_difference.hpp>

using namespace std;
namespace compute = boost::compute;

#define step_factor 8

[[noreturn]] void ram_easy() {
    uint64_t i = 0;
    vector<uint64_t> v;

    while (true) {
        i++;
        v.push_back(i);
        printf("Address of %lu is 0x%x\n", i, &v[i]);
    }
}

[[noreturn]] void ram_normal() {
    uint64_t i = 0;
    vector<array<uint64_t, 200000>> v;

    while (true) {
        v.push_back(*new array<uint64_t ,200000> {i});
        printf("Address of %lu is 0x%x\n", i, &v[i]);
        i++;
    }
}

void print_time(const chrono::time_point<chrono::system_clock> &t, const string& msg) {
    auto allocate_time = chrono::high_resolution_clock::now();
    auto time = (allocate_time - t) / chrono::milliseconds(1);
    printf("%s: %ldms\n", msg.c_str(), time);
}

long cpu_hard(uint32_t size, const string& id) {
    auto start_time = chrono::high_resolution_clock::now();
    vector<uint32_t[25000]> frame(size);
    uint32_t i = 0;

    string id_s = "[" + id + "] ";
    cout << id_s << "Creating table..." << endl;
    for (auto & row : frame) {
        for (uint32_t & px : row) {
            px = i;
            i++;
        }
    }
    print_time(start_time, id_s + "Allocation time");

    cout << id_s << "Flipping table..." << endl;
    for (auto & row : frame) {
        reverse(begin(row), end(row));
    }
    reverse(frame.begin(), frame.end());

    print_time(start_time, id_s + "Table flipped. Time taken");
    auto allocate_time = chrono::high_resolution_clock::now();
    return (allocate_time - start_time) / chrono::milliseconds(1);
}

void multi_cpu_hard(uint32_t height, uint8_t cores) {
    uint16_t size = height / cores;
    uint8_t remainder = height % cores;
    future<long> jobs[cores];
    char id_s[2];

    for (uint8_t i = 0; i < remainder; i++) {
        sprintf(id_s, "%d", i);
        jobs[i] = async(cpu_hard, size + 1, id_s);
    }
    for (uint8_t i = remainder; i < cores; i++) {
        sprintf(id_s, "%d", i);
        jobs[i] = async(cpu_hard, size, id_s);
    }

    long results[cores];
    for (uint8_t i = 0; i < cores; i++) {
        results[i] = jobs[i].get();
    }
    long total = 0;
    for (long l : results) total += l;
    printf("Average execution time: %ldms", total / cores);
}

void cpu_ram_extreme() {
    const uint32_t size = 250000;

    struct Row {uint64_t px[size];};
    vector<Row> frame;
    uint64_t i = 0;

    cout << "Your PC may lag, like HUGE lag spikes. Enter something to continue.";
    char x;
    cin >> x;

    for (uint32_t j = 0; j < size; j++) {
        Row r{};
        for (uint64_t & k : r.px) {
            i++;
            k = i;
        }
        printf("Address of %lu: 0x%x\n", i, &frame[i-1]);
        frame.push_back(r);
    }
}

uint16_t get_benchmark_target() {
    cout << endl <<"Note: For this option, the larger integer you input, the longer it takes to finish this benchmark, "
                   "but it is also more accurate because it actually stresses your system."<< endl;
    cout << "I don't recommend you to enter something >5000." << endl;
    cout << "Here are some examples for multi-core : 128-> ~100s, 5000-> ~1000-1100s" << endl;
    uint16_t ans;
    do {
        cout << "Please enter the target ms to pass a test (128-65535): ";
        string input;
        cin >> input;
        ans = atoi(input.c_str());
    } while (ans < 128 || ans > 65535);
    return ans;
}

uint32_t benchmark(const string& t, const uint16_t& target) {
    long elapsed;
    uint32_t size = 6000;
    uint16_t step = 65535;
    bool run = true;
    uint16_t limit = target - step_factor * 16;

    while (run) {
        auto start_time = chrono::high_resolution_clock::now();

        vector<double> v(size);
        for (uint32_t i = 0; i < size; ++i) {
            v[i] = i*i;
        }
        while (v.size() > 1) {
            for (uint64_t d = 0; d < v.size() - 1; d++) v[d] += v[d+1];
            v.pop_back();
        }
        v[0] = sqrt(v[0]);
        elapsed = (chrono::high_resolution_clock::now() - start_time) / chrono::milliseconds(1);
        uint64_t count = (size * (size - 1)) >> 1;

        printf("%sSize %d (0x%X), count %lu (0x%lX). Step: %d (0x%X), Time: %ldms, Step limit: %dms.\n",
               t.c_str(), size, size, count, count, step, step, elapsed, limit);

        if (elapsed < limit & step < 65535) {
            cout << t << "Stepping up..." << endl;
            step <<= 1;
            limit -= step_factor;
        } else if (elapsed > target) {
            if (step == 1) {
                run = false;
                size -= 2;
            } else {
                cout << t << "Stepping down..." << endl;
                size -= step;
                step >>= 1;
                limit += step_factor;
            }
        }
        size += step;
    }
    cout << t << "Score: " << size << endl;
    return size;
}

BOOST_COMPUTE_FUNCTION(double, square, (double x),
{
   return x*x;
});

uint32_t cl_benchmark(const compute::device& gpu, const uint16_t& target) {
    printf("This GPU has %d Compute Units.\n", gpu.compute_units());
    compute::context ctx(gpu);
    compute::command_queue queue(ctx, gpu);
    long elapsed;
    uint32_t size = 6000;
    uint16_t step = 1024;
    bool run = true;
    uint16_t limit = target - step_factor * 16;

    while (run) {
        auto start_time = chrono::high_resolution_clock::now();
        compute::vector<double> v(size, ctx);
        compute::iota(v.begin(), v.end(), 1, queue);
        compute::transform(  // b^2 and c^2 in one function
                v.begin(), v.end(), v.begin(), square, queue
        );
        for (uint32_t temp_size = size; temp_size > 1; temp_size--) {
            compute::adjacent_difference( // b^2 + c^2
                    v.begin(), v.end(), v.begin(), compute::plus<double>(), queue
            );
            v.erase(v.begin(), queue);
        }
        compute::transform( // sqrt(a)
                v.begin(), v.end(), v.begin(), compute::sqrt<double>(), queue
        );
        print_time(start_time, "Done");
        cout << size << endl;
        size += step;
        auto allocate_time = chrono::high_resolution_clock::now();
        auto time = (allocate_time - start_time) / chrono::milliseconds(1);
        run = time < target;
    }
    return size;
}

void display_cl_dev() {
    cout << "Detecting OpenCL devices..." << endl;
    uint8_t i = 0;
    for (auto & device : compute::system::devices()) {
        i++;
        printf("%d. %s | %s | %s\n", i, device.vendor().c_str(), device.name().c_str(), device.version().c_str());
    }
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "openmp-use-default-none"
int main() {
    const uint8_t cores = thread::hardware_concurrency();
    cout << "©2021 Dim. All rights reserved." << endl
    << "PCEater v1.3 compiled with gcc " << __VERSION__ << " " << PCEaterFlags << endl
    << "Using Boost " << BOOST_VERSION / 100000 << "." << BOOST_VERSION / 100 % 1000 << "." << BOOST_VERSION % 100
    << ", OpenCL " << CL_TARGET_OPENCL_VERSION / 100 << "." << CL_TARGET_OPENCL_VERSION / 10 % 10 << " via Boost.Compute" << endl
    << "Your CPU has " << to_string(cores) << " cores. PLEASE OPEN TASK MANAGER->Performance WHEN TRYING THIS PROGRAM!" << endl;
    while (true) {
        cout << endl << "-------------------------------------------------------" << endl
        << "RAM usage experiments:" << endl
        << "1. 1 core spamming 8 bytes with 0 each time (A few MB/s)" << endl
        << "2. 1 core spamming 200kB with 0 each time (Max RAM + A few MB/s page file)" << endl
        << "3. 1 core create a 250k x 250k integer table (Max 1 core + Max RAM + Max page file)" << endl
        << "-------------------------------------------------------" << endl
        << "CPU usage experiments:" << endl
        << "4. 1 core create a 25kx25k integer table, flip it horizontally then vertically. (Max 1 core, ~1.3s)" << endl
        << "5. All cores create a 25kx25k integer table, flip it horizontally then vertically. (Max all cores, ~0.94s)" << endl
        << "6. 1 core create a 25kx100k integer table, flip it horizontally then vertically. (Max 1 core, 10GB RAM, ~5.1s)" << endl
        << "7. All cores create a 25kx100k integer table, flip it horizontally then vertically. (Max all cores, 10GB RAM, ~3.7s)" << endl
        << "-------------------------------------------------------" << endl
        << "Dim's Pascal Pythagoras theorem benchmark" << endl
        << "8. Single core" << endl
        << "9. All cores (std::async)" << endl
        << "10. All cores (OpenMP)" << endl
        << "11. OpenCL device (usually GPU)" << endl
        << endl << "Please enter an option: ";

        string input;
        cin >> input;

        switch (atoi(input.c_str())) {
            case 1:
                ram_easy();
            case 2:
                ram_normal();
            case 3:
                cpu_ram_extreme();
                break;
            case 4:
                cpu_hard(25000, "");
                break;
            case 5:
                multi_cpu_hard(25000, cores);
                break;
            case 6:
                cpu_hard(100000, "");
                break;
            case 7:
                multi_cpu_hard(100000, cores);
                break;
            case 8:
                benchmark("", get_benchmark_target());
                break;
            case 9: {
                uint16_t target = get_benchmark_target();

                future<uint32_t> jobs[cores];
                auto start_time = chrono::high_resolution_clock::now();
                for (uint8_t i = 0; i < cores; i++) {
                    ostringstream oss;
                    oss << i + 1;
                    jobs[i] = async(benchmark, "[" + oss.str() + "] ", target);
                    oss.clear();
                }
                uint32_t total = 0;
                uint32_t scores[cores];
                for (uint8_t i = 0; i < cores; i++) {
                    scores[i] = jobs[i].get();
                    total += scores[i];
                }
                cout << "=============Scores=============" << endl;
                for (uint8_t i = 0; i < cores; i++) printf("[%d]: %d\n", i+1, scores[i]);
                printf("Total score: %d. Average: %d ", total, total / cores);
                print_time(start_time, "Total time elapsed");
                break;
            }
            case 10: {
                uint16_t target = get_benchmark_target();
                uint32_t scores[cores];
                auto start_time = chrono::high_resolution_clock::now();
                #pragma omp parallel for
                for (uint32_t & score : scores) score = benchmark("[" + to_string(omp_get_thread_num()+1) + "] ", target);
                cout << "=============Scores=============" << endl;
                uint32_t total = 0;
                for (uint16_t i = 0; i < cores; i++) {
                    total += scores[i];
                    printf("[%d: %d\n", i+1, scores[i]);
                }
                printf("Total score: %d. Average: %d ", total, total / cores);
                print_time(start_time, "Total time elapsed");
                break;
            }
            case 11: {
                uint16_t target = get_benchmark_target();
                display_cl_dev();
                uint8_t option;
                do {
                    cout << "Please enter the ID of GPU that you want to benchmark: ";
                    string gpu_id;
                    cin >> gpu_id;
                    option = atoi(gpu_id.c_str()) - 1;
                } while (option >= compute::system::device_count());

                uint32_t score = cl_benchmark(compute::system::devices()[option], target);
                cout << score << endl;
                break;
            }
            case 12: {
                compute::vector<double> test(100000);
                compute::iota(test.begin(), test.end(), 1);

            }
            default:
                cout << "I don't understand that." << endl << endl;
        }
    }
}
#pragma clang diagnostic pop