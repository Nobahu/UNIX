#!/bin/bash

set -eu

DIR="/flock"
LOCKFILE="$DIR/.lock"

mkdir -p "$DIR"
touch "$LOCKFILE"

CONTAINER_ID="container-$(hostname)"
COUNTER=1
echo "container $CONTAINER_ID (PID: $$)"

exec 9>"$LOCKFILE"

while true; do
  FILENAME=""
  flock -x 9
  for i in {001..999}; do
    if [ ! -f "$DIR/file$i" ]; then
      FILENAME="file$i"
      {
      echo "Container: $CONTAINER_ID"
      echo "Number: $COUNTER"
      echo "PID: $$"
      echo "$(date): Created $FILENAME"
      } > "$DIR/$FILENAME"
      break
    fi
  done
  flock -u 9
  if [ -n "$FILENAME" ] && [ -f "$DIR/$FILENAME" ]; then
    sleep 1
    rm -f "$DIR/$FILENAME"
    echo "$(date): $FILENAME deleted"
    COUNTER=$((COUNTER+1))
    sleep 1
  fi
  sleep 1
done




