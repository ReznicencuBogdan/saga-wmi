$arralias=@(
"ALIAS",
"BASEBOARD",
"BIOS",
"BOOTCONFIG",
"CDROM",
"COMPUTERSYSTEM",
"CPU",
"CSPRODUCT",
"DCOMAPP",
"DESKTOP",
"DESKTOPMONITOR",
"DEVICEMEMORYADDRESS",
"DISKDRIVE",
"DISKQUOTA",
"DMACHANNEL",
"ENVIRONMENT",
"GROUP",
"IDECONTROLLER",
"IRQ",
"JOB",
"LOADORDER",
"LOGICALDISK",
"LOGON",
"MEMCACHE",
"MEMORYCHIP",
"MEMPHYSICAL",
"NETCLIENT",
"NETLOGIN",
"NETPROTOCOL",
"NETUSE",
"NIC",
"NICCONFIG",
"NTEVENTLOG",
"ONBOARDDEVICE",
"OS",
"PAGEFILE",
"PAGEFILESET",
"PARTITION",
"PORT",
"PORTCONNECTOR",
"PRINTER",
"PRINTERCONFIG",
"PRINTJOB",
"PROCESS",
"QFE",
"QUOTASETTING",
"RDACCOUNT",
"RDNIC",
"RDPERMISSIONS",
"RDTOGGLE",
"RECOVEROS",
"REGISTRY",
"SCSICONTROLLER",
"SERVER",
"SERVICE",
"SHADOWCOPY",
"SHADOWSTORAGE",
"SHARE",
"SOUNDDEV",
"STARTUP",
"SYSACCOUNT",
"SYSDRIVER",
"SYSTEMENCLOSURE",
"SYSTEMSLOT",
"connectionspoints.",
"TAPEDRIVE",
"TEMPERATURE",
"TIMEZONE",
"UPS",
"USERACCOUNT",
"VOLTAGE",
"VOLUME",
"VOLUMEQUOTASETTING",
"VOLUMEUSERQUOTA",
"WMISET")

# Get-CimClass-Namespaceroot/CIMV2|Where-ObjectCimClassName-likeWin32*|Select-ObjectCimClassName
# wmic/output:e:\processes.csvprocessget/all/format:csv

[string]$path_current = Get-Location
[string]$path_wmi = "$path_current\wmi"
[string]$path_wmi_indiv = "$path_wmi\individual"
[string]$path_wmi_file = "$path_wmi\wmi_interfaces.html"

rm -r -fo $path_wmi
New-Item -ItemType Directory -Force -Path $path_wmi_indiv

Write-Output "path_current = $path_current"
Write-Output "path_wmi = $path_wmi"
Write-Output "path_wmi_indiv = $path_wmi_indiv"
Write-Output "path_wmi_file = $path_wmi_file"

Foreach ($i in $arralias)
{
    $path = "$path_wmi_indiv\wmi_$i.html"
    Write-Output "Writing to '$path', alias=$i"
    cmd.exe /c "wmic /output:'$path' $i get /all /format:htable"
}

Write-Output "-- Queried all wmi interfaces"
Write-Output "-- Combining wmi results"

Get-ChildItem -Path $path_wmi_indiv -Filter "*.html" | Get-Content | Add-Content -Path $path_wmi_file

Write-Output "-- Output generated - can be found at '$path_wmi_file'"
