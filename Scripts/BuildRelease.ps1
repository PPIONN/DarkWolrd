<#
.SYNOPSIS
    설치본 빌드: 패키징 → 자동 점검 → 배포용 zip까지 한 번에 만듭니다.

.DESCRIPTION
    1. Win64 Development 패키지를 만듭니다 (VC++ 재배포 패키지 포함, 디버그 파일 제외).
    2. 만든 설치본을 -DemoLoopTest로 실행해 튜토리얼을 시작부터 출구까지 자동으로 플레이합니다
       (이동 → 마법 → 사냥 → 카드 → 소환 → 함께 전투 → 출구).
       점검 화면과 결과는 <출력 폴더>\DarkWolrd-v<버전>-looptest에 남습니다.
    3. 플레이 안내문을 넣고 DarkWolrd-v<버전>-Win64.zip으로 묶습니다.

    점검에 실패하면 zip을 만들지 않고 멈춥니다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File Scripts\BuildRelease.ps1 -Version 0.1.0

.EXAMPLE
    # 에디터를 켜 둔 채로 빌드할 때 (켜 두면 에디터 코드는 다시 빌드할 수 없음)
    powershell -ExecutionPolicy Bypass -File Scripts\BuildRelease.ps1 -Version 0.1.0 -EditorOpen
#>
param(
    [Parameter(Mandatory = $true)][string]$Version,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$OutputRoot = 'C:\UnrealProject\Builds',
    [switch]$EditorOpen,
    [switch]$SkipTest
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'DarkWolrd.uproject'
$archiveDir = Join-Path $OutputRoot "DarkWolrd-v$Version"
$gameDir = Join-Path $archiveDir 'Windows'
$testDir = Join-Path $OutputRoot "DarkWolrd-v$Version-looptest"
$zipPath = Join-Path $OutputRoot "DarkWolrd-v$Version-Win64.zip"
$logPath = Join-Path $OutputRoot "DarkWolrd-v$Version-build.log"

function Invoke-Package {
    Write-Host "[1/3] 패키징 중 (로그: $logPath)"
    $uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
    $uatArgs = "BuildCookRun -project=`"$project`" -noP4 -platform=Win64 -clientconfig=Development " +
        "-build -cook -stage -pak -prereqs -nodebuginfo -archive -archivedirectory=`"$archiveDir`" -utf8output -unattended"
    if ($EditorOpen) { $uatArgs += ' -nocompileeditor' }

    # 로그를 파일로 바로 보냄 (PowerShell 5.1에서 직접 받으면 경고 한 줄도 오류로 처리됨)
    $build = Start-Process -FilePath $uat -ArgumentList $uatArgs -Wait -PassThru -NoNewWindow `
        -RedirectStandardOutput $logPath -RedirectStandardError "$logPath.err"
    if ($build.ExitCode -ne 0) {
        throw "패키징 실패 (종료 코드 $($build.ExitCode)). 로그를 확인하세요: $logPath"
    }
}

function Invoke-LoopTest {
    Write-Host '[2/3] 설치본 자동 점검 중 (게임 창이 잠시 열립니다)'
    $savedDir = Join-Path $gameDir 'DarkWolrd\Saved'
    if (Test-Path $savedDir) { Remove-Item $savedDir -Recurse -Force }
    if (Test-Path $testDir) { Remove-Item $testDir -Recurse -Force }
    New-Item -ItemType Directory -Path $testDir | Out-Null

    $game = Start-Process -FilePath (Join-Path $gameDir 'DarkWolrd.exe') -PassThru `
        -ArgumentList '-DemoLoopTest', '-windowed', '-ResX=1280', '-ResY=720'
    $finished = $game.WaitForExit(300000)
    if (-not $finished) {
        Get-Process -Name 'DarkWolrd*' -ErrorAction SilentlyContinue | Stop-Process -Force
    }

    # DarkWolrd.exe는 실제 게임을 띄우는 시작 파일이라, 게임 본체가 파일을 놓을 때까지 조금 더 기다림
    while (Get-Process -Name 'DarkWolrd*' -ErrorAction SilentlyContinue) { Start-Sleep -Milliseconds 500 }

    Copy-Item (Join-Path $savedDir 'Logs\DarkWolrd.log') $testDir
    Get-ChildItem $savedDir -Recurse -Filter '*.png' | Copy-Item -Destination $testDir

    $results = Select-String -Path (Join-Path $testDir 'DarkWolrd.log') -Pattern '\[LoopTest\]' |
        ForEach-Object { $_.Line -replace '^.*\[LoopTest\] ', '  ' }
    $results | Write-Host
    if (-not $finished) { throw "자동 점검이 5분 안에 끝나지 않았습니다. 기록: $testDir" }
    if (-not ($results -match 'PASS')) { throw "자동 점검 실패. 기록: $testDir" }

    # 점검 기록이 배포본에 섞이지 않게 지움
    Remove-Item $savedDir -Recurse -Force
}

function New-ReleaseZip {
    Write-Host '[3/3] zip 만드는 중'
    Copy-Item (Join-Path $PSScriptRoot 'PlayerReadme.txt') (Join-Path $gameDir 'ReadMe.txt') -Force
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    Compress-Archive -Path (Join-Path $gameDir '*') -DestinationPath $zipPath
}

Invoke-Package
if (-not $SkipTest) { Invoke-LoopTest }
New-ReleaseZip

$sizeMb = [math]::Round((Get-Item $zipPath).Length / 1MB)
Write-Host "완료: $zipPath ($sizeMb MB)"
