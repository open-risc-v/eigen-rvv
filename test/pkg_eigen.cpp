/* eigen 探针：整数矩阵乘（逐位精确 + 标量参照定性）+ 双精度矩阵乘（对 long double 参考校误差）。
   整数路径用于跨架构逐字节比对；浮点路径输出最大相对误差供容差判定。
   计时写 stderr，stdout 的哈希行在两侧可比。 */
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <time.h>
#include "Eigen/Dense"

static unsigned long long fnv1a(const void *buf, size_t n)
{
	const unsigned char *p = (const unsigned char *)buf;
	unsigned long long h = 0xcbf29ce484222325ULL;
	for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 0x100000001b3ULL; }
	return h;
}

static double now_ms(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec * 1000.0 + (double)t.tv_nsec / 1e6;
}

int main(int argc, char **argv)
{
	if (argc < 2) { std::fprintf(stderr, "usage: %s <out.bin> [reps]\n", argv[0]); return 2; }
	int reps = argc > 2 ? std::atoi(argv[2]) : 1;
	if (reps < 1) reps = 1;
	FILE *f = std::fopen(argv[1], "wb");
	if (!f) { std::perror("fopen"); return 2; }

	// 1) 整数 64x64：A*B，结果逐位确定
	Eigen::Matrix<int, 64, 64> A, B, C;
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++) {
			A(i, j) = (i * 7 + j * 13) % 97 - 48;
			B(i, j) = (i * 11 - j * 5) % 89 - 44;
		}
	C = A * B;

	// 标量参照：定性用（mismatch 数与首个差异位置）
	int bad = 0, first_i = -1, first_j = -1, first_got = 0, first_ref = 0;
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++) {
			int ref = 0;
			for (int k = 0; k < 64; k++) ref += A(i, k) * B(k, j);
			if (ref != C(i, j)) {
				bad++;
				if (first_i < 0) { first_i = i; first_j = j; first_got = C(i, j); first_ref = ref; }
			}
		}

	std::fwrite(C.data(), sizeof(int), (size_t)C.size(), f);
	std::printf("imatmul 64x64 fnv=%016llx mismatch=%d/%d",
	            fnv1a(C.data(), sizeof(int) * (size_t)C.size()), bad, 64 * 64);
	if (bad) std::printf(" first=(%d,%d) got=%d ref=%d", first_i, first_j, first_got, first_ref);
	std::printf("\n");

	// 2) 双精度 128x128：X*X^T，与 long double 参考比相对误差
	const int N = 128;
	Eigen::MatrixXd X(N, N);
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			X(i, j) = std::sin(0.017 * (i * N + j)) + 0.3 * std::cos(0.011 * i - 0.007 * j);
	Eigen::MatrixXd G = X * X.transpose();

	double max_rel = 0.0;
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++) {
			long double ref = 0.0L;
			for (int k = 0; k < N; k++) ref += (long double)X(i, k) * (long double)X(j, k);
			long double den = (ref == 0.0L) ? 1.0L : std::fabs(ref);
			double rel = (double)(std::fabs((long double)G(i, j) - ref) / den);
			if (rel > max_rel) max_rel = rel;
		}
	// 量化到 1e-6 再写与取哈希：跨架构末位微差不影响逐字节比对（误差本身单独打印）
	std::vector<double> q((size_t)N * N);
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			q[(size_t)i * N + j] = std::round(G(i, j) * 1e6) / 1e6;
	std::fwrite(q.data(), sizeof(double), q.size(), f);
	std::printf("dmatmul 128x128 max_rel=%.3e fnv=%016llx\n", max_rel,
	            fnv1a(q.data(), sizeof(double) * q.size()));

	// 3) 计时（best-of-reps）
	Eigen::Matrix<int, 64, 64> C2;
	double bi = 1e18, bd = 1e18;
	for (int r = 0; r < reps; r++) {
		double t0 = now_ms(); C2 = A * B; double t1 = now_ms();
		if (t1 - t0 < bi) bi = t1 - t0;
		t0 = now_ms(); G = X * X.transpose(); t1 = now_ms();
		if (t1 - t0 < bd) bd = t1 - t0;
	}
	std::fprintf(stderr, "imatmul64_ms=%.3f dmatmul128_ms=%.3f\n", bi, bd);

	std::fclose(f);
	return (max_rel < 1e-12) ? 0 : 1;
}
