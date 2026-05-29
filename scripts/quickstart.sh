#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ ! -d dreamerv3/dreamerv3 ]]; then
  echo "Missing submodule. Run:"
  echo "  git submodule update --init --recursive"
  exit 1
fi

if [[ ! -x .venv/bin/python ]]; then
  echo "Missing .venv. Run:"
  echo "  uv venv .venv --python 3.12"
  echo "  source .venv/bin/activate"
  echo "  uv pip install -r requirements.txt"
  exit 1
fi

MODE="${1:-crafter}"
LOGDIR="$ROOT/logdir/${MODE}-$(date +%Y%m%d-%H%M%S)"

case "$MODE" in
  smoke)
    TASK=dummy_disc
    CONFIGS="debug"
    STEPS=2000
    ENVS=2
    ;;
  crafter)
    TASK=crafter_reward
    CONFIGS="crafter,debug"
    STEPS=50000
    ENVS=4
    ;;
  crafter-long)
    TASK=crafter_reward
    CONFIGS="crafter,debug"
    STEPS=200000
    ENVS=4
    ;;
  *)
    echo "Usage: $0 [smoke|crafter|crafter-long]"
    exit 1
    ;;
esac

echo "Mode:     $MODE"
echo "Task:     $TASK"
echo "Logdir:   $LOGDIR"
echo "Steps:    $STEPS"
echo ""
echo "Metrics:  tail -f $LOGDIR/metrics.jsonl"
echo "Viewer:   python -m scope.viewer --basedir $ROOT/logdir --port 8000"
echo ""

.venv/bin/python dreamerv3/dreamerv3/main.py \
  --logdir "$LOGDIR" \
  --configs $CONFIGS \
  --task "$TASK" \
  --run.steps "$STEPS" \
  --run.envs "$ENVS" \
  --jax.platform cpu \
  --jax.prealloc False \
  --logger.outputs jsonl,scope
