# Этап 1: Компиляция C++ библиотеки
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    g++ \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY algorithm.cpp .
RUN g++ -fPIC -shared -o libcrypto.so algorithm.cpp -lssl -lcrypto

# Этап 2: Python с Kafka
FROM python:3.9-slim

WORKDIR /app
COPY --from=builder libcrypto.so .

COPY requirements.txt .
RUN pip install --no-cache-dir \
    flask==2.3.3 \
    gunicorn==21.2.0 \
    kafka-python==2.0.2 \
    requests==2.31.0

COPY app.py worker.py entrypoint.sh ./
RUN chmod +x entrypoint.sh

EXPOSE 5000
ENTRYPOINT ["./entrypoint.sh"]
