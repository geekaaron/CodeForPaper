#!/usr/bin/env bash

if [[ $# -lt 2 || $# -gt 3 ]]; then
    echo "Usage: $0 MIN_NODES MAX_NODES [STEP]"
    exit 1
fi

MIN_NODES=$1
MAX_NODES=$2
STEP=${3:-100}

BIN=./rarc-selection-process

ROUND=1

for ((NODES=MIN_NODES; NODES<=MAX_NODES; NODES+=STEP)); do
    echo "========== NODES=${NODES} =========="
    echo "[RUN] ${BIN} ${ROUND} ${NODES}" >> ./rarc-result.txt
    for x in $(seq 1 10); do
        EDGE_FILE=./dataset/edge_${NODES}_${x}.txt

        if [[ ! -f "$EDGE_FILE" ]]; then
            echo "[WARN] Missing file: $EDGE_FILE"
            continue
        fi

        echo "[RUN] ${BIN} ${ROUND} ${NODES} ${EDGE_FILE}"
        ${BIN} ${ROUND} ${NODES} ${EDGE_FILE} >> ./rarc-result.txt
    done
done

