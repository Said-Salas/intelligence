#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ ! -d dreamerv3/dreamerv3 ]]; then
  echo "Run: git submodule update --init --recursive"
  exit 1
fi

python dreamerv3/dreamerv3/main.py \
  --logdir "$ROOT/logdir/verify" \
  --configs crafter debug \
  --run.steps 5000 \
  --jax.platform cpu \
  --run.train_ratio 8
