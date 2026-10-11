#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
PYTHONPATH=src exec python3 -m living_village.main "$@"
