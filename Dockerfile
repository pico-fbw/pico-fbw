FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive
ENTRYPOINT ["/bin/bash"]

# Install prerequisites
RUN \
    apt update && \
    apt install -y git python3 && \
    apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
    
# Install Pico SDK
ARG SDK_PATH=/usr/share/pico_sdk
RUN git clone --depth 1 --branch 1.5.1 https://github.com/raspberrypi/pico-sdk $SDK_PATH && \
    cd $SDK_PATH && \
    git submodule update --init
ENV PICO_SDK_PATH=$SDK_PATH

# Set up project
WORKDIR /app
COPY . /app/
