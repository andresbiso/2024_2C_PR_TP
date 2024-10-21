FROM rhub/ubuntu-gcc:latest

LABEL mantainer="Diego"

# update and install dependencies
RUN apt-get update \
    && apt-get install -y \
    software-properties-common \
    curl \
    dnsutils \
    gdb \
    git \
    hping3 \
    htop \
    iproute2 \
    lsof \
    netcat \
    net-tools \
    strace \
    telnet \
    valgrind \
    vim \
    wget

WORKDIR /home

RUN mkdir -p workspace

VOLUME /home/workspace

# enable strace with docker
# --cap-add=SYS_PTRACE
