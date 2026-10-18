$ErrorActionPreference = 'Stop'
$env:Path = 'C:\Strawberry\c\bin;' + $env:Path
$root = $PSScriptRoot
$culture = [Globalization.CultureInfo]::InvariantCulture
$results = [Collections.Generic.List[object]]::new()
$configs = @(
  @{ Version = 'original'; Alpha = '1' },
  @{ Version = 'generalized'; Alpha = '1' },
  @{ Version = 'original'; Alpha = '1.5' },
  @{ Version = 'generalized'; Alpha = '1.5' },
  @{ Version = 'original'; Alpha = '2' },
  @{ Version = 'generalized'; Alpha = '2' }
)
$rng = [Random]::new(1462026)
New-Item -ItemType Directory -Force -Path (Join-Path $root 'benchmarks') | Out-Null
$metadata = [ordered]@{
  Started = (Get-Date).ToString('o')
  BaselineCommit = '45683819'
  Compiler = (& g++ --version | Select-Object -First 1)
  CPU = (Get-CimInstance Win32_Processor | Select-Object Name,NumberOfLogicalProcessors)
  OS = (Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version)
  Threads = 32
  WarmupsPerConfiguration = 1
  MeasuredRunsPerConfiguration = 5
  Ordering = 'Seeded randomized complete blocks; seed 1462026'
  OpenMP = @(Get-ChildItem Env: | Where-Object Name -Match '^(OMP_|GOMP_)' | Select-Object Name,Value)
  Binaries = @(Get-FileHash (Join-Path $root 'original-build/primecount.exe'), (Join-Path $root 'generalized-build/primecount.exe'))
}
$metadata | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $root 'benchmark-environment.json') -Encoding utf8

for ($round = 0; $round -le 5; $round++) {
  $order = @(0..5)
  for ($i = $order.Count - 1; $i -gt 0; $i--) {
    $j = $rng.Next($i + 1)
    $temp = $order[$i]
    $order[$i] = $order[$j]
    $order[$j] = $temp
  }
  foreach ($index in $order) {
    $config = $configs[$index]
    $exe = Join-Path $root ($config.Version + '-build/primecount.exe')
    $arguments = @('1e20', '--status', '--threads=32', ('--alpha-z=' + $config.Alpha))
    $started = (Get-Date).ToString('o')
    $watch = [Diagnostics.Stopwatch]::StartNew()
    $output = & $exe @arguments 2>&1
    $exitCode = $LASTEXITCODE
    $watch.Stop()
    $raw = $output -join "`n"
    $file = 'round-{0}-{1}-alpha-{2}.log' -f $round,$config.Version,$config.Alpha
    $raw | Set-Content (Join-Path $root ('benchmarks/' + $file)) -Encoding utf8
    if ($exitCode -ne 0) { throw "Benchmark failed: $file (exit $exitCode)" }
    if ($raw -notmatch '(?m)^2220819602560918840\r?$') { throw "Incorrect pi(1e20): $file" }
    $times = [regex]::Matches($raw, 'Seconds:\s*([0-9.]+)')
    if ($times.Count -ne 6) { throw "Expected six timings: $file" }
    $row = [pscustomobject][ordered]@{
      Round = $round
      Warmup = ($round -eq 0)
      Version = $config.Version
      AlphaZ = $config.Alpha
      AlphaY = [regex]::Match($raw, '(?m)^alpha_y = ([0-9.]+)').Groups[1].Value
      Y = [regex]::Match($raw, '(?m)^y = (\d+)').Groups[1].Value
      Z = [regex]::Match($raw, '(?m)^z = (\d+)').Groups[1].Value
      Sigma = [double]::Parse($times[0].Groups[1].Value,$culture)
      Phi0 = [double]::Parse($times[1].Groups[1].Value,$culture)
      AC = [double]::Parse($times[2].Groups[1].Value,$culture)
      B = [double]::Parse($times[3].Groups[1].Value,$culture)
      D = [double]::Parse($times[4].Groups[1].Value,$culture)
      Total = [double]::Parse($times[5].Groups[1].Value,$culture)
      WallSeconds = $watch.Elapsed.TotalSeconds
      Started = $started
      Log = $file
    }
    $results.Add($row)
    $results | Export-Csv (Join-Path $root 'benchmark-results.csv') -NoTypeInformation -Encoding utf8
    Write-Output ('round={0} {1} alpha_z={2} total={3:F3}s D={4:F3}s' -f $round,$config.Version,$config.Alpha,$row.Total,$row.D)
  }
}
