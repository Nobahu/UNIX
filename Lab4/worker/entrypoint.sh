#!/bin/bash
echo "Starting Flask API"
gunicorn -w 2 -b 0.0.0.0:5000 app:app --access-logfile - --error-logfile - &
sleep 3
echo "Starting Kafka Consumer"
python worker.py &
