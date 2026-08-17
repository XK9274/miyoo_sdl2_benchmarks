#!/bin/bash
# Pulls a render_suite geometry A/B log off the device (dropbear has no
# scp/sftp, so this reads the remote file over ssh and writes it locally).
# Usage: ./utility/pull_bench_logs.sh <tag> <remote_app_dir> [device_ip]
#   e.g. ./utility/pull_bench_logs.sh neon /mnt/SDCARD/App/sdl_bench_neon
set -euo pipefail

TAG="${1:?Usage: $0 <tag> <remote_app_dir> [device_ip]}"
REMOTE_APP_DIR="${2:?Usage: $0 <tag> <remote_app_dir> [device_ip]}"
DEVICE_IP="${3:-192.168.1.78}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LOCAL_LOG_DIR="${SCRIPT_DIR}/../logs/ab"
mkdir -p "$LOCAL_LOG_DIR"

REMOTE_LOG="${REMOTE_APP_DIR}/logs/render_suite_geometry_${TAG}.log"
LOCAL_LOG="${LOCAL_LOG_DIR}/render_suite_geometry_${TAG}.log"

echo "Pulling ${REMOTE_LOG} from ${DEVICE_IP}..."
sshpass -p 'onion' ssh "onion@${DEVICE_IP}" "cat '${REMOTE_LOG}'" > "$LOCAL_LOG"
echo "Saved to ${LOCAL_LOG}"

echo "--- [BENCH] lines ---"
grep '\[BENCH\]' "$LOCAL_LOG" || echo "(none found -- run may not have started RS_BENCH_TAG or hasn't reached 2s yet)"
