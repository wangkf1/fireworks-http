#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <unistd.h>
#include <future>
#include <chrono>
#include <optional>

#include <curl/curl.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace std::chrono;

struct Result {
    bool success;
    int64_t latencyUsecs;
};

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb;
}

Result launchGetRequest(const std::string& address) {
    // todo: object pool to reuse handles. 
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, address.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // perform request, measuring latency
    auto start = high_resolution_clock::now();
    auto ret = curl_easy_perform(curl);
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    // return w/ success and latency
    Result result{.success = ret == CURLE_OK, .latencyUsecs = duration.count()};
    return result;
}

struct Options {
    std::string address;
    int qps;
    int duration;
};

std::optional<Options> ParseOptions(int argc, char** argv) {
    Options options;

    try {
        // options
        po::options_description desc("HTTP benchmarking tool");
        desc.add_options()
            ("help",                                                    "produce help message")
            ("address",     po::value<std::string>(&options.address),   "HTTP address")
            ("qps",         po::value<int>(&options.qps),               "queries per second")
            ("duration",    po::value<int>(&options.duration)->default_value(5),"duration of test in seconds. default=5")
        ;

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                    options(desc).run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << "Usage: options_description [options]\n";
            std::cout << desc;
            return std::nullopt;
        }

        if (!vm.count("address")) {
            std::cerr << "--address is required!\n";
            return std::nullopt;
        }
        if (!vm.count("qps")) {
            std::cerr << "--qps is required!\n";
            return std::nullopt;
        }
    } catch(std::exception& e) {
        std::cerr << "Caught exception while parsing args: " << e.what() << "\n";
        return std::nullopt;
    }
    return options;
}

// usage: ./bench --address <address> --qps <num> [--duration <secs>] [--numhandles <handles>] [--warmupDuration <secs>]
int main(int argc, char** argv) {
    // Parse cmd line args
    auto maybeOptions = ParseOptions(argc, argv);
    if (maybeOptions == std::nullopt) {
        return 1;
    }
    auto options = maybeOptions.value();

    curl_global_init(CURL_GLOBAL_DEFAULT);
    const int64_t usecs = (1e6 / options.qps);
    std::vector<std::future<Result>> futures;
    futures.reserve(options.qps * options.duration);
    auto start = high_resolution_clock::now();
    for (int i = 0; i < options.qps*options.duration; ++i) {
        auto lastClock = high_resolution_clock::now();
        // launch request asynchronously
        futures.push_back(std::async(std::launch::async, launchGetRequest, options.address));

        // figure out how much time is left before submitting the next request
        auto usecsSinceLast = duration_cast<microseconds>(high_resolution_clock::now() - lastClock).count();
        auto usecsLeft = usecs - usecsSinceLast;
        usleep(usecsLeft);
    }
    auto submitStop = high_resolution_clock::now();

    // wait on futures and get results
    int successes = 0;
    int64_t latencySum = 0;
    for (int i = 0; i < futures.size(); ++i) {
        auto &f = futures[i];
        Result result = f.get();
        successes += (int)result.success;
        latencySum += result.latencyUsecs;
    }
    auto stop = high_resolution_clock::now();

    std::cout << "Submitted " << futures.size() << " requests over " 
            << duration_cast<milliseconds>(submitStop - start).count() / 1000.0 << " seconds at " << options.qps << " qps\n";
    std::cout << "Success rate: " << 100 * (double)successes / futures.size() << "%\n";
    std::cout << "Avg request latency: " << (double)latencySum / futures.size() / 1000.0 << "ms\n";
    std::cout << "Total submit + receive time: " << duration_cast<milliseconds>(stop - start).count() / 1000.0 << " seconds\n";

    curl_global_cleanup();
    return 0;
}
