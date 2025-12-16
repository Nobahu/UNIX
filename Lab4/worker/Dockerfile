FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y g++ libssl-dev
COPY algorithm.cpp .
RUN g++ -fPIC -shared -o libcrypto.so algorithm.cpp -lssl -lcrypto

FROM python:3.9-slim
WORKDIR /app
COPY --from=builder libcrypto.so .
COPY requirements.txt .
RUN pip install --no-cache-dir flask gunicorn
COPY app.py .
EXPOSE 5000
CMD ["gunicorn", "-w", "4", "-b", "0.0.0.0:5000", "app:app"]
