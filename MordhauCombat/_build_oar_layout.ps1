$ErrorActionPreference='Stop'

$repo = 'C:\Modding\Mithras Mods\MordhauCombat'
$root = Join-Path $repo 'Nexus Release\MordhauCombat\meshes\actors\character\_1stperson\animations\OpenAnimationReplacer'

$gsLeft = 'C:\Modding\MO2\mods\SIGMA - Greatsword animations - 1st person\meshes\actors\character\_1stperson\animations\OpenAnimationReplacer\GreatSword\gs_neutral_combat\_variants_2hm_attackleft'
$gsRight = 'C:\Modding\MO2\mods\SIGMA - Greatsword animations - 1st person\meshes\actors\character\_1stperson\animations\OpenAnimationReplacer\GreatSword\gs_neutral_combat\_variants_2hm_attackright'
$shBase = 'C:\Modding\MO2\mods\SIGMA - Sword animations - 1st person\meshes\actors\character\_1stperson\animations\OpenAnimationReplacer\SigmaBoy\S_N_C'

$oldPaths = @(
  (Join-Path $root 'config.json'),
  (Join-Path $root 'GreatSword'),
  (Join-Path $root 'Sword'),
  (Join-Path $root 'WarAxe'),
  (Join-Path $root 'TwoHanded'),
  (Join-Path $root 'OneHanded')
)
foreach ($p in $oldPaths) {
  if (Test-Path $p) { Remove-Item -Path $p -Recurse -Force }
}

$dirs = @(
  @{Name='Top'; Bucket=1; Src2H=(Join-Path $gsLeft '1.hkx');  Src1H=(Join-Path $shBase '1hm_attackright.hkx')},
  @{Name='TopRight'; Bucket=2; Src2H=(Join-Path $gsLeft '2.hkx'); Src1H=(Join-Path $shBase '1hm_attackleft.hkx')},
  @{Name='Right'; Bucket=3; Src2H=(Join-Path $gsLeft '3.hkx'); Src1H=(Join-Path $shBase '1hm_attackrightdiagonal.hkx')},
  @{Name='BottomRight'; Bucket=4; Src2H=(Join-Path $gsRight '1.hkx'); Src1H=(Join-Path $shBase '1hm_attacklefttdiagonal.hkx')},
  @{Name='Bottom'; Bucket=5; Src2H=(Join-Path $gsRight '2.hkx'); Src1H=(Join-Path $shBase '1hm_attackleft.hkx')},
  @{Name='BottomLeft'; Bucket=6; Src2H=(Join-Path $gsRight '3.hkx'); Src1H=(Join-Path $shBase '1hm_attackright.hkx')},
  @{Name='Left'; Bucket=7; Src2H=(Join-Path $gsRight '4.hkx'); Src1H=(Join-Path $shBase '1hm_attacklefttdiagonal.hkx')},
  @{Name='TopLeft'; Bucket=8; Src2H=(Join-Path $gsLeft '2.hkx'); Src1H=(Join-Path $shBase '1hm_attackrightdiagonal.hkx')}
)

$targets2H = @(
  'gs_neutral_combat_variants_2hm_attackleft\2hm_attackleft.hkx',
  'gs_neutral_combat_variants_2hm_attackright\2hm_attackright.hkx',
  'gs_neutral_combat_variants_2hm_attackleftdiagonal\2hm_attackleftdiagonal.hkx',
  'gs_neutral_combat_variants_2hm_attackrightdiagonal\2hm_attackrightdiagonal.hkx'
)

$targets1H = @(
  'gs_neutral_combat_variants_1hm_attackleft\1hm_attackleft.hkx',
  'gs_neutral_combat_variants_1hm_attackright\1hm_attackright.hkx',
  'gs_neutral_combat_variants_1hm_attackleftdiagonal\1hm_attackleftdiagonal.hkx',
  'gs_neutral_combat_variants_1hm_attackrightdiagonal\1hm_attackrightdiagonal.hkx'
)

function Ensure-CopyTargets($src, $baseDir, $targets) {
  if (-not (Test-Path $src)) { throw "Missing source: $src" }
  foreach ($rel in $targets) {
    $dst = Join-Path $baseDir $rel
    $dstdir = Split-Path -Parent $dst
    if (-not (Test-Path $dstdir)) { New-Item -Path $dstdir -ItemType Directory -Force | Out-Null }
    Copy-Item -Path $src -Destination $dst -Force
  }
}

$eq1HObjs = @(
  @{v=[double]1.0}, @{v=[double]2.0}, @{v=[double]3.0}, @{v=[double]4.0}
)
$eq2HObjs = @(
  @{v=[double]5.0}, @{v=[double]6.0}
)

function Build-EquipConds($arr) {
  $out = @()
  foreach($e in $arr){
    $out += [ordered]@{
      condition='IsEquippedType'
      requiredVersion='1.0.0.0'
      Type=[ordered]@{ value=$e.v }
      'Left hand'=$false
    }
  }
  return $out
}

function Write-Config($path, $name, $desc, $priority, $bucket, $equipArray) {
  $cfg = [ordered]@{
    name=$name
    description=$desc
    priority=$priority
    ignoreDontConvertAnnotationsToTriggersFlag=$true
    hasCustomBlendTimeOnLoop=$true
    blendTimeOnLoop=0.550000011920929
    conditions=@(
      [ordered]@{
        condition='OR'
        requiredVersion='1.0.0.0'
        Conditions=(Build-EquipConds $equipArray)
      },
      [ordered]@{ condition='MordhauIsDirectionalAttack'; requiredVersion='1.0.0.0' },
      [ordered]@{ condition='MordhauIsPowerAttack'; requiredVersion='1.0.0.0'; negated=$true },
      [ordered]@{ condition='MordhauDirectionBucket'; requiredVersion='1.0.0.0'; Comparison='=='; Bucket=[ordered]@{ value=[double]$bucket } },
      [ordered]@{ condition='IsWeaponDrawn'; requiredVersion='1.0.0.0' }
    )
  }
  $json = $cfg | ConvertTo-Json -Depth 8
  Set-Content -Path $path -Value $json -Encoding UTF8
}

foreach ($d in $dirs) {
  $oneDir = Join-Path $root (Join-Path 'OneHanded' $d.Name)
  $twoDir = Join-Path $root (Join-Path 'TwoHanded' $d.Name)

  Ensure-CopyTargets -src $d.Src1H -baseDir $oneDir -targets $targets1H
  Ensure-CopyTargets -src $d.Src2H -baseDir $twoDir -targets $targets2H

  Write-Config -path (Join-Path $oneDir 'config.json') -name "Mordhau OneHanded $($d.Name)" -desc "One-handed bucket $($d.Name)" -priority (210001000 + $d.Bucket) -bucket $d.Bucket -equipArray $eq1HObjs
  Write-Config -path (Join-Path $twoDir 'config.json') -name "Mordhau TwoHanded $($d.Name)" -desc "Two-handed bucket $($d.Name)" -priority (210002000 + $d.Bucket) -bucket $d.Bucket -equipArray $eq2HObjs
}

Write-Host 'DONE'
