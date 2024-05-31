# HTTP load/benchmarking tool

## Features

 - Run queries at a fixed rate for a specified duration
 - Measure and calculate latencies
 - Separate library (`bench.h` + `bench.cpp`) files as well as a CLI (main.cpp)
 - Uses libcurl api under the hood for HTTP calls

## Instructions to run
Build: 
```
docker build . -t fireworks-http
```

Run:
```
docker run fireworks-http --address http://www.google.com --qps 50 --duration 5 --warmup 1
```

Debug inside container's bash environment:
```
docker run --rm -it --entrypoint bash fireworks-http 
```

## TODO

Remaining tasks would be to lower overhead of the test framework as much as possible. 
 - Thread pool: `std::async` implementations all spawn a new thread each time. threadpool to avoid that will improve throughput vastly.
 - Object pool: creating+initializing libcurl objects every query adds tens of microseconds each call. not much but if we really want to pump throughput...

Improving library functionality:
 - Extending to a python wrapper
 - Allowing for multiple HTTP addresses
 - Test throughput of saving data (to disk or other)
