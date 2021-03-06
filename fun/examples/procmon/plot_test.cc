﻿#include "plot.h"
#include <math.h>
#include <stdio.h>
#include <vector>
#include "fun/base/timestamp.h"

int main() {
  std::vector<double> cpu_usage;
  for (int i = 0; i < 300; ++i)
    cpu_usage.push_back(1.0 + sin(pow(i / 30.0, 2)));
  Plot plot(640, 100, 600, 2);
  fun::Timestamp Start(fun::Timestamp::Now());
  const int N = 10000;
  for (int i = 0; i < N; ++i) {
    String png = plot.plotCpu(cpu_usage);
  }
  double elapsed = TimeDifference(fun::Timestamp::Now(), Start);
  printf("%d plots in %f seconds, %f PNG per second, %f ms per PNG\n", N,
         elapsed, N / elapsed, elapsed * 1000 / N);
  String png = plot.plotCpu(cpu_usage);

  FILE* fp = fopen("test.png", "wb");
  fwrite(png.data(), 1, png.size(), fp);
  fclose(fp);
  printf("Image saved to test.png\n");
}
