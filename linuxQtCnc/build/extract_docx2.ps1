Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead('d:\cnclx\linuxcnc\LinuxCNC_Qt_Refactoring_Plan.docx')
$entry = $zip.Entries | Where-Object { $_.FullName -eq 'word/document.xml' }
$stream = $entry.Open()
$reader = New-Object System.IO.StreamReader($stream)
$xml = $reader.ReadToEnd()
$reader.Close()
$stream.Close()
$zip.Dispose()

# Use regex to extract text from <w:t>...</w:t> elements
$matches = [System.Text.RegularExpressions.Regex]::Matches($xml, '<w:t[^>]*>([^<]*)</w:t>')
$text = ""
foreach ($m in $matches) {
    $text += $m.Groups[1].Value
}

# Split by paragraph breaks (<w:p>) to get lines
$paragraphs = [System.Text.RegularExpressions.Regex]::Split($xml, '</w:p>')
$lines = New-Object System.Collections.Generic.List[string]
foreach ($p in $paragraphs) {
    $txts = [System.Text.RegularExpressions.Regex]::Matches($p, '<w:t[^>]*>([^<]*)</w:t>')
    $line = ""
    foreach ($t in $txts) {
        $line += $t.Groups[1].Value
    }
    if ($line.Trim().Length -gt 0) {
        $lines.Add($line)
    }
}

$outPath = 'd:\cnclx\linuxcnc\linuxQtCnc\build\plan_document.txt'
$lines | Out-File -FilePath $outPath -Encoding UTF8
Write-Host "Lines extracted:" $lines.Count

# Print first 300 lines for context
$counter = 0
foreach ($l in $lines) {
    $counter++
    Write-Host "[$counter]" $l
    if ($counter -ge 400) { break }
}
