# Stage 1: Builder - compile fsm with static Boost
FROM ubuntu:24.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    wget \
    libboost-all-dev \
 && rm -rf /var/lib/apt/lists/*

## Download and build Boost statically
#RUN wget -q https://boostorg.jfrog.io/artifactory/main/release/1.83.0/source/boost_1_83_0.tar.gz && \
#    tar -xzf boost_1_83_0.tar.gz && \
#    cd boost_1_83_0 && \
#    ./bootstrap.sh --prefix=/usr/local/boost-static && \
#    ./b2 link=static runtime-link=static install && \
#    cd .. && \
#    rm -rf boost_1_83_0 boost_1_83_0.tar.gz

# Set working directory
WORKDIR /build

# Copy project files
COPY CMakeLists.txt fsm.cpp /build/

# Create build directory and compile with static linking
RUN mkdir -p build && cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release \
          -D BOOST_ROOT=/usr/local/boost-static \
          -D Boost_USE_STATIC_LIBS=ON \
          -D Boost_USE_STATIC_RUNTIME=ON \
          .. && \
    make

# Stage 2: Runtime - run fsm and Python postprocessing
FROM ubuntu:24.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    python3 \
    python3-numpy \
    python3-matplotlib \
 && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy compiled executable from builder
COPY --from=builder /build/build/fsm /app/

# Make fsm executable
RUN chmod +x /app/fsm

# Copy source files and scripts
COPY plot_log.py /app/

# Default command: run fsm and then generate plot
CMD ["/bin/bash", "-c", "./fsm && python3 plot_log.py log.csv -o log.png --dpi 200 && ls -lh log.*"]
