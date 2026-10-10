param(
    [Parameter(Mandatory)][string]$File,
    [string]$Vmrun = "$env:ProgramFiles\VMware\VMware Workstation\vmrun.exe",
    [string]$Vmx = "$env:USERPROFILE\Documents\Virtual Machines\Windows 11 x64\Windows 11 x64.vmx",
    [string]$VmCredential = "$env:USERPROFILE\.codex\vmware\win11-vm.xml",
    [string]$GuestCredential = "$env:USERPROFILE\.codex\vmware\win11-guest.xml",
    [string]$Destination = 'C:\Users\Public\dwm_hook-{hash}.dll'
)
$ErrorActionPreference = 'Stop'
foreach ($path in @($File, $Vmrun, $Vmx, $VmCredential, $GuestCredential)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "File not found: $path" }
}
$vmLogin = Import-Clixml -LiteralPath $VmCredential
$guestLogin = Import-Clixml -LiteralPath $GuestCredential
if ($vmLogin -isnot [PSCredential] -or $guestLogin -isnot [PSCredential]) {
    throw 'VM credentials must be PSCredential objects saved with Export-Clixml.'
}
$sourceFile = (Resolve-Path -LiteralPath $File).Path
$sha256 = [Security.Cryptography.SHA256]::Create()
try {
    $digest = $sha256.ComputeHash([IO.File]::ReadAllBytes($sourceFile))
    $hash = [BitConverter]::ToString($digest).Replace('-', '').Substring(0, 12).ToLowerInvariant()
} finally { $sha256.Dispose() }
$destinationFile = $Destination.Replace('{hash}', $hash).Replace('/', '\')
$arguments = @('-T', 'ws', '-vp', $vmLogin.GetNetworkCredential().Password,
    '-gu', $guestLogin.UserName, '-gp', $guestLogin.GetNetworkCredential().Password)
if ($Destination.Contains('{hash}')) {
    $existing = & $Vmrun @arguments fileExistsInGuest $Vmx $destinationFile 2>&1
    $alreadyUploaded = $LASTEXITCODE -eq 0 -and "$existing".Trim() -eq 'The file exists.'
} else {
    $alreadyUploaded = $false
}
if (!$alreadyUploaded) {
    $result = & $Vmrun @arguments CopyFileFromHostToGuest $Vmx $sourceFile $destinationFile 2>&1
    if ($LASTEXITCODE -ne 0) { throw "VM upload failed: $result" }
}
Write-Host "DLL on VM: $destinationFile"
