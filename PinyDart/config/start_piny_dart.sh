#!/bin/sh

APP_PATH="/root/Piny_Dart_release/Piny_Dart"
APP_NAME="Piny_Dart"
LOG_FILE="/root/run.log"

echo "=============================="
echo " Piny Dart Startup Script"
echo "=============================="

# 1. Check if binary exists
if [ ! -f "$APP_PATH" ]; then
    echo "[ERROR] Binary not found: $APP_PATH"
    exit 1
fi

# 2. Kill existing process
echo "[INFO] Checking existing processes..."

PIDS=$(ps | grep "$APP_NAME" | grep -v grep | awk '{print $1}')

if [ ! -z "$PIDS" ]; then
    echo "[WARN] Existing process found: $PIDS"
    for PID in $PIDS
    do
        echo "[INFO] Killing process $PID"
        kill -9 $PID
    done
    sleep 1
else
    echo "[INFO] No existing process"
fi

# 3. Clean zombie processes (best effort)
echo "[INFO] Checking zombie processes..."
ZOMBIES=$(ps -el | grep Z | awk '{print $4}')
if [ ! -z "$ZOMBIES" ]; then
    echo "[WARN] Zombie processes detected: $ZOMBIES"
else
    echo "[INFO] No zombie processes"
fi

# 4. Check memory
echo "[INFO] Memory status:"
free -h

# 5. Check CPU load
echo "[INFO] CPU load:"
uptime

# 6. Optional: check UDP port (5000)
PORT=5000
echo "[INFO] Checking UDP port $PORT ..."
netstat -an | grep ":$PORT" > /dev/null
if [ $? -eq 0 ]; then
    echo "[WARN] Port $PORT is in use"
else
    echo "[INFO] Port $PORT is free"
fi

# 7. Sync filesystem
sync

# 8. Start application
echo "[INFO] Starting application..."
chmod +x "$APP_PATH"

# Run in foreground OR background (choose one)

# ---- Option A: foreground (recommended for SSH debug)
# exec "$APP_PATH"

# ---- Option B: background + log
nohup "$APP_PATH" > "$LOG_FILE" 2>&1 &

sleep 1

# 9. Verify startup
NEW_PID=$(ps | grep "$APP_NAME" | grep -v grep | awk '{print $1}')

if [ ! -z "$NEW_PID" ]; then
    echo "[SUCCESS] Started successfully. PID=$NEW_PID"
else
    echo "[ERROR] Failed to start"
    exit 1
fi

echo "=============================="