
#include <optional>
#include <iostream>
#include "bench.hpp"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

std::optional<Options> ParseOptions(int argc, char** argv) {
    Options options;

    try {
        // options
        po::options_description desc("HTTP benchmarking tool. Submit HTTP requests at a fixed rate for a duration of time");
        desc.add_options()
            ("help",                                                    "produce help message")
            ("address",     po::value<std::string>(&options.address),   "HTTP address")
            ("qps",         po::value<int>(&options.qps),               "queries per second")
            ("duration",    po::value<int>(&options.duration)->default_value(5),"duration of test in seconds. default=5")
            ("warmup",      po::value<int>(&options.warmup)->default_value(0),  "duration of warmup in seconds")
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

int main(int argc, char** argv) {
    // Parse cmd line args
    auto maybeOptions = ParseOptions(argc, argv);
    if (maybeOptions == std::nullopt) {
        return 1;
    }
    auto options = maybeOptions.value();
    auto benchmarkstats = RunHttpBenchmark(options);

    // printing results out
    std::cout << "Submitted " << benchmarkstats.results.size() << " requests over " 
            << benchmarkstats.submissionTimeMs / 1000.0 << " seconds at " << options.qps << " qps\n";
    std::cout << "Total submit + receive time: " 
            << (benchmarkstats.submissionTimeMs + benchmarkstats.waitTimeMs) / 1000.0 << " seconds\n";
    std::cout << "Avg request roundtrip latency: " << benchmarkstats.avgLatencyMs << "ms\n";
    std::cout << "Success rate: " << 100 * benchmarkstats.successRate << "%\n";

    return 0;
}
