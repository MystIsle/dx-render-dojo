# -Lesson 을 빼면 원고가 있는 편을 모두 만든다. 목차 페이지는 매번 만든다.
param([int] $Lesson = 0)

$ErrorActionPreference = 'Stop'
$Dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Repo = Split-Path -Parent (Split-Path -Parent $Dir)
$Utf8 = New-Object Text.UTF8Encoding $false
$Toc = [IO.File]::ReadAllText((Join-Path $Dir 'toc.json'), $Utf8) | ConvertFrom-Json
$Labels = $Toc.labels

function Escape-Html([string] $Text) {
	return $Text.Replace('&', '&amp;').Replace('<', '&lt;').Replace('>', '&gt;')
}

# 마커의 @N 은 N단계 커밋(tutNN-sN)을 읽는다. DIFF@N 은 N-1단계와 N단계의 차이다. @ 가 없으면 편 태그와 앞 편 태그.
function Set-Step([string] $Step) {
	if ($Step -eq '') {
		$script:Tag = $LessonTag
		$script:BaseTag = $LessonBase
		return
	}
	$Number = [int]$Step
	$script:Tag = "refs/tags/$($Entry.Tag)-s$Number"
	$script:BaseTag = $(if ($Number -gt 1) { "refs/tags/$($Entry.Tag)-s$($Number - 1)" } else { $LessonBase })
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
	Set-Step $Match.Groups['step'].Value
	return Escape-Html ((Get-BlobLines $Match.Groups['path'].Value) -join "`n")
}

$SymbolEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	Set-Step $Match.Groups['step'].Value
	return Escape-Html (Get-Symbol $Match.Groups['path'].Value $Match.Groups['symbol'].Value)
}

$SliceEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	Set-Step $Match.Groups['step'].Value
	return Escape-Html (Get-Slice $Match.Groups['path'].Value $Match.Groups['begin'].Value $Match.Groups['until'].Value)
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
# 헝크 머리글 끝에 git 이 붙이는 문맥 줄은 뗀다. 헝크보다 위의 가장 가까운 함수 머리라서 바뀐 줄이 든 함수와 다를 수 있다.
function Get-DiffHunks([string] $Path) {
	$Paths = @($Path)
	$OldPath = Get-OldPath $Path
	if ($null -ne $OldPath) { $Paths = @($OldPath, $Path) }
	$Lines = @(& git -C $Repo diff -M -U3 $BaseTag $Tag -- @Paths)
	if ($LASTEXITCODE -ne 0) { throw "git diff failed: $Path" }
	$First = 0
	while ($First -lt $Lines.Count -and $Lines[$First].StartsWith('@@') -eq $false) { $First++ }
	if ($First -ge $Lines.Count) { throw "no diff: $Path" }
	$Hunks = $Lines[$First..($Lines.Count - 1)] | ForEach-Object { $_ -replace '^(@@ [^@]+ @@).*$', '$1' }
	return ,@($Hunks)
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
	Set-Step $Match.Groups['step'].Value
	return Escape-Html ((Get-DiffHunks $Match.Groups['path'].Value) -join "`n")
}

$DiffRangeEvaluator = [Text.RegularExpressions.MatchEvaluator] {
	param($Match)
	Set-Step $Match.Groups['step'].Value
	return Escape-Html (Get-DiffRange $Match.Groups['path'].Value $Match.Groups['begin'].Value $Match.Groups['until'].Value)
}

# 목차 순서로 편 목록을 편다. 이전·다음 편은 이 순서를 따른다.
function Get-TocLessons {
	$List = New-Object System.Collections.Generic.List[object]
	foreach ($Stage in $Toc.stages) {
		foreach ($Entry in $Stage.lessons) {
			$Id = $Entry.tag.Substring(3)
			$List.Add([pscustomobject]@{
				Tag      = $Entry.tag
				Base     = $Entry.base
				Title    = $Entry.title
				Original = $Entry.original
				Stage    = $Stage
				Id       = $Id
				Label    = $(if ($Id.StartsWith('alpha')) { [string][char]0x03B1 + $Id.Substring(5) } else { $Id })
				Source   = Join-Path $Dir "template-$Id.html"
				Page     = "tutorial-$Id.html"
				Written  = Test-Path (Join-Path $Dir "template-$Id.html")
			})
		}
	}
	return ,$List
}

# 원고 머리 요약의 첫 문장. 목차 페이지의 편 요약으로 쓴다.
function Get-Summary([string] $Path) {
	$Text = [IO.File]::ReadAllText($Path, $Utf8)
	$Lede = [regex]::Match($Text, '<p class="lede">(.*?)</p>', 'Singleline')
	if (-not $Lede.Success) { throw "lede not found: $Path" }
	return [regex]::Match($Lede.Groups[1].Value, '^.*?[.?!](?=\s|$)', 'Singleline').Value.Trim()
}

