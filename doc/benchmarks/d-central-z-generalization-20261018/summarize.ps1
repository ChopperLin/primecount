$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$rows = @(Import-Csv (Join-Path $root 'benchmark-results.csv') | Where-Object Warmup -eq 'False')
function Median($values) {
  $sorted = @($values | ForEach-Object { [double]$_ } | Sort-Object)
  if ($sorted.Count % 2) { return $sorted[[int][math]::Floor($sorted.Count / 2)] }
  return ($sorted[$sorted.Count / 2 - 1] + $sorted[$sorted.Count / 2]) / 2
}
$summary = foreach ($group in ($rows | Group-Object Version,AlphaZ)) {
  $data = @($group.Group)
  [pscustomobject][ordered]@{
    Version = $data[0].Version
    AlphaZ = $data[0].AlphaZ
    Runs = $data.Count
    AlphaY = $data[0].AlphaY
    Y = $data[0].Y
    Z = $data[0].Z
    ACMedian = Median $data.AC
    BMedian = Median $data.B
    DMedian = Median $data.D
    TotalMedian = Median $data.Total
    TotalMin = ($data.Total | ForEach-Object { [double]$_ } | Measure-Object -Minimum).Minimum
    TotalMax = ($data.Total | ForEach-Object { [double]$_ } | Measure-Object -Maximum).Maximum
  }
}
$summary | Sort-Object Version,{[double]$_.AlphaZ} | Export-Csv (Join-Path $root 'benchmark-summary.csv') -NoTypeInformation -Encoding utf8
$summary | Sort-Object Version,{[double]$_.AlphaZ} | Format-Table Version,AlphaZ,Runs,ACMedian,BMedian,DMedian,TotalMedian,TotalMin,TotalMax
