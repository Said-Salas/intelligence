#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CMD="${1:-train}"
shift || true

LOGDIR="${LOGDIR:-$ROOT/logdir/crafter-$(date +%Y%m%d-%H%M%S)}"

run_train() {
  local configs="${1:-crafter,debug}"
  local steps="${2:-50000}"
  local envs="${3:-4}"

  if [[ ! -x .venv/bin/python ]]; then
    echo "Run ./scripts/setup.sh first"
    exit 1
  fi

  echo "Task:     crafter_reward"
  echo "Configs:  $configs"
  echo "Steps:    $steps"
  echo "Envs:     $envs"
  echo "Logdir:   $LOGDIR"
  echo ""
  echo "Watch:    ./scripts/crafter.sh watch"
  echo "Metrics:  tail -f $LOGDIR/metrics.jsonl"
  echo ""

  .venv/bin/python dreamerv3/dreamerv3/main.py \
    --logdir "$LOGDIR" \
    --configs "$configs" \
    --task crafter_reward \
    --run.steps "$steps" \
    --run.envs "$envs" \
    --env.crafter.logs True \
    --jax.platform cpu \
    --jax.prealloc False \
    --logger.outputs jsonl,scope \
    "$@"
}

case "$CMD" in
  train)
    run_train "crafter,debug" 50000 4 "$@"
    ;;
  learn)
    run_train "crafter,debug" 200000 4 "$@"
    ;;
  full)
    run_train "crafter,size1m" 1100000 1 "$@"
    ;;
  resume)
    if [[ -z "${1:-}" ]]; then
      echo "Usage: $0 resume /path/to/logdir"
      exit 1
    fi
    LOGDIR="$1"
    shift
    .venv/bin/python dreamerv3/dreamerv3/main.py \
      --logdir "$LOGDIR" \
      --configs crafter,debug \
      --task crafter_reward \
      --env.crafter.logs True \
      --jax.platform cpu \
      --jax.prealloc False \
      --logger.outputs jsonl,scope \
      "$@"
    ;;
  watch)
    .venv/bin/python -m scope.viewer --basedir "$ROOT/logdir" --port 8000
    ;;
  play)
    .venv/bin/python - <<'PY'
import crafter
env = crafter.Env()
env.reset()
env.render()
print("Crafter window open. WASD-style actions via crafter keys in the UI.")
print("Close the window when done.")
import time
while env.window.is_open():
    env.step(env.action_space.sample())
    env.render()
    time.sleep(0.05)
PY
    ;;
  *)
    cat <<EOF
Crafter + Dreamer

  ./scripts/crafter.sh train     # ~50k steps, tiny debug model (~30-60 min CPU)
  ./scripts/crafter.sh learn     # ~200k steps, still debug model
  ./scripts/crafter.sh full      # paper-ish config (slow on CPU; use GPU if you can)
  ./scripts/crafter.sh resume LOGDIR
  ./scripts/crafter.sh watch     # Scope viewer at http://localhost:8000
  ./scripts/crafter.sh play      # Play Crafter yourself (no ML)

Env vars:
  LOGDIR=./logdir/my-run ./scripts/crafter.sh train
EOF
    ;;
esac
