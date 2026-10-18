PR #146: central-region cancellation for z >= y
Experiment date: 2026-10-18

Purpose
-------
Compare the original PR implementation with the generalized implementation
at alpha_z = 1, 1.5 and 2, using the complete Gourdon computation of pi(1e20).
Maintainer request:
https://github.com/kimwalisch/primecount/pull/146#issuecomment-6240071138

Source versions
---------------
Original:    45683819431d0c68d5e68a5e79a86756d1ba7439
Generalized: 44ee6ff8ee56e1acddd1431ed784e7e58c6517a7

The generalized version was measured before committing. Its complete source
diff was saved before the runs and checked unchanged afterwards and before
the code commit. This documentation commit does not change the executable.

Environment and protocol
------------------------
AMD Ryzen 9 9950X3D, 16 cores / 32 logical threads; Windows 11.
MinGW-w64 GCC 13.2.0, UCRT target, distributed with Strawberry.
Both builds use Release (-O3), OpenMP, and the same C and C++ flags:
  -Wall -Wextra -pedantic -Werror
D selects AVX512 through runtime dispatch; AC selects libdivide.

Command for each executable:
  primecount.exe 1e20 --status --threads=32 --alpha-z=N

Each of the six configurations has one discarded warm-up and five measured
runs. The runner uses seeded randomized complete blocks (seed 1462026), with
one run of every configuration per round. Runs are serial. No builds or tests
ran concurrently. No CPU affinity/frequency isolation was imposed.

All 36 runs returned pi(1e20) = 2220819602560918840. All timings, including
the slower late runs, are retained. Total is primecount's internal Seconds
value. WallSeconds additionally includes process launch/capture overhead.

Results in seconds
------------------
All stage and total figures below are medians of five measured runs.
Stage medians need not sum to the total median.

Version       alpha_z   A+C      B       D       Total    Total min-max
Original      1          8.932   2.903   11.200   23.166   22.779-23.934
Generalized   1          8.911   2.888   11.203   23.247   22.911-23.763
Original      1.5        8.816   4.336   11.488   24.797   24.310-26.769
Generalized   1.5        8.686   4.405   11.009   24.097   23.595-25.054
Original      2          8.530   5.642   11.038   25.183   25.021-27.266
Generalized   2          8.519   5.798   10.967   25.340   24.861-27.712

The default alpha_y heuristic was used, as in the requested command.
Increasing alpha_z reduces y while leaving z almost constant:

alpha_z   alpha_y   y           z
1         52.899    245535363   245535363
1.5       35.266    163690242   245535363
2         26.449    122765361   245530722

B depends on x and y. Its increased cost across alpha_z settings is associated
with the reduction in y, not an expansion of z. At the same alpha_z, B is
unchanged between implementations; its timing differences illustrate the
measurement variability.

Interpretation
--------------
For this CPU, x and automatic alpha_y rule, alpha_z > 1 provides no advantage
in median total runtime among the tested values. Original alpha_z=1 has the
lowest median, but its advantage over generalized alpha_z=1 is only 0.081 s
(about 0.35%), and their ranges overlap substantially. The original wins three
of the five same-round alpha_z=1 comparisons; the generalized version wins two.
These data support treating the alpha_z=1 implementations as similar in speed.

Generalized alpha_z=1.5 and 2 are respectively 4.02% and 9.38% slower than
original alpha_z=1 in median total time. This is evidence for the maintainer's
integration decision, not a claim about all CPUs, x values or alpha_y choices.

Correctness verification
------------------------
Generalized Release: 69/69 ctest passed; primecount --test passed;
pi(1e18) = 24739954287740860.
Original Release: 69/69 ctest passed.
Generalized Debug: D and DCentral passed with runtime assertions enabled.
No ARM execution or new ARM cross-build was performed for this experiment.
The prescribed MSYS2 UCRT64 tools were unavailable at the standard location;
these results use the existing GCC 13.2.0 distribution identified above.

Files
-----
benchmark-results.csv       All 36 runs; Warmup=True marks discarded warm-ups.
benchmark-summary.csv       Measured medians, ranges and parameter values.
benchmark-environment.json  Machine/compiler metadata and executable hashes.
raw-logs.zip               logs/: exact stdout from each run;
                           verification/: configure and verification logs.
benchmark.ps1              Exact runner used for the campaign.
summarize.ps1              Summary calculation, excluding warm-ups.

The captured metadata contains the original local executable paths. The runner
expects original-build/ and generalized-build/ next to it. To rerun, copy the
runner to an ignored build directory rather than running it in this data folder.
Its newly generated stdout files will be placed in benchmarks/; the archived
stdout files in this commit have been collected in raw-logs.zip under logs/.

Reproduce with PowerShell from the repository root
------------------------------------------------
Adjust the compiler directory if using another installation, and record the
new toolchain rather than treating it as identical to this campaign.

$env:Path = 'C:\Strawberry\c\bin;' + $env:Path
git worktree add --detach build-pr146-original-src 45683819431d0c68d5e68a5e79a86756d1ba7439
if ($LASTEXITCODE -ne 0) { throw 'Original source checkout failed' }
git worktree add --detach build-pr146-generalized-src 44ee6ff8ee56e1acddd1431ed784e7e58c6517a7
if ($LASTEXITCODE -ne 0) { throw 'Generalized source checkout failed' }
foreach ($variant in @('original', 'generalized')) {
  cmake -S "build-pr146-$variant-src" -B "build-pr146-benchmark/$variant-build" -G 'Unix Makefiles' -DCMAKE_MAKE_PROGRAM=C:/Strawberry/c/bin/mingw32-make.exe -DCMAKE_CXX_COMPILER=C:/Strawberry/c/bin/g++.exe -DCMAKE_C_COMPILER=C:/Strawberry/c/bin/gcc.exe '-DCMAKE_CXX_FLAGS=-Wall -Wextra -pedantic -Werror' '-DCMAKE_C_FLAGS=-Wall -Wextra -pedantic -Werror' -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
  if ($LASTEXITCODE -ne 0) { throw 'Configuration failed' }
  cmake --build "build-pr146-benchmark/$variant-build" --parallel 14
  if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
  ctest --test-dir "build-pr146-benchmark/$variant-build" --output-on-failure
  if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
}
Copy-Item doc/benchmarks/d-central-z-generalization-20261018/benchmark.ps1 build-pr146-benchmark/
Copy-Item doc/benchmarks/d-central-z-generalization-20261018/summarize.ps1 build-pr146-benchmark/
& ./build-pr146-benchmark/benchmark.ps1
& ./build-pr146-benchmark/summarize.ps1
