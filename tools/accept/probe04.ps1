param([string]$Exe, [string]$Mode = 'shot', [string]$WorkDir = '', [string]$OutDir = '')

$ErrorActionPreference = 'Stop'
[System.Text.Encoding]::RegisterProvider([System.Text.CodePagesEncodingProvider]::Instance)
if (-not ('Probe' -as [type])) {
    Add-Type -TypeDefinition (Get-Content -Raw "$PSScriptRoot\Probe.cs") -ReferencedAssemblies 'System.IO.MemoryMappedFiles', 'System.Collections', 'System.Threading.Thread', 'System.Threading', 'System.Text.Encoding.CodePages', 'System.Console'
}
if (-not ('Probe4' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class Probe4
{
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [DllImport("user32.dll")] public static extern bool SetProcessDpiAwarenessContext(IntPtr v);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
}
'@
}
[void][Probe4]::SetProcessDpiAwarenessContext([IntPtr](-4))
Add-Type -AssemblyName System.Drawing

function Launch {
    if ($WorkDir) { $p = Start-Process -FilePath $Exe -WorkingDirectory $WorkDir -PassThru }
    else { $p = Start-Process -FilePath $Exe -PassThru }
    [Probe]::SetTarget($p.Id)
    $h = [IntPtr]::Zero
    for ($i = 0; $i -lt 50 -and $h -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $h = [Probe]::FindProcessWindow($p.Id, 'DxRenderDojo') }
    Start-Sleep -Milliseconds 1500
    return @($p, $h)
}
function Show([string]$Title) {
    Write-Output "--- $Title"
    $ls = [Probe]::TakeLines()
    $script:AllLines += $ls
    $ls | ForEach-Object { Write-Output "  $_" }
}
function ClientRect([IntPtr]$H) { $c = New-Object Probe+RECT; [void][Probe]::GetClientRect($H, [ref]$c); return $c }
function SetClient([IntPtr]$H, [int]$CW, [int]$CH) {
    $c = ClientRect $H
    $w = New-Object Probe+RECT; [void][Probe]::GetWindowRect($H, [ref]$w)
    $dx = ($w.Right - $w.Left) - $c.Right; $dy = ($w.Bottom - $w.Top) - $c.Bottom
    [void][Probe]::SetWindowPos($H, [IntPtr]::Zero, 0, 0, $CW + $dx, $CH + $dy, 0x2 -bor 0x4 -bor 0x10)
    Start-Sleep -Milliseconds 700
}
function AltEnter([IntPtr]$H) {
    [void][Probe]::PostMessageW($H, 0x0104, [IntPtr]0x0D, [IntPtr](1 -bor (1 -shl 29)))
    Start-Sleep -Milliseconds 1200
}
function Front([IntPtr]$H) {
    [void][Probe]::SetWindowPos($H, [IntPtr](-1), 0, 0, 0, 0, 0x1 -bor 0x2)
    [void][Probe4]::SetForegroundWindow($H)
    Start-Sleep -Milliseconds 600
}
function Capture([IntPtr]$H, [string]$Tag) {
    $c = ClientRect $H
    $pt = New-Object Probe4+POINT
    [void][Probe4]::ClientToScreen($H, [ref]$pt)
    $bmp = New-Object System.Drawing.Bitmap $c.Right, $c.Bottom
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($pt.X, $pt.Y, 0, 0, $bmp.Size)
    $g.Dispose()
    if ($OutDir) { $bmp.Save((Join-Path $OutDir "shot-$Tag.png"), [System.Drawing.Imaging.ImageFormat]::Png) }
    $minX = [int]::MaxValue; $minY = [int]::MaxValue; $maxX = -1; $maxY = -1; $n = 0; $other = 0
    $data = $bmp.LockBits((New-Object System.Drawing.Rectangle 0, 0, $bmp.Width, $bmp.Height), [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bytes = New-Object byte[] ($data.Stride * $bmp.Height)
    [System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
    $bmp.UnlockBits($data)
    for ($y = 0; $y -lt $bmp.Height; $y++) {
        $row = $y * $data.Stride
        for ($x = 0; $x -lt $bmp.Width; $x++) {
            $i = $row + $x * 4
            $b = $bytes[$i]; $gg = $bytes[$i + 1]; $r = $bytes[$i + 2]
            if ($gg -gt 128 -and $r -lt 80 -and $b -lt 80) {
                $n++
                if ($x -lt $minX) { $minX = $x }; if ($x -gt $maxX) { $maxX = $x }
                if ($y -lt $minY) { $minY = $y }; if ($y -gt $maxY) { $maxY = $y }
            }
            elseif ($r -gt 16 -or $gg -gt 16 -or $b -gt 16) { $other++ }
        }
    }
    $center = $bmp.GetPixel([int]($bmp.Width / 2), [int]($bmp.Height / 2))
    $bw = $maxX - $minX + 1; $bh = $maxY - $minY + 1
    Write-Output ("[{0}] client {1}x{2} | green px {3} | box {4}x{5} @({6},{7}) | box w/h {8:N3} | box h / client h {9:N4} | non-black non-green px {10} | center {11}" -f $Tag, $c.Right, $c.Bottom, $n, $bw, $bh, $minX, $minY, ($bw / $bh), ($bh / $c.Bottom), $other, $center)
    $bmp.Dispose()
}

if ($Mode -eq 'log') {
    $script:AllLines = @()
    [Probe]::StartListener()
    $p, $h = Launch
    Show 'start'
    Write-Output "client $((ClientRect $h).Right)x$((ClientRect $h).Bottom)"
    SetClient $h 1200 600; Show 'client 1200x600'; Write-Output "client $((ClientRect $h).Right)x$((ClientRect $h).Bottom)"
    SetClient $h 600 800; Show 'client 600x800'; Write-Output "client $((ClientRect $h).Right)x$((ClientRect $h).Bottom)"
    SetClient $h 800 600; Show 'client 800x600'; Write-Output "client $((ClientRect $h).Right)x$((ClientRect $h).Bottom)"
    AltEnter $h; Show 'alt+enter on'; Write-Output "client $((ClientRect $h).Right)x$((ClientRect $h).Bottom)"
    AltEnter $h; Show 'alt+enter off'; Write-Output "client $((ClientRect $h).Right)x$((ClientRect $h).Bottom)"
    Start-Sleep -Milliseconds 1000; Show 'idle 1s'
    [void][Probe]::PostMessageW($h, 0x0100, [IntPtr]0x1B, [IntPtr]1)
    $p.WaitForExit(5000) | Out-Null
    Start-Sleep -Milliseconds 300
    Show 'after ESC'
    Write-Output "exited: $($p.HasExited) code: $($p.ExitCode)"
    [Probe]::StopListener()
    $tagged = @($script:AllLines | Where-Object { $_ -match '\[D3D\]|\[DXGI\]' })
    $bad = @($tagged | Where-Object { $_ -match '\[경고\]|\[에러\]' })
    Write-Output "log lines $($script:AllLines.Count) | [D3D]/[DXGI] lines $($tagged.Count) | of which warning/error $($bad.Count) | all warning/error lines $(@($script:AllLines | Where-Object { $_ -match '\[경고\]|\[에러\]' }).Count)"
    return
}

$p, $h = Launch
if ($Mode -eq 'shot') {
    Front $h
    Capture $h 'start'
    SetClient $h 800 600; Front $h; Capture $h 'c800x600'
    SetClient $h 1200 600; Front $h; Capture $h 'c1200x600'
    SetClient $h 600 800; Front $h; Capture $h 'c600x800'
    SetClient $h 800 600
    AltEnter $h; Front $h; Capture $h 'fullscreen'
    AltEnter $h; Front $h; Capture $h 'windowed-again'
}
else {
    Front $h
    Capture $h 'quick'
}
[void][Probe]::PostMessageW($h, 0x0100, [IntPtr]0x1B, [IntPtr]1)
$p.WaitForExit(5000) | Out-Null
Write-Output "exited: $($p.HasExited) code: $($p.ExitCode)"
