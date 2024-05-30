FROM ubuntu:focal

RUN apt update && apt install -y libcurl4-openssl-dev clang libboost-program-options-dev

WORKDIR /app
COPY . .

RUN clang++ -std=c++17 bench.cpp -I/usr/include/aarch64-linux-gnu/ -L/usr/lib/aarch64-linux-gnu/ -lcurl -o bench
ENTRYPOINT ["./bench"]