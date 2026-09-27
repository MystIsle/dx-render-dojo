# 단계 커밋 tutNN-s1 부터 마지막 단계까지를 차례로 컴파일만 한다. 링크·실행은 하지 않는다.
# 중간 커밋의 조건은 컴파일 하나이고, 컴파일이 "쓰기 전에 선언했는가" 를 본다 (docs/DOC_STYLE.md 2절).
# 임시 worktree 하나를 단계마다 옮겨 가며 쓰므로 바뀐 파일과 그 파일을 include 하는 cpp 만 다시 컴파일된다.
param(
    [Parameter(Mandatory = $true)] [string] $Lesson,
    [string] $WorkDir = '',
    [string] $Configuration = 'Debug',
    [int] $ErrorLines = 5
)

$ErrorActionPreference = 'Stop'
$Repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Utf8 = New-Object Text.UTF8Encoding $false

# 02 · 2 · tut02 · alpha01 을 모두 받는다.
if ($Lesson -match '^\d+$') { $Tag = 'tut{0:D2}' -f [int]$Lesson }
elseif ($Lesson.StartsWith('tut')) { $Tag = $Lesson }
else { $Tag = "tut$Lesson" }

$Pattern = '^' + [regex]::Escape($Tag) + '-s(\d+)$'
$Steps = @(& git -C $Repo tag -l "$Tag-s*" | Where-Object { $_ -match $Pattern } | Sort-Object { [int]($_ -replace '^.*-s', '') })
if ($Steps.Count -eq 0) { throw "no step tags: $Tag-s*" }

# 임시 폴더 아래에 두면 MSB8029(중간 디렉터리가 임시 폴더 아래라 증분 빌드를 믿을 수 없음)가 나고,
# 리포 안에 두면 리포 작업 트리를 건드린다. 기본값은 리포 옆 폴더다.
if ($WorkDir -eq '') { $WorkDir = Join-Path (Split-Path -Parent $Repo) ((Split-Path -Leaf $Repo) + '-accept') }
$WorkDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($WorkDir)
foreach ($Forbidden in @($env:TEMP, $Repo)) {
    $Root = [IO.Path]::GetFullPath($Forbidden).TrimEnd('\') + '\'
    if (($WorkDir.TrimEnd('\') + '\').StartsWith($Root, [StringComparison]::OrdinalIgnoreCase)) { throw "WorkDir must be outside $Forbidden : $WorkDir" }
}
$Worktree = Join-Path $WorkDir "$Tag-steps"
if (Test-Path $Worktree) { throw "already exists: $Worktree (leftover from an earlier run? git worktree remove --force)" }

$VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $VsWhere)) { throw "vswhere not found: $VsWhere" }
$MSBuild = @(& $VsWhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe') | Select-Object -First 1
if (-not $MSBuild) { throw 'MSBuild not found' }

"msbuild : $MSBuild"
"worktree : $Worktree"
$CreatedWorkDir = -not (Test-Path $WorkDir)
New-Item -ItemType Directory -Force $WorkDir | Out-Null
& git -C $Repo worktree add --quiet --detach $Worktree "refs/tags/$($Steps[0])"
if ($LASTEXITCODE -ne 0) { throw "git worktree add failed: $Worktree" }

$Failed = New-Object System.Collections.Generic.List[string]
try {
    foreach ($Step in $Steps) {
        & git -C $Worktree switch --quiet --detach "refs/tags/$Step"
        if ($LASTEXITCODE -ne 0) { throw "git switch failed: $Step" }

        # 콘솔 출력은 코드 페이지를 타므로 로그 파일을 UTF-8 로 받아 읽는다.
        $Log = Join-Path $WorkDir "$Step.log"
        & $MSBuild (Join-Path $Worktree 'DxRenderDojo.vcxproj') -t:ClCompile "-p:Configuration=$Configuration" -p:Platform=x64 -nologo -nr:false -noconlog "-flp:LogFile=$Log;Encoding=UTF-8;Verbosity=minimal"
        if ($LASTEXITCODE -eq 0) {
            "$Step : pass"
            Remove-Item $Log
            continue
        }

        # 실패한 단계의 로그는 남긴다. 에러 줄이 없으면(MSBuild 자체 실패 등) 로그 끝을 보인다.
        $Failed.Add($Step)
        $All = [IO.File]::ReadAllLines($Log, $Utf8)
        $Errors = @($All | Where-Object { $_ -match '\berror [A-Z]+\d+' } | ForEach-Object { ($_.Replace("$Worktree\", '') -replace ' \[[^\]]*\.vcxproj\]$', '').Trim() } | Select-Object -Unique)
        if ($Errors.Count -eq 0) { $Errors = @($All | Select-Object -Last $ErrorLines) }
        "$Step : FAIL ($($Errors.Count) error lines, log $Log)"
        $Errors | Select-Object -First $ErrorLines | ForEach-Object { "    $_" }
    }
}
finally {
    # 컴파일 산출물(x64/)은 .gitignore 에 있어 --force 없이 지워진다. 그 밖의 파일이 생겨 거절되면 사람이 보고 지운다.
    # Windows PowerShell 5.1 은 Stop 에서 네이티브 명령의 stderr 를 예외로 바꾸므로 여기서는 Continue 로 둔다.
    $ErrorActionPreference = 'Continue'
    & git -C $Repo worktree remove $Worktree
    if (Test-Path $Worktree) { "could not remove $Worktree. Check it, then : git worktree remove --force `"$Worktree`"" }
    elseif ($CreatedWorkDir -and @(Get-ChildItem $WorkDir -Force).Count -eq 0) { Remove-Item $WorkDir }
}

"$Tag : $($Steps.Count) steps, $($Failed.Count) failed"
if ($Failed.Count -gt 0) { exit 1 }
