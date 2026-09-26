param([string]$Exe, [string]$Png)

[System.Text.Encoding]::RegisterProvider([System.Text.CodePagesEncodingProvider]::Instance)
if (-not ('Probe' -as [type])) {
    Add-Type -TypeDefinition (Get-Content -Raw "$PSScriptRoot\Probe.cs") -ReferencedAssemblies 'System.IO.MemoryMappedFiles', 'System.Collections', 'System.Threading.Thread', 'System.Threading', 'System.Text.Encoding.CodePages', 'System.Console'
}
Add-Type -AssemblyName System.Drawing

$p = Start-Process -FilePath $Exe -PassThru
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 50 -and $h -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $h = [Probe]::FindProcessWindow($p.Id, 'DxRenderDojo') }
Start-Sleep -Milliseconds 1500

$w = New-Object Probe+RECT; [void][Probe]::GetWindowRect($h, [ref]$w)
$bmp = New-Object System.Drawing.Bitmap ($w.Right - $w.Left), ($w.Bottom - $w.Top)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($w.Left, $w.Top, 0, 0, $bmp.Size)
$bmp.Save($Png, [System.Drawing.Imaging.ImageFormat]::Png)
Write-Output ("center pixel: {0}" -f $bmp.GetPixel([int]($bmp.Width / 2), [int]($bmp.Height / 2)))

$p.Refresh(); $cpu0 = $p.TotalProcessorTime; Start-Sleep -Seconds 2; $p.Refresh()
Write-Output ("cpu over 2s: {0:N0} ms" -f ($p.TotalProcessorTime - $cpu0).TotalMilliseconds)

[void][Probe]::PostMessageW($h, 0x0100, [IntPtr]0x1B, [IntPtr]1)
$p.WaitForExit(5000) | Out-Null
Write-Output "exited: $($p.HasExited) code: $($p.ExitCode)"
