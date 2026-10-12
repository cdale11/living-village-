#!/bin/sh
set -eu
VILLAGE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PORTS_DIR=$(CDPATH= cd -- "$VILLAGE_DIR/.." && pwd)
exec "$VILLAGE_DIR/bin/living-village" --register-port-art "$PORTS_DIR"
