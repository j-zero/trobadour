FROM ubuntu:22.04 as cross-base
ENV DEBIAN_FRONTEND=noninteractive
FROM cross-base as build
RUN apt-get update && apt-get install --assume-yes --no-install-recommends build-essential git curl wget dialog cmake pkg-config glib-2.0 ssh
RUN mkdir -p /root/src/toolchain
RUN wget --no-check-certificate https://developer.arm.com/-/media/Files/downloads/gnu/11.3.rel1/binrel/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-elf.tar.xz -O /tmp/toolchain.xz
RUN tar -xf /tmp/toolchain.xz -C /root/src/toolchain
ENV PATH="$PATH:/root/src/toolchain/arm-gnu-toolchain-11.3.rel1-x86_64-aarch64-none-elf/bin/"
#ADD ./mt32-pi /root/src/mt32-pi
#RUN git -c http.sslVerify=false clone --recursive https://github.com/j-zero/mt32-pi.git /root/src/mt32-pi
#RUN cd /root/src/mt32-pi; make all
