param(
    [Parameter(Mandatory)]
    [ValidatePattern('^COM[1-9][0-9]*$')]
    [string]$Port
)

$ErrorActionPreference = 'Stop'
$serial = [System.IO.Ports.SerialPort]::new($Port, 115200)
$serial.ReadTimeout = 3500
try {
    $serial.Open()
    # Let the dongle observe DTR low before requesting a fresh dump.
    Start-Sleep -Milliseconds 1200
    $serial.DtrEnable = $true
    $header = $serial.ReadLine().TrimEnd("`r")
    if ($header -notlike 'SOFLE LOG v1 *') {
        throw "$Port did not return a Sofle log header."
    }
    $header
    for ($i = 0; $i -le 256; $i++) {
        $line = $serial.ReadLine().TrimEnd("`r")
        $line
        if ($line -eq 'END SOFLE LOG') { return }
    }
    throw 'Sofle log dump did not terminate.'
} finally {
    $serial.Dispose()
}
