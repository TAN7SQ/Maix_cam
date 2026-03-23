#!/bin/sh

APP_NAME="Piny_Dart"
APP_PATH="/root/Piny_Dart_release/Piny_Dart"
LOG_FILE="/root/run.log"

echo "=============================="
echo " Piny Dart Stop Script"
echo "=============================="

# 1. Check process

echo "[INFO] Checking running processes..."

PIDS=$(ps | grep "$APP_NAME" | grep -v grep | awk '{print $1}')

if [ -z "$PIDS" ]; then
echo "[INFO] No running process found"
exit 0
fi

echo "[INFO] Found process(es): $PIDS"

# 2. Try graceful shutdown (SIGTERM)

echo "[INFO] Sending SIGTERM..."
for PID in $PIDS
do
kill $PID
done

# Wait for graceful exit

sleep 2

# 3. Check if still running

REMAIN=$(ps | grep "$APP_NAME" | grep -v grep | awk '{print $1}')

if [ ! -z "$REMAIN" ]; then
echo "[WARN] Process still alive, force killing..."
for PID in $REMAIN
do
echo "[INFO] Killing process $PID (SIGKILL)"
kill -9 $PID
done
sleep 1
else
echo "[INFO] Process exited gracefully"
fi

# 4. Final check

FINAL=$(ps | grep "$APP_NAME" | grep -v grep | awk '{print $1}')

if [ -z "$FINAL" ]; then
echo "[SUCCESS] All processes stopped"
else
echo "[ERROR] Some processes still running: $FINAL"
exit 1
fi

# 5. Check port release (optional)

PORT=5000
echo "[INFO] Checking UDP port $PORT ..."
netstat -an | grep ":$PORT" > /dev/null
if [ $? -eq 0 ]; then
echo "[WARN] Port $PORT still in use"
else
echo "[INFO] Port $PORT released"
fi

# 6. Log info

echo "[INFO] Last 10 lines of log:"
if [ -f "$LOG_FILE" ]; then
tail -n 10 "$LOG_FILE"
else
echo "[INFO] No log file found"
fi

echo "=============================="
