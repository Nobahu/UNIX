from flask import Flask, request, jsonify
import ctypes
import os

app = Flask(__name__)

try:
    lib = ctypes.CDLL('/app/libcrypto.so')
    
    # Используем функции напрямую из algorithm.cpp
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

@app.route('/hash', methods=['POST'])
def hash_text():
    if not CPP_AVAILABLE:
        return jsonify({'error': 'C++ library not available'}), 500
    
    data = request.get_json()
    text = data.get('text', '')
    algorithm = data.get('algorithm', 'md5')
    
    try:
        if algorithm == 'md5':
            result = lib.md5_hash(text.encode()).decode()
        elif algorithm == 'sha3_256':
            result = lib.sha3_256_hash(text.encode()).decode()
        elif algorithm == 'murmur2':
            result = lib.murmur2_hash(text.encode()).decode()
        else:
            return jsonify({'error': f'Unknown algorithm: {algorithm}'}), 400
        
        return jsonify({
            'algorithm': algorithm,
            'hash': result,
            'text': text,
            'engine': 'c++',
            'worker': os.uname().nodename
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/algorithms', methods=['GET'])
def list_algorithms():
    return jsonify({
        'algorithms': ['md5', 'sha3_256', 'murmur2'],
        'cpp_available': CPP_AVAILABLE
    })

@app.route('/health', methods=['GET'])
def health():
    return jsonify({
        'status': 'ok',
        'worker': os.uname().nodename,
        'cpp': CPP_AVAILABLE
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
