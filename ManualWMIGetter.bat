@echo off
setlocal enableextensions disabledelayedexpansion

WMIC Path Win32_DiskDrive Get DeviceID
WMIC Path Win32_DiskPartition Get DeviceID
WMIC Path Win32_LogicalDisk Get DeviceID
WMIC Path Win32_PhysicalMedia Get SerialNumber,Tag
WMIC Path Win32_BaseBoard Get SerialNumber
WMIC Path Win32_NetworkAdapter Get macaddress
WMIC Path Win32_Processor Get processorid