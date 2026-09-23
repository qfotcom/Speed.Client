# Send one Legacy line (4-byte BE length + UTF-8) to Speed.Server :9001
param(
    [string]$ServerHost = "127.0.0.1",
    [int]$Port = 9001,
    [string]$Line = "PUB quote.test data"
)

function Send-SpeedLegacyLine {
    param(
        [string]$ServerHost,
        [int]$Port,
        [string]$Line
    )
    $tcp = [System.Net.Sockets.TcpClient]::new($ServerHost, $Port)
    $stream = $tcp.GetStream()
    $payload = [System.Text.Encoding]::UTF8.GetBytes($Line)
    $len = [uint32]$payload.Length
    $lenBytes = [BitConverter]::GetBytes($len)
    if ([BitConverter]::IsLittleEndian) { [Array]::Reverse($lenBytes) }
    $stream.Write($lenBytes, 0, 4)
    $stream.Write($payload, 0, $payload.Length)
    $stream.Flush()

    $rb = New-Object byte[] 4
    [void]$stream.Read($rb, 0, 4)
    if ([BitConverter]::IsLittleEndian) { [Array]::Reverse($rb) }
    $rlen = [BitConverter]::ToUInt32($rb, 0)
    $body = New-Object byte[] $rlen
    if ($rlen -gt 0) { [void]$stream.Read($body, 0, $rlen) }
    $resp = [System.Text.Encoding]::UTF8.GetString($body)
    $tcp.Close()
    Write-Host ">> $Line"
    Write-Host "<< $resp"
}

Send-SpeedLegacyLine -ServerHost $ServerHost -Port $Port -Line $Line
