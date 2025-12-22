# Stage 1: Builder - compile fsm with static Boost
FROM ubuntu:24.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Download and build Boost statically
RUN wget -q https://boostorg.jfrog.io/artifactory/main/release/1.83.0/source/boost_1_83_0.tar.gz && \
    tar -xzf boost_1_83_0.tar.gz && \
    cd boost_1_83_0 && \
    ./bootstrap.sh --prefix=/usr/local/boost-static && \
    ./b2 link=static runtime-link=static install && \
    cd .. && \
    rm -rf boost_1_83_0 boost_1_83_0.tar.gz

# Copy project files
COPY CMakeLists.txt fsm.cpp /build/

# Create build directory and compile with static linking
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBOOST_ROOT=/usr/local/boost-static \
          -DBoost_USE_STATIC_LIBS=ON \
          -DBoost_USE_STATIC_RUNTIME=ON \
          .. && \
    make

# Stage 2: Runtime - run fsm and Python postprocessing
FROM ubuntu:24.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages
RUN pip3 install --no-cache-dir \
    numpy \
    matplotlib

# Set working directory
WORKDIR /app

# Copy compiled executable from builder
COPY --from=builder /build/build/fsm /app/

# Copy source files and scripts
COPY fsm.cpp CMakeLists.txt /app/
COPY plot_log.py /app/

# Make fsm executable
RUN chmod +x /app/fsm

# Default command: run fsm and then generate plot
CMD ["/bin/bash", "-c", "./fsm && python3 plot_log.py log.csv -o log.png --dpi 200 && ls -lh log.*"]
