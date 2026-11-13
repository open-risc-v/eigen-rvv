#!/usr/bin/env bash
# 上游 eigen 是 header-only：不需要编译，把 Eigen/ 与 unsupported/ 拷进前缀即可。
set -euo pipefail
SRC=$1
OUT=$2
rm -rf "${OUT}"
mkdir -p "${OUT}/prefix/include"
cp -R "${SRC}/Eigen" "${OUT}/prefix/include/"
[ -d "${SRC}/unsupported" ] && cp -R "${SRC}/unsupported" "${OUT}/prefix/include/"
echo "== 上游 eigen 头文件就位：${OUT}/prefix/include"
