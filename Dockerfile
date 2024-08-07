ARG PYTHON_IMG_TAG=3.11-slim-bookworm
FROM python:$PYTHON_IMG_TAG AS builder

WORKDIR /usr/src/s2geometry

ENV PYTHONDONTWRITEBYTECODE 1
ENV PYTHONUNBUFFERED 1

RUN apt-get update
RUN apt-get install -y git libssl-dev build-essential clang cmake swig
RUN pip install --upgrade pip
RUN pip install cmake_build_extension wheel

# Install Googletest
RUN cd /tmp && \
    git clone https://github.com/google/googletest.git && \
    cd googletest && \
    git checkout v1.12.0 && \
    mkdir build && \
    cmake -S /tmp/googletest -B /tmp/googletest/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_GMOCK=ON && \
    cmake --build /tmp/googletest/build --target install

# Install Abseil
RUN cd /tmp && \
    git clone https://github.com/abseil/abseil-cpp.git && \
    cd abseil-cpp && \
    git checkout 20240116.2 && \
    mkdir build && \
    cmake -S /tmp/abseil-cpp -B /tmp/abseil-cpp/build -DCMAKE_CXX_STANDARD=17 -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DCMAKE_PREFIX_PATH=/usr -DCMAKE_INSTALL_PREFIX=/usr -DABSL_ENABLE_INSTALL=ON \
      -DABSL_BUILD_TESTING=ON -DABSL_USE_EXTERNAL_GOOGLETEST=ON && \
    cmake --build /tmp/abseil-cpp/build && \
    ctest --test-dir /tmp/abseil-cpp/build && \
    cmake --build /tmp/abseil-cpp/build --target install

FROM builder AS final

CMD bash
