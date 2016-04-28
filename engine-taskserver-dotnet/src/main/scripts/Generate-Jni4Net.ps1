# .SYNOPSIS
#     Generate .NET and Java proxy files, JobScheduler .NET adapter dll and copy this and the jni4net .NET dlls to the given locations
# .DESCRIPTION
#     - generates two proxy files to the given locations and use "JarBaseName" basename as the proxy basename
#       1 - .jn4-<version>.dll
#       2 - .jn4-<version>.jar
#     - generates the JobScheduler .NET adapter .dll (com.sos-berlin.jobscheduler.dotnet.adapter-<version>.dll)
#     - copies jni4net .NET dll files from the "proxygen" directory to the DLL location
#
# .PARAMETER DotnetJobSchedulerAdapterSourceDirectory
#     Path to the .cs files from the .NET JobScheduler adapter implementation
# .PARAMETER OutputProxyDllDirectory
#     Output directory for the generated proxy .NET dll files
# .EXAMPLE
#     .\Generate-Jni4Net.ps1 "C:\Temp\com.sos-berlin.jobscheduler.engine.engine-job-api-1.10.3.jar" "C:\Temp\adapter_cs" "C:\Temp\proxy"
#          1) copied the generated .NET and Java proxy
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.dll
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.jar
#          2) copied the generated .NET adapter
#               com.sos-berlin.jobscheduler.dotnet.adapter-1.10.3.dll
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
	[parameter(Mandatory=$true)]
    [string] $DotnetJobSchedulerAdapterSourceDirectory,
    [parameter(Mandatory=$true)]
	[string] $OutputProxyDllDirectory,
    [parameter(Mandatory=$true)]
	[string] $OutputProxyJarDirectory
)
# ----------------------------------------------------------------------
# Globals
# ----------------------------------------------------------------------
$Global:FrameworkDirectory      = "C:\Windows\Microsoft.NET\Framework64\v4.0.30319"
$Global:JDKDirectory            = [Environment]::GetEnvironmentVariable("JAVA_HOME")
# ----------------------------------------------------------------------
# Script
# ----------------------------------------------------------------------
$Script:ProxyAssemblyBasename   = "com.sos-berlin.jobscheduler.dotnet.job-api.proxy" # This name is constant and can't be changed - the name defines the assembly name and is referenced by the another .dll files.
$Script:ProxyOutputNamePrefix   = "com.sos-berlin.jobscheduler."
$Script:AdapterAssemblyBasename = "com.sos-berlin.jobscheduler.dotnet.adapter"
$Script:TargetDirectory         = Join-Path (Get-Location) "\target"
$Script:ProxyGenExe             = Join-Path (Get-Location) "\target\jni4net\bin\proxygen.exe"
$Script:DllDirectory            = Join-Path (Get-Location) "\target\jni4net\lib"
$Script:Jni4NetReferenceDll     = Join-Path (Get-Location) "\target\jni4net\lib\jni4net.n-0.8.8.0.dll"
$Script:ExtractedJobApiDirectory = Join-Path (Get-Location) "\target\jni4net-input\javaClasses"
$Script:Jni4NetDlls             = @("jni4net.n-0.8.8.0.dll", "jni4net.n.w32.v40-0.8.8.0.dll", "jni4net.n.w64.v40-0.8.8.0.dll")
$Script:JDKBinDirectory         = Join-Path $Global:JDKDirectory "bin"
$Script:TempDirectory           = New-Item -Type Directory -Path (Join-Path (Get-Location) "\target\jni4net.tmp")
$Script:BuildDirectory          = Join-Path $Script:TempDirectory "build"
$Script:ProxyDll                = Join-Path $Script:BuildDirectory "${Script:ProxyAssemblyBasename}.j4n.dll"
$Script:JobApiJar               = Join-Path $Script:TempDirectory "${Script:ProxyAssemblyBasename}.jar"

[Environment]::SetEnvironmentVariable("PATH", "${Global:FrameworkDirectory};${Script:JDKBinDirectory};${env:Path}");

function CreateJobApiJar() {
    ExecuteCommand "jar" @("cf", """$Script:JobApiJar""", "-C", $Script:ExtractedJobApiDirectory, "sos/spooler")
}

function GenerateProxyJarAndDll() {
    ExecuteCommand $ProxyGenExe @("""$Script:JobApiJar""", "-wd", """${Script:BuildDirectory}""")

    Set-Location $Script:BuildDirectory
    ExecuteCommand (Join-Path $Script:BuildDirectory "build.cmd") $null;   # proxygen has generated build.cmd

    CopyGeneratedDll
    CopyGeneratedJavaClassFiles
}

function CopyGeneratedDll() {
    $targetDll = Join-Path $OutputProxyDllDirectory ($Script:ProxyOutputNamePrefix + "engine-job-api.j4n.dll")
    Copy-Item $Script:ProxyDll $targetDll -ea Stop
}

function CopyGeneratedJavaClassFiles() {
    $fromDir = Join-Path $Script:BuildDirectory "target/classes/sos/spooler"
    $toDir = Join-Path $Script:TargetDirectory "classes/sos/spooler"
    mkdir $toDir -ea stop
    dir -file $fromDir | foreach {
        Move-Item (Join-Path $fromDir $_) $toDir -ea stop
    }
}

function GenerateJobSchedulerAdapterDll() {
    $dll = Join-Path $OutputProxyDllDirectory ($Script:AdapterAssemblyBasename + ".dll")
    $powershellRef = [PsObject].Assembly.Location
    ExecuteCommand "csc" @("/nologo", "/warn:0", "/t:library", "/out:$dll",
                           "/recurse:""$DotnetJobSchedulerAdapterSourceDirectory\*.cs""",
                           "/reference:$powershellRef;$Script:Jni4NetReferenceDll;""$Script:ProxyDll""")
}

function CopyJni4NetDlls() {
    $Script:Jni4NetDlls | foreach {
        $from = Join-Path $Script:DllDirectory $_
        $to   = Join-Path $OutputProxyDllDirectory $_
        Copy-Item $from $to -ea Stop
    }
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

CreateJobApiJar
GenerateProxyJarAndDll
GenerateJobSchedulerAdapterDll
CopyJni4NetDlls
