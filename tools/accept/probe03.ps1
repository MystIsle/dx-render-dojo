param([string]$Exe, [string]$Mode = 'normal')

$ErrorActionPreference = 'Stop'
[System.Text.Encoding]::RegisterProvider([System.Text.CodePagesEncodingProvider]::Instance)
if (-not ('Probe' -as [type])) {
    Add-Type -TypeDefinition (Get-Content -Raw "$PSScriptRoot\Probe.cs") -ReferencedAssemblies 'System.IO.MemoryMappedFiles', 'System.Collections', 'System.Threading.Thread', 'System.Threading','System.Text.Encoding.CodePages', 'System.Console'
}

function Show([string]$Title) { Write-Output "--- $Title"; [Probe]::TakeLines() | ForEach-Object { Write-Output "  $_" } }
function Rect([IntPtr]$H) { $c = New-Object Probe+RECT; [void][Probe]::GetClientRect($H, [ref]$c); $w = New-Object Probe+RECT; [void][Probe]::GetWindowRect($H, [ref]$w); "client $($c.Right)x$($c.Bottom), window $w" }

[Probe]::StartListener()
$p = Start-Process -FilePath $Exe -PassThru
[Probe]::SetTarget($p.Id)
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 50 -and $h -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $h = [Probe]::FindProcessWindow($p.Id, 'DxRenderDojo') }
Start-Sleep -Milliseconds 1500

if ($Mode -eq 'resizefatal') {
    Show 'start'
    [void][Probe]::PostMessageW($h, 0x0232, [IntPtr]::Zero, [IntPtr]::Zero)
    [void][Probe]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 1000, 700, 0x2 -bor 0x4 -bor 0x10 -bor 0x4000)
    Start-Sleep -Milliseconds 1500
    $Mode = 'fatal'
}

if ($Mode -eq 'fatal') {
    $box = [Probe]::FindProcessWindow($p.Id, '#32770')
    Write-Output "box found: $($box -ne [IntPtr]::Zero)"
    if ($box -ne [IntPtr]::Zero) {
        Write-Output "box title: $([Probe]::WindowText($box))"
        Write-Output "box text:`n$([Probe]::WindowText([Probe]::GetDlgItem($box, 0xFFFF)))"
        Write-Output "main window enabled while box is up: $([Probe]::IsWindowEnabled($h))"
        Show 'log before OK'
        [void][Probe]::PostMessageW($box, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
    }
    $p.WaitForExit(5000) | Out-Null
    Write-Output "exited: $($p.HasExited) code: $($p.ExitCode)"
    [Probe]::StopListener()
    return
}

Show 'start'
Write-Output (Rect $h)

$p.Refresh(); $cpu0 = $p.TotalProcessorTime; Start-Sleep -Seconds 2; $p.Refresh()
Write-Output ("cpu over 2s: {0:N0} ms" -f ($p.TotalProcessorTime - $cpu0).TotalMilliseconds)

[void][Probe]::SendMessageW($h, 0x0231, [IntPtr]::Zero, [IntPtr]::Zero)
[void][Probe]::SendMessageW($h, 0x0232, [IntPtr]::Zero, [IntPtr]::Zero)
Start-Sleep -Milliseconds 300
Show 'move only (enter/exit size-move, same size)'

[void][Probe]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 1000, 700, 0x2 -bor 0x4 -bor 0x10)
Start-Sleep -Milliseconds 500
Show 'resize to 1000x700 outer'
Write-Output (Rect $h)

$before = Rect $h
for ($k = 0; $k -lt 2; $k++) {
    [void][Probe]::PostMessageW($h, 0x0104, [IntPtr]0x0D, [IntPtr](1 -bor (1 -shl 29)))
    Start-Sleep -Milliseconds 700
    [void][Probe]::PostMessageW($h, 0x0104, [IntPtr]0x0D, [IntPtr](1 -bor (1 -shl 29)))
    Start-Sleep -Milliseconds 700
}
Show 'alt+enter x4 (on, off, on, off)'
[void][Probe]::PostMessageW($h, 0x0104, [IntPtr]0x0D, [IntPtr](1 -bor (1 -shl 29)))
Start-Sleep -Milliseconds 1000
Show 'alt+enter posted (1st)'
Write-Output (Rect $h)
[void][Probe]::PostMessageW($h, 0x0104, [IntPtr]0x0D, [IntPtr](1 -bor (1 -shl 29)))
Start-Sleep -Milliseconds 1000
Show 'alt+enter posted (2nd)'
Write-Output "back to the same rect: $((Rect $h) -eq $before)"

[void][Probe]::PostMessageW($h, 0x0100, [IntPtr]0x1B, [IntPtr]1)
$p.WaitForExit(5000) | Out-Null
Start-Sleep -Milliseconds 300
Show 'after ESC'
Write-Output "exited: $($p.HasExited) code: $($p.ExitCode)"
[Probe]::StopListener()
