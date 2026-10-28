#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
if [[ ! -x ./gepetas ]]; then ./build.sh; fi
exec ./gepetas "$@"
