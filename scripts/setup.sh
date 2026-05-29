#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v uv >/dev/null 2>&1; then
  echo "Installing uv..."
  curl -LsSf https://astral.sh/uv/install.sh | sh
  export PATH="$HOME/.local/bin:$PATH"
fi

if [[ ! -d dreamerv3/dreamerv3 ]]; then
  git submodule update --init --recursive
fi

uv venv .venv --python 3.12
uv pip install -r requirements.txt

echo "Done. Next:"
echo "  source .venv/bin/activate"
echo "  ./scripts/crafter.sh play      # try Crafter yourself first"
echo "  ./scripts/crafter.sh train     # Dreamer learns Crafter (~50k steps)"
echo "  ./scripts/crafter.sh watch     # charts at http://localhost:8000"
