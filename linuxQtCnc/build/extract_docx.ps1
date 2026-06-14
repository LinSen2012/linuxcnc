Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead('d:\cnclx\linuxcnc\LinuxCNC_Qt_Refactoring_Plan.docx')
$entry = $zip.Entries | Where-Object { $_.FullName -eq 'word/document.xml' }
if ($entry) {
    $stream = $entry.Open()
    $reader = New-Object System.IO.StreamReader($stream)
    $xml = $reader.ReadToEnd()
    $reader.Close()
    $stream.Close()
    $xml | Out-File -FilePath 'd:\cnclx\linuxcnc\linuxQtCnc\build\plan_document.xml' -Encoding UTF8
    Write-Host "Extracted OK. Size:" $xml.Length
} else {
    Write-Host "document.xml not found in docx"
    foreach ($e in $zip.Entries) { Write-Host " -" $e.FullName }
}
$zip.Dispose()

# Now convert XML to plain text
[xml]$doc = Get-Content 'd:\cnclx\linuxcnc\linuxQtCnc\build\plan_document.xml'
$ns = @{w = 'http://schemas.openxmlformats.org/wordprocessingml/2006/main'}
$lines = $doc.SelectNodes('//w:p', $ns) | ForEach-Object {
    $textNodes = $_.SelectNodes('.//w:t', $ns)
    if ($textNodes.Count -gt 0) {
        ($textNodes | ForEach-Object { $_.InnerText }) -join ''
    }
}
$lines | Out-File -FilePath 'd:\cnclx\linuxcnc\linuxQtCnc\build\plan_document.txt' -Encoding UTF8
Write-Host "Text extracted to plan_document.txt"
Write-Host "=== PREVIEW ==="
$lines | Select-Object -First 200 | ForEach-Object { Write-Host $_ }
