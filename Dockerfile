FROM ubuntu:focal

RUN apt update && apt install -y libcurl4-openssl-dev clang libboost-program-options-dev libasio-dev

WORKDIR /app
COPY . .

RUN clang++ -std=c++17 -O3 -Wall -Werror bench.cpp main.cpp -I/usr/include/aarch64-linux-gnu/ -L/usr/lib/aarch64-linux-gnu/ -lcurl -lpthread -lboost_program_options -o bench
ENTRYPOINT ["./bench"]