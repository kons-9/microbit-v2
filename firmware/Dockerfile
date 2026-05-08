FROM ubuntu:latest

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi build-essential cmake wget unzip \
    && rm -rf /var/lib/apt/lists/*

ARG ZIP_PASSWORD
WORKDIR /work

COPY . .

RUN cd kernel && \
    wget -q https://www.personal-media.co.jp/book/tw/data/362_mbit_mtk3.zip && \
    unzip -P "${ZIP_PASSWORD}" 362_mbit_mtk3.zip && \
    rm 362_mbit_mtk3.zip

RUN make build