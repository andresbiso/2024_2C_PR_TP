FROM rhub/ubuntu-gcc:latest

LABEL mantainer="Diego"

# update and install dependencies
RUN apt-get update \
    && apt-get install -y \
    software-properties-common \
    wget \
    git \
    curl \
    netcat \
    strace \
    iproute2 \
    hping3 \
    gdb \
    net-tools \
    vim \
    htop \
    dnsutils

WORKDIR /home

RUN mkdir -p workspace

VOLUME /home/workspace

# enable strace with docker
# --cap-add=SYS_PTRACE
