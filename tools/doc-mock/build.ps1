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

# Get-Slice 와 범위 DIFF 가 같이 쓰는 범위 계산. 0부터 센 첫 줄과 끝 줄을 돌려준다.
function Get-SliceBounds([string[]] $All, [string] $Path, [string] $Begin, [string] $Until) {
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

	return @($First, $Last)
}

# Begin 문자열이 나오는 줄부터 Until 문자열 앞줄까지. 시작 쪽 주석은 포함하고,
# 끝에 남는 빈 줄·주석(다음 덩어리 것)은 잘라낸다. 들여쓰기는 소스 그대로 둔다.
function Get-Slice([string] $Path, [string] $Begin, [string] $Until) {
	$All = Get-BlobLines $Path
	$First, $Last = Get-SliceBounds $All $Path $Begin $Until
	return ($All[$First..$Last]) -join "`n"
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

# 옮긴 파일의 옛 경로. 옛 경로를 함께 넘겨야 git 이 짝을 지어 바뀐 줄만 보인다.
function Get-OldPath([string] $Path) {
	foreach ($Line in @(& git -C $Repo diff -M --name-status $BaseTag $Tag)) {
		$Parts = $Line -split "`t"
		if ($Parts.Count -eq 3 -and $Parts[0].StartsWith('R') -and $Parts[2] -eq $Path) { return $Parts[1] }
	}
	return $null
}

# diff 에서 머리글을 빼고 헝크만 돌려준다. 머리글 줄 수가 새 파일과 바뀐 파일에서 달라서 첫 @@ 부터 자른다.
function Get-DiffHunks([string] $Path) {
	$Paths = @($Path)
	$OldPath = Get-OldPath $Path
	if ($null -ne $OldPath) { $Paths = @($OldPath, $Path) }
	$Lines = @(& git -C $Repo diff -M -U3 $BaseTag $Tag -- @Paths)
	if ($LASTEXITCODE -ne 0) { throw "git diff failed: $Path" }
	$First = 0
	while ($First -lt $Lines.Count -and $Lines[$First].StartsWith('@@') -eq $false) { $First++ }
	if ($First -ge $Lines.Count) { throw "no diff: $Path" }
	return ,$Lines[$First..($Lines.Count - 1)]
}

# 범위 DIFF. SLICE 와 같은 규칙으로 잡은 범위 안의 줄만 남기고 헝크 머리글을 다시 계산한다. 범위 끝 줄 바로 뒤에서 지운 줄도 범위에 넣는다.
function Get-DiffRange([string] $Path, [string] $Begin, [string] $Until) {
	$All = Get-BlobLines $Path
	$First, $Last = Get-SliceBounds $All $Path $Begin $Until
	$From = $First + 1
	$To = $Last + 1

	$Result = New-Object System.Collections.Generic.List[string]
	$Kept = New-Object System.Collections.Generic.List[string]
	$KeptOld = 0
	$KeptNew = 0
	$OldNo = 0
	$NewNo = 0

	$Flush = {
		if (($Kept | Where-Object { $_.StartsWith('+') -or $_.StartsWith('-') }).Count -gt 0) {
			$OldCount = ($Kept | Where-Object { $_.StartsWith(' ') -or $_.StartsWith('-') }).Count
			$NewCount = ($Kept | Where-Object { $_.StartsWith(' ') -or $_.StartsWith('+') }).Count
			$Result.Add("@@ -$KeptOld,$OldCount +$KeptNew,$NewCount @@")
			$Result.AddRange($Kept)
		}
		$Kept.Clear()
	}

	foreach ($Line in (Get-DiffHunks $Path)) {
		$Header = [regex]::Match($Line, '^@@ -(\d+)(?:,\d+)? \+(\d+)(?:,\d+)? @@')
		if ($Header.Success) {
			. $Flush
			$OldNo = [int]$Header.Groups[1].Value
			$NewNo = [int]$Header.Groups[2].Value
			continue
		}
		if ($Line.StartsWith('\')) { continue }

		$Removed = $Line.StartsWith('-')
		if (($NewNo -ge $From -and $NewNo -le $To) -or ($Removed -and $NewNo -eq $To + 1)) {
			if ($Kept.Count -eq 0) { $KeptOld = $OldNo; $KeptNew = $NewNo }
			$Kept.Add($Line)
		}
		elseif ($Kept.Count -gt 0) {
			. $Flush
		}

		if ($Removed) { $OldNo++ }
		elseif ($Line.StartsWith('+')) { $NewNo++ }
		else { $OldNo++; $NewNo++ }
	}
	. $Flush

	if ($Result.Count -eq 0) { throw "no diff in range: $Begin => $Until ($Path)" }
	return ($Result -join "`n")
}

$DiffEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	return Escape-Html ((Get-DiffHunks $Match.Groups[1].Value) -join "`n")
}

$DiffRangeEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	return Escape-Html (Get-DiffRange $Match.Groups[1].Value $Match.Groups[2].Value $Match.Groups[3].Value)
}

$Output = [regex]::Replace($Template, '<!--FILE:(.+?)-->', $FileEvaluator)
$Output = [regex]::Replace($Output, '<!--SYMBOL:(.+?):(.+?)-->', $SymbolEvaluator)
$Output = [regex]::Replace($Output, '<!--SLICE:(.+?):(.+?)=>(.+?)-->', $SliceEvaluator)
$Output = [regex]::Replace($Output, '<!--DIFF:([^:>\n]+?):(.+?)=>(.+?)-->', $DiffRangeEvaluator)
$Output = [regex]::Replace($Output, '<!--DIFF:([^:>\n]+?)-->', $DiffEvaluator)
$Target = Join-Path $Dir "tutorial-$Number.html"
[IO.File]::WriteAllText($Target, $Output, $Utf8)

$Left = [regex]::Matches($Output, '<!--(FILE|SYMBOL|SLICE|DIFF):').Count
"written : $Target ($([Math]::Round((Get-Item $Target).Length / 1KB)) KB), unreplaced placeholders : $Left"
