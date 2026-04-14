param(
    [string]$Root = "Source"
)

$ErrorActionPreference = "Stop"

$utf8 = [System.Text.UTF8Encoding]::new($false, $true)
$targets = @("*.h", "*.hpp", "*.c", "*.cpp", "*.cs", "*.md", "*.ini", "*.json", "*.uplugin", "*.uproject")
$files = Get-ChildItem -Path $Root -Recurse -File -Include $targets
$invalid = @()

foreach ($file in $files) {
    try {
        $bytes = [System.IO.File]::ReadAllBytes($file.FullName)
        [void]$utf8.GetString($bytes)
    }
    catch {
        $invalid += $file.FullName
    }
}

if ($invalid.Count -gt 0) {
    Write-Host "Invalid UTF-8 files:" -ForegroundColor Red
    $invalid | ForEach-Object { Write-Host $_ }
    exit 1
}

Write-Host "All checked files are valid UTF-8." -ForegroundColor Green
