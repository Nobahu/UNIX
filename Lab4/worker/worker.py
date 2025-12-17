from kafka import KafkaConsumer
import json
import ctypes
import os
import requests
import time
import sys

# Загрузка C++ библиотеки
try:
    lib = ctypes.CDLL('/app/libcrypto.so')
    lib.md5_hash.argtypes = [ctypes.c_char_p]
    lib.md5_hash.restype = ctypes.c_char_p
    lib.sha3_256_hash.argtypes = [ctypes.c_char_p]
    lib.sha3_256_hash.restype = ctypes.c_char_p
    lib.murmur2_hash.argtypes = [ctypes.c_char_p]
    lib.murmur2_hash.restype = ctypes.c_char_p
    CPP_AVAILABLE = True
except Exception as e:
    print(f"C++ library error: {e}")
    CPP_AVAILABLE = False

# Конфигурация
KAFKA_BROKER = os.getenv('KAFKA_BROKER', 'kafka:9092')
FLASK_API = os.getenv('FLASK_API', 'http://app:5000')
REQUEST_TOPIC = 'hash-requests'

def wait_for_kafka(max_retries=3, retry_delay=3):
    for attempt in range(max_retries):
        try:
            print(f"Testing Kafka connection (attempt {attempt + 1}/{max_retries})")
            test_consumer = KafkaConsumer(
                bootstrap_servers=[KAFKA_BROKER],
                consumer_timeout_ms=2000,
                api_version_auto_timeout_ms=3000
            )
            test_consumer.topics()
            test_consumer.close()
            print("Kafka is ready!")
            return True
        except Exception as e:
            print(f"Kafka not ready yet: {e}")
            if attempt < max_retries - 1:
                time.sleep(retry_delay)
    
    print("Warning: Starting without Kafka connection")
    return False

def compute_hash(text, algorithm):
    if algorithm == 'md5':
        return lib.md5_hash(text.encode()).decode()
    elif algorithm == 'sha3_256':
        return lib.sha3_256_hash(text.encode()).decode()
    elif algorithm == 'murmur2':
        return lib.murmur2_hash(text.encode()).decode()
    else:
        raise Exception(f"Unknown algorithm: {algorithm}")

def process_message(message):
    request_id = message['request_id']
    text = message['text']
    algorithm = message['algorithm']
    
    print(f"Processing request {request_id} with algorithm {algorithm}")
    
    try:
        if not CPP_AVAILABLE:
            raise Exception("C++ library not available")
        
        start_time = time.time()
        hash_result = compute_hash(text, algorithm)
        processing_time = time.time() - start_time
        
        result = {
            'request_id': request_id,
            'algorithm': algorithm,
            'hash': hash_result,
            'text': text[:50] + ('...' if len(text) > 50 else ''),
            'engine': 'c++',
            'worker': os.uname().nodename,
            'status': 'success',
            'processing_time': round(processing_time, 4),
            'timestamp': time.time()
        }
        
    except Exception as e:
        result = {
            'request_id': request_id,
            'status': 'error',
            'error': str(e),
            'worker': os.uname().nodename,
            'timestamp': time.time()
        }
    
    max_retries = 3
    for attempt in range(max_retries):
        try:
            response = requests.post(
                f"{FLASK_API}/result",
                json=result,
                timeout=10
            )
            
            if response.status_code == 200:
                print(f"Result sent successfully for {request_id}")
                return True
            else:
                print(f"Failed to send result for {request_id}: HTTP {response.status_code}")
                
        except Exception as e:
            print(f"Attempt {attempt + 1} failed for {request_id}: {e}")
            
        if attempt < max_retries - 1:
            time.sleep(2)  # Ждем перед повторной попыткой
    
    print(f"All retries failed for {request_id}")
    return False

def main():

    print(f"Worker starting up...")
    print(f"Kafka broker: {KAFKA_BROKER}")
    print(f"Flask API: {FLASK_API}")
    print(f"CPP available: {CPP_AVAILABLE}")
    
    # Ждем пока Kafka станет доступна
    if not wait_for_kafka():
        print("Exiting due to Kafka unavailability")
        sys.exit(1)
    
    # Создаем consumer
    consumer = KafkaConsumer(
        REQUEST_TOPIC,
        bootstrap_servers=[KAFKA_BROKER],
        value_deserializer=lambda v: json.loads(v.decode('utf-8')),
        group_id='hash-workers-group',
        auto_offset_reset='earliest',
        enable_auto_commit=True,
        max_poll_records=1,
        session_timeout_ms=6000,
        heartbeat_interval_ms=2000
    )
    
    print(f"Consumer started. Listening to {REQUEST_TOPIC}...")
    
    # Обрабатываем сообщения
    try:
        for message in consumer:
            try:
                process_message(message.value)
            except Exception as e:
                print(f"Error processing message: {e}")
    except KeyboardInterrupt:
        print("Shutting down...")
    finally:
        consumer.close()

if __name__ == '__main__':
    main()
