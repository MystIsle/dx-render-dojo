$ErrorActionPreference = 'Stop'
$Dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Repo = Split-Path -Parent (Split-Path -Parent $Dir)
$Utf8 = New-Object Text.UTF8Encoding $false
$Template = [IO.File]::ReadAllText((Join-Path $Dir 'template.html'), $Utf8)

function Escape-Html([string] $Text) {
	return $Text.Replace('&', '&amp;').Replace('<', '&lt;').Replace('>', '&gt;')
}

function Get-BlobLines([string] $Path) {
	$Lines = @(& git -C $Repo show "tut02:$Path")
	if ($LASTEXITCODE -ne 0) { throw "git show failed: $Path" }
	return ,$Lines
}

$IncludeEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	return [IO.File]::ReadAllText((Join-Path $Dir $Match.Groups[1].Value), $Utf8)
}

$FileEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	return Escape-Html ((Get-BlobLines $Match.Groups[1].Value) -join "`n")
}

$LinesEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	$All = Get-BlobLines $Match.Groups[1].Value
	$Start = [int]$Match.Groups[2].Value
	$End = [int]$Match.Groups[3].Value
	if ($End -gt $All.Count) { throw "line range out of file: $($Match.Value)" }
	return Escape-Html (($All[($Start - 1)..($End - 1)]) -join "`n")
}

$DiffEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	$Path = $Match.Groups[1].Value
	$Lines = & git -C $Repo diff -U3 tut01 tut02 -- $Path
	if ($LASTEXITCODE -ne 0) { throw "git diff failed: $Path" }
	$Body = $Lines | Select-Object -Skip 4
	return Escape-Html (($Body -join "`n"))
}

$Output = [regex]::Replace($Template, '<!--INCLUDE:(.+?)-->', $IncludeEvaluator)
$Output = [regex]::Replace($Output, '<!--FILE:(.+?)-->', $FileEvaluator)
$Output = [regex]::Replace($Output, '<!--LINES:(.+?):(\d+)-(\d+)-->', $LinesEvaluator)
$Output = [regex]::Replace($Output, '<!--DIFF:(.+?)-->', $DiffEvaluator)
$Target = Join-Path $Dir 'tutorial-02.html'
[IO.File]::WriteAllText($Target, $Output, $Utf8)

$Left = [regex]::Matches($Output, '<!--(INCLUDE|FILE|LINES|DIFF):').Count
"written : $Target ($([Math]::Round((Get-Item $Target).Length / 1KB)) KB), unreplaced placeholders : $Left"
