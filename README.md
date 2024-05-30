# HTTP load/benchmarking tool

## Features

 - Run queries at a fixed rate for a specified duration
 - Measure and calculate latencies
 - Separate library (`bench.h` + `bench.cpp`) files as well as a CLI (main.cpp)

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
