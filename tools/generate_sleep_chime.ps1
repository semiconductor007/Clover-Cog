# Original synthesized placeholder cue. Run from any directory; no dependencies.
$outputPath = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../wav/sleep_success.wav'))
$sampleRate = 22050
$sampleCount = [int]($sampleRate * 0.36)
$stream = [IO.File]::Create($outputPath)
$writer = [IO.BinaryWriter]::new($stream)
try {
    $writer.Write([Text.Encoding]::ASCII.GetBytes('RIFF'))
    $writer.Write([int](36 + $sampleCount * 2))
    $writer.Write([Text.Encoding]::ASCII.GetBytes('WAVEfmt '))
    $writer.Write([int]16)
    $writer.Write([int16]1)
    $writer.Write([int16]1)
    $writer.Write([int]$sampleRate)
    $writer.Write([int]($sampleRate * 2))
    $writer.Write([int16]2)
    $writer.Write([int16]16)
    $writer.Write([Text.Encoding]::ASCII.GetBytes('data'))
    $writer.Write([int]($sampleCount * 2))
    for ($i = 0; $i -lt $sampleCount; $i++) {
        $time = $i / [double]$sampleRate
        $frequency = if ($time -lt 0.16) { 523.251 } else { 659.255 }
        $localTime = if ($time -lt 0.16) { $time } else { $time - 0.16 }
        $duration = if ($time -lt 0.16) { 0.16 } else { 0.20 }
        $envelope = [Math]::Min(1.0, $localTime / 0.012) * [Math]::Max(0.0, 1 - $localTime / $duration)
        $sample = [int16][Math]::Round(4200 * $envelope * [Math]::Sin(2 * [Math]::PI * $frequency * $localTime))
        $writer.Write($sample)
    }
} finally {
    $writer.Dispose()
    $stream.Dispose()
}
