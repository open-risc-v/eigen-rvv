# RVV 编码笔记 —— `dev-cpp/eigen`

> 每次踩坑写一条：**现象 / 原因 / 正确写法 / 怎么发现的**。这份文件是交接文档。

## 通用纪律（沿用 libspeex-rvv 的结论，先照做再质疑）

1. **`vsetvl` 放循环内**，用 `__riscv_vsetvl_e<sew>m<lmul>(avail)`；不要把 `vlmax` 当 `vl` 用。
   尾部靠它收敛，循环外设一次是典型错误。
2. **`vmerge` 操作数序**是 `(mask, false_val, true_val, vl)`——顺序写反不会报错，只会安静地错。
3. **`vredsum` 参数序**是 `(dst_carrier, vector, scalar_init, vl)`，结果在 `vs1[0]`。
4. **尾部策略**：`_tu`（tail-agnostic）与 `ta` 混用会在"整寄存器写回内存"时写入脏 lane；
   用 `vle/vse` 显式带 `vl` 最安全。
5. **`vbool*` 类型宽度随 LMUL 变**（m1→`vbool8_t`，m8→`vbool1_t`），跨 LMUL 传 mask 必须显式转换。
6. **数组不能存向量类型**：`vint16m1_t a[4]` 会强制溢出到内存，用多个局部变量展开。
7. **`vlse` 的 stride 是字节**，不是元素数；且实测比 `vle` 慢 3–5 倍，能重排内存就先重排。

## 本包特有

（示例格式，实际填写时删掉本行）

```
现象  : 某内核在 VLEN=256 下正确，在 VLEN=128 下尾块错
原因  : 误用 vsetvlmax_e8m8 固定 vl，尾块多算
写法  : while (i<n) { vl = vsetvl_e8m1(n-i); ... }
发现  : 毒化矩阵 vlen=128 跑 diff_kernels
```

## 待查清单

- [ ] 无 RVV 目标上的 dispatch 回退（`qemu-riscv64 -cpu rv64` 无 v 扩展）
- [ ] VLEN 128 / 256 / 512 三档毒化
- [ ] `-march` 不含 `zvl*` 时行为一致

## 真机 A/B 复跑与后端真相（2026-11-25）

门禁入口本来就是 `test/cases/01_eigen.sh` → `$BIN/eigen_harness`，板子上直接可用：

```
cmake -S . -B build/ab-a -DSRC_DIR=$PWD/src -D_USE_RVV=ON -DRVV_MARCH=rv64gcv
cmake -S . -B build/ab-b -DSRC_DIR=$PWD/src -D_USE_RVV=OFF          # 同一份头，只切 -march
RVV_A=$PWD/build/ab-a RVV_B=$PWD/build/ab-b bash test/run_ab.sh
```

结果 `PASS 00_framework`、`PASS 01_eigen`（整数 64 阶矩阵乘逐位一致，双精度 128 阶在容差内一致）。

**但这不算 Eigen 自己的 RVV 后端跑起来了**。`Eigen/src/Core/util/ConfigureVectorization.h:532` 要求显式定义 `EIGEN_RISCV64_USE_RVV10`，而打开后又硬性要求 `__riscv_v_fixed_vlen`（即 `-mrvv-vector-bits=zvl`），否则 `#error`；gcc 15.2 不支持 `-mrvv-vector-bits`。所以当前 A/B 的差别只是 GCC 对 Eigen 标量循环的自动向量化。要让 Eigen 的 RVV10 内核真正生效，得换支持固定 VLEN 的编译器（clang 或更高版本 gcc），这条在 gcc 15.2 上走不通。
