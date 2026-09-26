param([string]$Exe)

[System.Text.Encoding]::RegisterProvider([System.Text.CodePagesEncodingProvider]::Instance)
Add-Type -TypeDefinition (Get-Content -Raw "$PSScriptRoot\Probe.cs") -ReferencedAssemblies 'System.IO.MemoryMappedFiles', 'System.Collections', 'System.Threading.Thread', 'System.Threading', 'System.Text.Encoding.CodePages', 'System.Console'
Add-Type -AssemblyName System.Drawing

function Launch {
    $p = Start-Process -FilePath $Exe -PassThru
    [Probe]::SetTarget($p.Id)
    $h = [IntPtr]::Zero
    for ($i = 0; $i -lt 50 -and $h -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $h = [Probe]::FindProcessWindow($p.Id, 'DxRenderDojo') }
    Start-Sleep -Milliseconds 1200
    return @($p, $h)
}
function Show([string]$Title) { Write-Output "--- $Title"; [Probe]::TakeLines() | ForEach-Object { Write-Output "  $_" } }

[Probe]::StartListener()

$p, $h = Launch
Show 'start'
$w = New-Object Probe+RECT; [void][Probe]::GetWindowRect($h, [ref]$w)
$c = New-Object Probe+RECT; [void][Probe]::GetClientRect($h, [ref]$c)
Write-Output "client $($c.Right)x$($c.Bottom), window $w"
$bmp = New-Object System.Drawing.Bitmap 1, 1
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen([int](($w.Left + $w.Right) / 2), [int](($w.Top + $w.Bottom) / 2), 0, 0, (New-Object System.Drawing.Size 1, 1))
Write-Output ("center pixel: {0}" -f $bmp.GetPixel(0, 0))

[void][Probe]::SendMessageW($h, 0x0231, [IntPtr]::Zero, [IntPtr]::Zero)
[void][Probe]::SendMessageW($h, 0x0232, [IntPtr]::Zero, [IntPtr]::Zero)
Start-Sleep -Milliseconds 300
Show 'move only (enter/exit size-move)'

[void][Probe]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 100, 100, 0x2 -bor 0x4 -bor 0x10)
Start-Sleep -Milliseconds 300
Show 'shrink to 100x100 outer (min size clamp)'
[void][Probe]::GetClientRect($h, [ref]$c); [void][Probe]::GetWindowRect($h, [ref]$w)
Write-Output "client $($c.Right)x$($c.Bottom), window $w"

[void][Probe]::PostMessageW($h, 0x0100, [IntPtr]0x1B, [IntPtr]1)
$p.WaitForExit(5000) | Out-Null
Show 'after ESC'
Write-Output "ESC exited: $($p.HasExited) code: $($p.ExitCode)"

$p, $h = Launch
[void][Probe]::TakeLines()
[void][Probe]::PostMessageW($h, 0x0112, [IntPtr]0xF060, [IntPtr]::Zero)
$p.WaitForExit(5000) | Out-Null
Show 'after close button (WM_SYSCOMMAND SC_CLOSE)'
Write-Output "close exited: $($p.HasExited) code: $($p.ExitCode)"
[Probe]::StopListener()
