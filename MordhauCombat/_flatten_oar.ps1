$ErrorActionPreference='Stop'
$root='C:\Modding\Mithras Mods\MordhauCombat\Nexus Release\MordhauCombat\meshes\actors\character\_1stperson\animations\OpenAnimationReplacer'
$types=@('OneHanded','TwoHanded')
$dirs=@('Top','TopRight','Right','BottomRight','Bottom','BottomLeft','Left','TopLeft')
foreach($t in $types){
  foreach($d in $dirs){
    $bucket=Join-Path (Join-Path $root $t) $d
    if(-not (Test-Path $bucket)){ continue }
    Get-ChildItem -Path $bucket -Recurse -File -Filter *.hkx | ForEach-Object {
      Copy-Item -Path $_.FullName -Destination (Join-Path $bucket $_.Name) -Force
    }
    Get-ChildItem -Path $bucket -Directory | ForEach-Object {
      Remove-Item -Path $_.FullName -Recurse -Force
    }
  }
}
