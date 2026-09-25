param([Parameter(Mandatory)] [int] $Lesson)

$ErrorActionPreference = 'Stop'
$Dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Repo = Split-Path -Parent (Split-Path -Parent $Dir)
$Utf8 = New-Object Text.UTF8Encoding $false
$Number = '{0:D2}' -f $Lesson
$Template = [IO.File]::ReadAllText((Join-Path $Dir "template-$Number.html"), $Utf8)

$BaseTag = 'refs/tags/tut{0:D2}' -f ($Lesson - 1)
$Tag = "refs/tags/tut$Number"

function Escape-Html([string] $Text) {
	return $Text.Replace('&', '&amp;').Replace('<', '&lt;').Replace('>', '&gt;')
}

function Get-BlobLines([string] $Path) {
	$Lines = @(& git -C $Repo show "${Tag}:$Path")
	if ($LASTEXITCODE -ne 0) { throw "git show failed: $Path" }
	return ,$Lines
}

# 심볼이 나오는 첫 줄부터 중괄호 짝이 맞는 줄까지. 바로 위의 연속된 주석을 함께 넣는다.
function Get-Symbol([string] $Path, [string] $Symbol) {
	$All = Get-BlobLines $Path

	$Start = -1
	for ($i = 0; $i -lt $All.Count; $i++) {
		if ($All[$i].Contains($Symbol)) { $Start = $i; break }
	}
	if ($Start -lt 0) { throw "symbol not found: $Symbol ($Path)" }

	$First = $Start
	while ($First -gt 0 -and $All[$First - 1].TrimStart().StartsWith('//')) { $First-- }

	$Depth = 0
	$Opened = $false
	$End = -1
	for ($i = $Start; $i -lt $All.Count; $i++) {
		foreach ($Char in $All[$i].ToCharArray()) {
			if ($Char -eq '{') { $Depth++; $Opened = $true }
			elseif ($Char -eq '}') { $Depth-- }
		}
		if ($Opened -and $Depth -le 0) { $End = $i; break }
	}
	if ($End -lt 0) { throw "unbalanced braces: $Symbol ($Path)" }

	return ($All[$First..$End]) -join "`n"
}

# Begin 문자열이 나오는 줄부터 Until 문자열 앞줄까지. 시작 쪽 주석은 포함하고,
# 끝에 남는 빈 줄·주석(다음 덩어리 것)은 잘라낸 뒤 공통 들여쓰기를 제거한다.
function Get-Slice([string] $Path, [string] $Begin, [string] $Until) {
	$All = Get-BlobLines $Path

	$Start = -1
	for ($i = 0; $i -lt $All.Count; $i++) {
		if ($All[$i].Contains($Begin)) { $Start = $i; break }
	}
	if ($Start -lt 0) { throw "slice begin not found: $Begin ($Path)" }

	$Stop = -1
	for ($i = $Start + 1; $i -lt $All.Count; $i++) {
		if ($All[$i].Contains($Until)) { $Stop = $i; break }
	}
	if ($Stop -lt 0) { throw "slice end not found: $Until ($Path)" }

	$First = $Start
	while ($First -gt 0 -and $All[$First - 1].TrimStart().StartsWith('//')) { $First-- }

	$Last = $Stop - 1
	while ($Last -gt $Start -and ($All[$Last].Trim() -eq '' -or $All[$Last].TrimStart().StartsWith('//'))) { $Last-- }

	$Chunk = $All[$First..$Last]
	$Indent = ($Chunk | Where-Object { $_.Trim() -ne '' } |
		ForEach-Object { $_.Length - $_.TrimStart("`t").Length } | Measure-Object -Minimum).Minimum
	return (($Chunk | ForEach-Object { if ($_.Trim() -eq '') { $_ } else { $_.Substring($Indent) } }) -join "`n")
}

$FileEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	return Escape-Html ((Get-BlobLines $Match.Groups[1].Value) -join "`n")
}

$SymbolEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	return Escape-Html (Get-Symbol $Match.Groups[1].Value $Match.Groups[2].Value)
}

$SliceEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	return Escape-Html (Get-Slice $Match.Groups[1].Value $Match.Groups[2].Value $Match.Groups[3].Value)
}

$DiffEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	$Path = $Match.Groups[1].Value
	$Lines = & git -C $Repo diff -U3 $BaseTag $Tag -- $Path
	if ($LASTEXITCODE -ne 0) { throw "git diff failed: $Path" }
	$Body = $Lines | Select-Object -Skip 4
	return Escape-Html (($Body -join "`n"))
}

$Output = [regex]::Replace($Template, '<!--FILE:(.+?)-->', $FileEvaluator)
$Output = [regex]::Replace($Output, '<!--SYMBOL:(.+?):(.+?)-->', $SymbolEvaluator)
$Output = [regex]::Replace($Output, '<!--SLICE:(.+?):(.+?)=>(.+?)-->', $SliceEvaluator)
$Output = [regex]::Replace($Output, '<!--DIFF:(.+?)-->', $DiffEvaluator)
$Target = Join-Path $Dir "tutorial-$Number.html"
[IO.File]::WriteAllText($Target, $Output, $Utf8)

$Left = [regex]::Matches($Output, '<!--(FILE|SYMBOL|SLICE|DIFF):').Count
"written : $Target ($([Math]::Round((Get-Item $Target).Length / 1KB)) KB), unreplaced placeholders : $Left"
