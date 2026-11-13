#!/usr/bin/env bash
# 真实包用例：eigen 整数矩阵乘（逐位）+ 双精度矩阵乘（容差）。
set -euo pipefail
: "${STAGE:=run}" "${SIDE:=A}" "${BIN:=build/native}" "${DATA:=build/ab-data}" "${OUT:=build/ab-data/out}"
mkdir -p "$DATA" "$OUT"
case "$STAGE" in
  gen) : > "$DATA/eigen-input.txt" ;;
  run) "$BIN/eigen_harness" "$OUT/eigen.bin" > "$OUT/hash.txt" ;;
  *) echo "unknown STAGE=$STAGE" >&2; exit 2 ;;
esac
