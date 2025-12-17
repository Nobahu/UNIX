from flask import Flask, request, jsonify
from kafka import KafkaProducer
import json
import uuid
import os
import threading
import time

app = Flask(__name__)

# Kafka конфигурация
KAFKA_BROKER = os.getenv('KAFKA_BROKER', 'localhost:9092')
REQUEST_TOPIC = 'hash-requests'

# Ленивая инициализация
producer = None
producer_lock = threading.Lock()
results_cache = {}
cache_lock = threading.Lock()

def get_kafka_producer():
    """Ленивое получение Kafka producer"""
    global producer
    
    with producer_lock:
        if producer is not None:
            return producer
        
        try:
            print(f"Connecting to Kafka at {KAFKA_BROKER}")
            producer = KafkaProducer(
                bootstrap_servers=[KAFKA_BROKER],
                value_serializer=lambda v: json.dumps(v).encode('utf-8'),
                acks=1,
                retries=1,
                request_timeout_ms=3000,
                api_version_auto_timeout_ms=5000
            )
            
            # Быстрая проверка
            future = producer.send(REQUEST_TOPIC, value={'test': 'quick_check'})
            future.get(timeout=2)
            
            print("Kafka connected")
            return producer
            
        except Exception as e:
            print(f"Kafka connection failed: {e}")
            producer = None
            return None

@app.route('/hash', methods=['POST'])
def hash_text():
    data = request.get_json()
    text = data.get('text', '')
    algorithm = data.get('algorithm', 'md5')
    
    if not text:
        return jsonify({'error': 'Text is required'}), 400
    
    # Получаем producer (ленивая инициализация)
    kafka_producer = get_kafka_producer()
    if kafka_producer is None:
        return jsonify({'error': 'Kafka service is unavailable'}), 503
    
    # Генерация ID запроса
    request_id = str(uuid.uuid4())
    
    # Создание сообщения
    message = {
        'request_id': request_id,
        'text': text,
        'algorithm': algorithm,
        'timestamp': time.time()
    }
    
    try:
        # Отправка в Kafka
        future = kafka_producer.send(REQUEST_TOPIC, value=message)
        future.get(timeout=5)
        
        return jsonify({
            'request_id': request_id,
            'status': 'queued',
            'message': 'Request sent to Kafka'
        })
    except Exception as e:
        return jsonify({'error': f'Kafka error: {str(e)}'}), 500

@app.route('/hash/<request_id>', methods=['GET'])
def get_hash_result(request_id):
    """Получение результата из кэша"""
    with cache_lock:
        result = results_cache.get(request_id)
    
    if result:
        return jsonify({
            'status': 'completed',
            'result': result
        })
    else:
        return jsonify({
            'status': 'processing',
            'message': 'Still processing'
        }), 202

@app.route('/result', methods=['POST'])
def receive_result():
    """Endpoint для worker'ов чтобы отправлять результаты"""
    data = request.get_json()
    request_id = data.get('request_id')
    
    if request_id:
        with cache_lock:
            results_cache[request_id] = data
            
            # Очищаем старые результаты
            if len(results_cache) > 1000:
                keys = list(results_cache.keys())
                for key in keys[:100]:
                    del results_cache[key]
        
        print(f"Received result for {request_id}")
        return jsonify({'status': 'received'})
    
    return jsonify({'error': 'No request_id'}), 400

@app.route('/algorithms', methods=['GET'])
def list_algorithms():
    return jsonify({
        'algorithms': ['md5', 'sha3_256', 'murmur2'],
        'kafka_broker': KAFKA_BROKER
    })

@app.route('/health', methods=['GET'])
def health():
    kafka_status = 'unknown'
    
    # Быстрая проверка - не ждем отправки сообщения
    try:
        test_producer = KafkaProducer(
            bootstrap_servers=[KAFKA_BROKER],
            request_timeout_ms=2000
        )
        test_producer.close()
        kafka_status = 'healthy'
    except Exception:
        kafka_status = 'connecting'
    
    return jsonify({
        'status': 'ok',
        'kafka': kafka_status,
        'cache_size': len(results_cache),
        'worker': os.uname().nodename,
        'ready': True
    })

# Фоновая очистка старых результатов
def cleanup_old_results():
    while True:
        time.sleep(60)
        current_time = time.time()
        with cache_lock:
            keys_to_delete = []
            for key, value in results_cache.items():
                if current_time - value.get('timestamp', 0) > 600:
                    keys_to_delete.append(key)
            
            for key in keys_to_delete:
                del results_cache[key]
            
            if keys_to_delete:
                print(f"Cleaned up {len(keys_to_delete)} old results")

cleanup_thread = threading.Thread(target=cleanup_old_results, daemon=True)
cleanup_thread.start()

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)
