param(
  [string]$HostIp,
  [switch]$Tunnel,
  [switch]$PrintOnly
)

function Get-AdapterIpv4([object]$adapter) {
  $address = Get-NetIPAddress -AddressFamily IPv4 -InterfaceIndex $adapter.ifIndex -ErrorAction SilentlyContinue |
    Where-Object { $_.IPAddress -notlike '169.254.*' } |
    Select-Object -First 1
  if ($address) { return $address.IPAddress }
  return $null
}

if (-not $HostIp) {
  $adapters = Get-NetAdapter -ErrorAction SilentlyContinue | Where-Object { $_.Status -eq 'Up' }

  $tether = $adapters | Where-Object {
    $_.InterfaceDescription -match 'Remote NDIS|Internet Sharing|RNDIS|USB.*Sharing'
  } | Select-Object -First 1
  if ($tether) { $HostIp = Get-AdapterIpv4 $tether }

  if (-not $HostIp) {
    $wifi = $adapters | Where-Object { $_.InterfaceAlias -match 'Wi-Fi|Wireless|WLAN' } | Select-Object -First 1
    if ($wifi) { $HostIp = Get-AdapterIpv4 $wifi }
  }

  if (-not $HostIp) {
    $defaultRoute = Get-NetRoute -AddressFamily IPv4 -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue |
      Sort-Object RouteMetric |
      Select-Object -First 1
    if ($defaultRoute) {
      $adapter = Get-NetAdapter -InterfaceIndex $defaultRoute.InterfaceIndex -ErrorAction SilentlyContinue
      if ($adapter) { $HostIp = Get-AdapterIpv4 $adapter }
    }
  }

  if (-not $HostIp) {
    $fallback = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
      Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' } |
      Select-Object -First 1
    if ($fallback) { $HostIp = $fallback.IPAddress }
  }
}

if (-not $HostIp) {
  Write-Error 'No LAN IPv4 address found. Pass -HostIp explicitly.'
  exit 1
}

Write-Host "Metro host: $HostIp"
Write-Host "Test from the phone browser: http://$HostIp`:8081"

if ($PrintOnly) { exit 0 }

$env:REACT_NATIVE_PACKAGER_HOSTNAME = $HostIp

if ($Tunnel) {
  npx expo start --dev-client --tunnel
} else {
  npx expo start --dev-client --host lan
}