function Get-StageTrack($Stage) {
	if ($Stage.common) { return $Labels.common }
	return $Labels.optional
}

function Get-PagerLink($Entry, [string] $Direction, [string] $Class) {
	$ClassAttr = $(if ($Class) { " class=`"$Class`"" } else { '' })
	$Href = $(if ($Entry.Written) { " href=`"$($Entry.Page)`"" } else { '' })
	return "<a$ClassAttr$Href><span class=`"dir`">$(Escape-Html $Direction)</span><span class=`"name`">$(Escape-Html "$($Entry.Label) $($Entry.Title)")</span></a>"
}

function Get-Pager($Lessons, [int] $Index) {
	if ($Index -gt 0) { $Left = Get-PagerLink $Lessons[$Index - 1] $Labels.prev '' }
	else { $Left = "<a href=`"index.html`"><span class=`"dir`">$(Escape-Html $Labels.tocPrev)</span><span class=`"name`">$(Escape-Html $Labels.home)</span></a>" }
	if ($Index -lt $Lessons.Count - 1) { $Right = Get-PagerLink $Lessons[$Index + 1] $Labels.next 'next' }
	else { $Right = "<a class=`"next`" href=`"index.html`"><span class=`"dir`">$(Escape-Html $Labels.tocNext)</span><span class=`"name`">$(Escape-Html $Labels.home)</span></a>" }
	return "$Left`n`t`t$Right"
}

function Get-StageRange($Stage) {
	$Numbers = @($Stage.lessons | ForEach-Object { $_.tag.Substring(3) } | Where-Object { $_ -match '^\d+$' })
	if ($Numbers.Count -eq 0) { return '' }
	$Contiguous = $true
	for ($i = 1; $i -lt $Numbers.Count; $i++) {
		if ([int]$Numbers[$i] -ne [int]$Numbers[$i - 1] + 1) { $Contiguous = $false }
	}
	if ($Contiguous) { return "tut$($Numbers[0])~$($Numbers[-1])" }
	return 'tut' + ($Numbers -join [string][char]0x00B7)
}

function Get-StageId($Stage) {
	if ($null -eq $Stage.no) { return 'extra' }
	return "stage$($Stage.no)"
}

function Get-Progress($Lessons) {
	$Numbered = @($Lessons | Where-Object { $_.Id -match '^\d+$' } | Sort-Object { [int]$_.Id })
	$Extra = @($Lessons | Where-Object { $_.Id -notmatch '^\d+$' })
	$Cells = New-Object System.Collections.Generic.List[string]
	$AddCell = {
		param($Entry)
		$Class = $(if ($Entry.Written) { 'cell done' } elseif ($Entry.Stage.common) { 'cell common' } else { 'cell' })
		$Cells.Add("<span class=`"$Class`" title=`"$($Entry.Tag) $(Escape-Html $Entry.Title)`"></span>")
	}
	for ($i = 0; $i -lt $Numbered.Count; $i++) {
		& $AddCell $Numbered[$i]
		$EndsCommon = $Numbered[$i].Stage.common -and $i + 1 -lt $Numbered.Count -and -not $Numbered[$i + 1].Stage.common
		if ($EndsCommon) { $Cells.Add('<span class="cell gap"></span>') }
	}
	foreach ($Entry in $Extra) {
		$Cells.Add('<span class="cell gap"></span>')
		& $AddCell $Entry
	}
	return ($Cells -join '')
}

function Get-Stages($Lessons) {
	$Sections = New-Object System.Collections.Generic.List[string]
	foreach ($Stage in $Toc.stages) {
		$Rows = New-Object System.Collections.Generic.List[string]
		foreach ($Entry in ($Lessons | Where-Object { $_.Stage -eq $Stage })) {
			$Inner = "<span class=`"num`">$($Entry.Label)</span><span class=`"title`">$(Escape-Html $Entry.Title)</span><span class=`"orig`">$(Escape-Html $Entry.Original)</span>"
			if ($Entry.Written) {
				$Inner += "<span class=`"summary`">$(Get-Summary $Entry.Source)</span>"
				$Rows.Add("`t`t`t<li class=`"entry`"><a class=`"row`" href=`"$($Entry.Page)`">$Inner</a></li>")
			}
			else {
				$Rows.Add("`t`t`t<li class=`"entry todo`"><div class=`"row`">$Inner</div></li>")
			}
		}
		$Number = $(if ($null -ne $Stage.no) { "<span class=`"stage-no`">STAGE $($Stage.no)</span>" } else { '' })
		$Range = Get-StageRange $Stage
		$RangeSpan = $(if ($Range) { "<span class=`"stage-range`">$Range</span>" } else { '' })
		$TrackClass = $(if ($Stage.common) { 'common' } else { 'optional' })
		$Head = "<div class=`"stage-head`">$Number<h2 id=`"$(Get-StageId $Stage)`">$(Escape-Html $Stage.name)</h2>$RangeSpan<span class=`"track $TrackClass`">$(Escape-Html (Get-StageTrack $Stage))</span></div>"
		$Sections.Add("<section class=`"stage`">`n`t`t$Head`n`t`t<p class=`"stage-desc`">$(Escape-Html $Stage.desc)</p>`n`t`t<ol class=`"rows`">`n$($Rows -join "`n")`n`t`t</ol>`n`t</section>")
	}
	return ($Sections -join "`n`t")
}

function Get-Rail {
	$Items = foreach ($Stage in $Toc.stages) {
		$Mark = $(if ($null -ne $Stage.no) { $Stage.no } else { [string][char]0x03B1 })
		"<li><a href=`"#$(Get-StageId $Stage)`"><span class=`"n`">$Mark</span>$(Escape-Html $Stage.name)</a></li>"
	}
	return ($Items -join "`n`t`t")
}

function Write-Page([string] $Target, [string] $Output) {
	[IO.File]::WriteAllText($Target, $Output, $Utf8)
	$Left = [regex]::Matches($Output, '<!--(FILE|SYMBOL|SLICE|DIFF|TOC)[:@]').Count
	"written : $Target ($([Math]::Round((Get-Item $Target).Length / 1KB)) KB), unreplaced placeholders : $Left"
}

$Lessons = Get-TocLessons
if ($Lesson -gt 0) {
	$Wanted = @($Lessons | Where-Object { $_.Id -eq ('{0:D2}' -f $Lesson) })
	if ($Wanted.Count -eq 0 -or -not $Wanted[0].Written) { throw "no template for lesson $Lesson" }
}
else {
	$Wanted = @($Lessons | Where-Object { $_.Written })
}

foreach ($Entry in $Wanted) {
	$Template = [IO.File]::ReadAllText($Entry.Source, $Utf8)
	$LessonTag = "refs/tags/$($Entry.Tag)"
	$LessonBase = $(if ($Entry.Base) { "refs/tags/$($Entry.Base)" } else { $null })

	# 원고 h1 과 목차 제목이 다르면 멈춘다. 이전·다음 편 이름이 목차에서 온다.
	$Heading = [regex]::Match($Template, '<h1>(.*?)</h1>').Groups[1].Value
	if ($Heading -ne (Escape-Html $Entry.Title)) { throw "title mismatch: h1 '$Heading', toc '$($Entry.Title)' ($($Entry.Tag))" }

	$Eyebrow = $(if ($null -ne $Entry.Stage.no) { "STAGE $($Entry.Stage.no)" } else { Escape-Html $Entry.Stage.name })
	$Output = $Template.Replace('<!--TOC:eyebrow-->', "<b>$Eyebrow</b> $([char]0x00B7) $(Escape-Html (Get-StageTrack $Entry.Stage))")
	$Output = $Output.Replace('<!--TOC:base-->', [string]$Entry.Base)
	$Output = $Output.Replace('<!--TOC:pager-->', (Get-Pager $Lessons $Lessons.IndexOf($Entry)))
	$Step = '(?:@(?<step>\d+))?'
	$Output = [regex]::Replace($Output, "<!--FILE${Step}:(?<path>.+?)-->", $FileEvaluator)
	$Output = [regex]::Replace($Output, "<!--SYMBOL${Step}:(?<path>.+?):(?<symbol>.+?)-->", $SymbolEvaluator)
	$Output = [regex]::Replace($Output, "<!--SLICE${Step}:(?<path>.+?):(?<begin>.+?)=>(?<until>.+?)-->", $SliceEvaluator)
	$Output = [regex]::Replace($Output, "<!--DIFF${Step}:(?<path>[^:>\n]+?):(?<begin>.+?)=>(?<until>.+?)-->", $DiffRangeEvaluator)
	$Output = [regex]::Replace($Output, "<!--DIFF${Step}:(?<path>[^:>\n]+?)-->", $DiffEvaluator)
	Write-Page (Join-Path $Dir $Entry.Page) $Output
}

$Index = [IO.File]::ReadAllText((Join-Path $Dir 'template-index.html'), $Utf8)
$Written = @($Lessons | Where-Object { $_.Written }).Count
$Index = $Index.Replace('<!--TOC:progress-->', (Get-Progress $Lessons))
$Index = $Index.Replace('<!--TOC:count-->', "$Written / $($Lessons.Count)")
$Index = $Index.Replace('<!--TOC:stages-->', (Get-Stages $Lessons))
$Index = $Index.Replace('<!--TOC:rail-->', (Get-Rail))
Write-Page (Join-Path $Dir 'index.html') $Index
