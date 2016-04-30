# .SYNOPSIS
#     Generate .NET and Java proxy files, JobScheduler .NET adapter dll and copy this and the jni4net .NET dlls to the given locations
# .DESCRIPTION
#     - generates two proxy files to the given locations and use "JarBaseName" basename as the proxy basename
#       1 - .jn4-<version>.dll
#       2 - .jn4-<version>.jar
#     - generates the JobScheduler .NET adapter .dll (com.sos-berlin.jobscheduler.dotnet.adapter.dll)
#     - copies jni4net .NET dll files from the "proxygen" directory to the DLL location
#
#     Environment variables WINDOWS_NET_SDK_HOME with the installation path of Micosoft Windows SDK (.Net-SDK) is needed,
#     for example: set WINDOWS_NET_SDK_HOME=%windir%\Microsoft.NET\Framework\v4.0.30319
#
# .PARAMETER DotnetJobSchedulerAdapterSourceDirectory
#     Path to the .cs files from the .NET JobScheduler adapter implementation
# .PARAMETER ProxyDllResultDirectory
#     Output directory for the generated proxy .NET dll files
# .EXAMPLE
#     .\Generate-Jni4Net.ps1 "C:\Temp\com.sos-berlin.jobscheduler.engine.engine-job-api-1.10.3.jar" "C:\Temp\adapter_cs" "C:\Temp\proxy"
#          1) copied the generated .NET and Java proxy
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.dll
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.jar
#          2) copied the generated .NET adapter
#               com.sos-berlin.jobscheduler.dotnet.adapter.dll
#          3) copied the existing jni4net .NET dll files from the "proxygen" location
#             to the "C:\Temp\proxy" location
#     .\Generate-Jni4Net.ps1 "C:\Temp\com.sos-berlin.jobscheduler.engine.engine-job-api-1.10.3.jar" "C:\Temp\adapter_cs" "C:\Temp\proxy_dll"
#          1) copies the generated .NET proxy
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.dll
#             to the "C:\Temp\proxy_dll" location
#          2) copies the generated .NET adapter
#               com.sos-berlin.jobscheduler.dotnet.adapter-1.10.3.dll
#             to the "C:\Temp\proxy_dll" location
#          3) copies the existing jni4net .NET dll files from the "proxygen" location
#             to the "C:\Temp\proxy_dll" location


# ----------------------------------------------------------------------
# Command Line Arguments
# ----------------------------------------------------------------------
param(
	[parameter(Mandatory=$true)] [string] $DotnetJobSchedulerAdapterSourceDirectory,
    [parameter(Mandatory=$true)] [string] $ProxyDllResultDirectory
)

$ProxyAssemblyBasename   = "com.sos-berlin.jobscheduler.dotnet.job-api.proxy"  # This name is constant and can't be changed - the name defines the assembly name and is referenced by the another .dll files.

$JobApiClassesDirectory  = Join-Path (Get-Location) "target\jni4net-input\javaClasses"
$ResultAdapterAssemblyDll= Join-Path $ProxyDllResultDirectory "com.sos-berlin.jobscheduler.dotnet.adapter.dll"

$TargetDirectory         = Join-Path (Get-Location) "target"
$Jni4Net                 = Join-Path (Get-Location) "target\jni4net"
$Jni4NDllName            = "jni4net.n-0.8.8.0.dll"
$Jni4NetDlls             = @($Jni4NDllName, "jni4net.n.w32.v40-0.8.8.0.dll", "jni4net.n.w64.v40-0.8.8.0.dll")
$BuildDirectory          = mkdir (Join-Path $TargetDirectory "jni4net-build") -force

[Environment]::SetEnvironmentVariable("PATH", "${env:WINDOWS_NET_SDK_HOME};${env:JAVA_HOME}\bin;${env:PATH}");

function GenerateProxyJarAndDll() {
    # proxygen.exe wants a Jar named as the assembly
    $JobApiJar = Join-Path $BuildDirectory "$ProxyAssemblyBasename.jar"
    ExecuteCommand "jar" @("cf", """$JobApiJar""", "-C", $JobApiClassesDirectory, "sos/spooler")

    ExecuteCommand "$Jni4Net\bin\proxygen.exe" @("""$JobApiJar""", "-wd", """$BuildDirectory""")
}

function CopyJni4NetDlls() {
    $Jni4NetDlls | foreach {
        Copy-Item "$Jni4Net\lib\$_" $ProxyDllResultDirectory -ea Stop
    }
}

function CompileJobSchedulerAdapter() {
    # We simply mix our C# files with the generated ones to get a single DLL
    Copy-Item $DotnetJobSchedulerAdapterSourceDirectory (join-path $BuildDirectory "clr") -recurse -force -ea stop
    $powershellRef = [PsObject].Assembly.Location
    ExecuteCommand "csc" @("/nologo", "/warn:0", "/t:library", "/out:$ResultAdapterAssemblyDll",
                           "/recurse:""$BuildDirectory\clr\*.cs""",
                           "/reference:$powershellRef;""$Jni4Net\lib\$Jni4NDllName""")
}

function ExecuteCommand([string] $command, [Array]$arguments) {
    if ($arguments -ne $null) {
        $process = Start-Process $command -NoNewWindow -Wait -PassThru -ArgumentList $arguments
    } else {
        $process = Start-Process $command -NoNewWindow -Wait -PassThru
    }
    [Int32]$exitCode = $process.exitCode
    if ($exitCode -ne 0) {
        throw "Command failed with exit code ${exitCode}: $command"
    }
}

GenerateProxyJarAndDll
CopyJni4NetDlls
CompileJobSchedulerAdapter
