# .SYNOPSIS
#     Generate .NET and Java proxy files, JobScheduler .NET adapter dll and copy this and the jni4net .NET dlls to the given locations
# .DESCRIPTION
#     the Script execute
#     - create a working temp folder in the user TEMP directory 
#     - extract version (if exists) from "InputJobApiJar"
#     - generate two proxy files to the given locations and use "InputJobApiJar" basename as the proxy basename
#       1 - .jn4-<version>.dll
#       2 - .jn4-<version>.jar
#     - generate the JobScheduler .NET adapter .dll (com.sos-berlin.jobscheduler.dotnet.adapter-<version>.dll)
#     - copied jni4net .NET dll files from the "proxygen" directory to the DLL location
#     - remove the working temp folder in the user TEMP directory
#
# .PARAMETER InputJobApiJar
#     Path to the .jar file with the job api (sos.spooler.*) implementation
# .PARAMETER DotnetJobSchedulerAdapterSourceDirectory
#     Path to the .cs files from the .NET JobScheduler adapter implementation
# .PARAMETER OutputDirectoryProxyDll
#     Output directory for the generated proxy .NET dll files
# .PARAMETER OutputDirectoryProxyJar
#     Output directory for the generated proxy Java jar file
#     Default(empty) - OutputDirectoryProxyDll will be used 
# .EXAMPLE
#     .\Generate-Jni4Net.ps1 "C:\Temp\com.sos-berlin.jobscheduler.engine.engine-job-api-1.10.3.jar" "C:\Temp\adapter_cs" "C:\Temp\proxy"
#          1) copied the generated .NET and Java proxy 
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.dll
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.jar
#          2) copied the generated .NET adapter
#               com.sos-berlin.jobscheduler.dotnet.adapter-1.10.3.dll
#          3) copied the existing jni4net .NET dll files from the "proxygen" location 
#          to the "C:\Temp\proxy" location
#     .\Generate-Jni4Net.ps1 "C:\Temp\com.sos-berlin.jobscheduler.engine.engine-job-api-1.10.3.jar" "C:\Temp\adapter_cs" "C:\Temp\proxy_dll" "C:\Temp\proxy_jar"
#          1) copied the generated .NET proxy 
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.dll
#             to the "C:\Temp\proxy_dll" location
#          2) copied the generated Java proxy
#               com.sos-berlin.jobscheduler.engine.engine-job-api.j4n-1.10.3.jar
#             to the "C:\Temp\proxy_jar" location
#          3) copied the generated .NET adapter
#               com.sos-berlin.jobscheduler.dotnet.adapter-1.10.3.dll
#             to the "C:\Temp\proxy_dll" location
#          4) copied the existing jni4net .NET dll files from the "proxygen" location 
#             to the "C:\Temp\proxy_dll" location
# ----------------------------------------------------------------------
# Command Line Arguments
# ----------------------------------------------------------------------
param(
	[parameter(Mandatory=$true)]
    [string] $InputJobApiJar,
	[parameter(Mandatory=$true)]
    [string] $DotnetJobSchedulerAdapterSourceDirectory,
    [parameter(Mandatory=$true)]
	[string] $OutputDirectoryProxyDll,
	[string] $OutputDirectoryProxyJar
)
# ----------------------------------------------------------------------
# Globals
# ----------------------------------------------------------------------
$Global:FrameworkDirectory      = "C:\Windows\Microsoft.NET\Framework64\v4.0.30319"
$Global:JDKDirectory            = [Environment]::GetEnvironmentVariable("JAVA_HOME")
# ----------------------------------------------------------------------
# Environment
# ----------------------------------------------------------------------
$env:APP_SCRIPT                 = $MyInvocation.MyCommand.Name
$env:APP_PATH                   = Split-Path $MyInvocation.MyCommand.Path
# ----------------------------------------------------------------------
# Script
# ----------------------------------------------------------------------
$Script:ProxyAssemblyBasename   = "com.sos-berlin.jobscheduler.dotnet.job-api.proxy" # This name is constant and can't be changed - the name defines the assembly name and is referenced by the another .dll files.
$Script:ProxyOutputNamePrefix   = "com.sos-berlin.jobscheduler."
$Script:AdapterAssemblyBasename = "com.sos-berlin.jobscheduler.dotnet.adapter"       # 
$Script:DllDirectory            = Join-Path (Get-Location) "\target\jni4net\lib"
$Script:ProxyGenExe             = Join-Path (Get-Location) "\target\jni4net\bin\proxygen.exe"
$Script:Jni4NetDlls             = @("jni4net.n-0.8.8.0.dll","jni4net.n.w32.v40-0.8.8.0.dll", "jni4net.n.w64.v40-0.8.8.0.dll")
$Script:Jni4NetReferenceDll     = Join-Path $OutputDirectoryProxyDll "jni4net.n-0.8.8.0.dll"
$Script:JDKBinDirectory         = Join-Path $Global:JDKDirectory "bin"
$Script:CopiedFiles             = @()
$Script:ErrorExitCode           = 99
$Script:TempDirectory           = $null
$Script:InputApiJar             = $null
$Script:ApiJarVersion           = $null
$Script:TempApiJar              = $null
$Script:OutDirProxyDll          = $null
$Script:OutDirProxyJar          = $null
$Script:DotnetAdapterSourceDir  = $null
$Script:ProxyDll                = $null
$Script:ProxyJar                = $null
# ----------------------------------------------------------------------
# Implementation
# ----------------------------------------------------------------------
function Init([string] $outDirProxyDll,[string] $outDirProxyJar,[string] $dotnetAdapterSourceDirectory){
    $newPath = $Global:FrameworkDirectory+";"+$Script:JDKBinDirectory+";"+$env:Path;
    [Environment]::SetEnvironmentVariable("PATH", $newPath)
    
    $Script:OutDirProxyDll = $outDirProxyDll
    $Script:OutDirProxyJar = $outDirProxyJar
    
    if([string]::IsNullOrEmpty($Script:OutDirProxyDll)){
        $Script:OutDirProxyDll = $env:APP_PATH
    }
    if([string]::IsNullOrEmpty($Script:OutDirProxyJar)){
        $Script:OutDirProxyJar = $Script:OutDirProxyDll
    }
    
    $Script:DotnetAdapterSourceDir = $dotnetAdapterSourceDirectory
}
function CreateTempDirectory(){
    try
    {
        $tempDirName            = "dotnet-proxy-"+[System.Guid]::NewGuid().ToString();
        $tempDirPath            = Join-Path $env:Temp $tempDirName
        $Script:TempDirectory   = New-Item -Type Directory -Path $tempDirPath -ea Stop
    }
    catch{
        throw "[CreateTempDirectory] $($_.Exception.ToString())"
    }
}
function RemoveTempDirectory(){
    try
    {
        Set-Location $env:APP_PATH
    
        if(Test-Path($Script:TempDirectory.Fullname)){
            Remove-Item -path $Script:TempDirectory.Fullname -Recurse -Force -ea Stop
        }
    }
    catch{
        throw "[RemoveTempDirectory] $($_.Exception.ToString())"
    }
}
function CreateNewApiJarForProxy($jobApiJar){
    try
    {
        $Script:InputApiJar = Get-Item $jobApiJar
    
        SetApiJarVersion
    
        Set-Location $Script:TempDirectory
    
        [Array]$arguments   = "xvf",$jobApiJar
        ExecuteCommand "jar" $arguments;
    
        if(Test-Path("com")){
            Remove-Item "com" -Recurse -Force -ea Stop
        }
        if(Test-Path("META-INF")){
            Remove-Item "META-INF" -Recurse -Force -ea Stop
        }
        if(Test-Path("sos\spooler\jobs")){
            Remove-Item "sos\spooler\jobs" -Recurse -Force -ea Stop
        }
        if(Test-Path("sos\spooler\Spooler_program.class")){
            Remove-Item "sos\spooler\Spooler_program.class" -Force -ea Stop
        }
    
        $name = $Script:ProxyAssemblyBasename+".jar"
        [Array]$arguments = "cvf",$name,"sos\spooler"
        ExecuteCommand "jar" $arguments;
    
        Remove-Item "sos" -Recurse -Force -ea Stop
    
        $Script:TempApiJar  = Get-Item $name
    }
    catch{
        throw "[CreateNewApiJarForProxy] $($_.Exception.ToString())"
    }
}

function GenerateProxy(){
    try
    {
        Set-Location $env:APP_PATH
    
        $build              = Join-Path $Script:TempDirectory.Fullname "build"
        [Array]$arguments   = """$Script:TempApiJar""","-wd","""$build"""
        ExecuteCommand $ProxyGenExe $arguments;
    
        Set-Location $build
        $buildCommand       = Join-Path $build "build.cmd"
        ExecuteCommand $buildCommand $null;
        
        CopyGeneratedProxyFiles $build
    }
    catch{
        throw "[GenerateProxy] $($_.Exception.ToString())"
    }
}

function CopyGeneratedProxyFiles([string] $dir){
    try
    {
        Set-Location $env:APP_PATH
    
        $generatedTempDllName   = $Script:TempApiJar.Basename+".j4n.dll"
        $generatedTempJarName   = $Script:TempApiJar.Basename+".j4n.jar"
        
        $Script:ProxyDll    = Join-Path $dir $generatedTempDllName
        $Script:ProxyJar    = Join-Path $dir $generatedTempJarName
        
        $targetDllName      = RemoveInputApiJarVersion $Script:InputApiJar.Basename
        $targetDllName     += ".j4n"
        $targetDllName      = AddInputApiJarVersion $targetDllName "dll"
        if(!($targetDllName.StartsWith($Script:ProxyOutputNamePrefix))){
            $targetDllName = $Script:ProxyOutputNamePrefix+$targetDllName
        }
    
        $targetJarName      = RemoveInputApiJarVersion $Script:InputApiJar.Basename
        $targetJarName     += ".j4n"
        $targetJarName      = AddInputApiJarVersion $targetJarName "jar"
        if(!($targetJarName.StartsWith($Script:ProxyOutputNamePrefix))){
            $targetJarName = $Script:ProxyOutputNamePrefix+$targetJarName
        }
    
        $targetDll          = Join-Path $Script:OutDirProxyDll $targetDllName
        $targetJar          = Join-Path $Script:OutDirProxyJar $targetJarName
    
        if(Test-Path($targetDll)){
            Remove-Item $targetDll -ea Stop
        }
        if(Test-Path($targetJar)){
            Remove-Item $targetJar -ea Stop
        }
        
        Copy-Item $Script:ProxyDll $targetDll -ea Stop
        $Script:CopiedFiles += $targetDll
        Copy-Item $Script:ProxyJar $targetJar -ea Stop
        $Script:CopiedFiles += $targetJar
    }
    catch{
        throw "[CopyGeneratedProxyFiles] $($_.Exception.ToString())"
    }
}

function SetApiJarVersion(){
    try
    {
        $arr  = $Script:InputApiJar.Basename.Split("-")
        $last = $arr[$arr.length-1];
        $add  = "";
        if($last.ToLower().equals("snapshot")){
            $vers   = $arr[$arr.length-2];
            $add    = "-"+$last;
        }
        else{
            $vers   = $arr[$arr.length-1];
        }
        if ($vers.Replace(".","") -match "^[\d\.]+$"){
            $Script:ApiJarVersion = $vers+$add
        }
    }
    catch{
        throw "[SetApiJarVersion] $($_.Exception.ToString())"
    }
}

function GenerateJobSchedulerAdapterDll($proxyDll){
    try{
        $adapterName    = $Script:AdapterAssemblyBasename+".dll"
        $adapter        = Join-Path $Script:OutDirProxyDll $adapterName
        $powershellRef  = [PsObject].Assembly.Location
    
        if(Test-Path($adapter)){
            Remove-Item $adapter -ea Stop
        }
    
        $command            = "csc" 
        [Array]$arguments   = "/nologo","/warn:0","/t:library","/out:$adapter","/recurse:""$Script:DotnetAdapterSourceDir\*.cs""","/reference:$powershellRef;$Script:Jni4NetReferenceDll;""$Script:ProxyDll"""
        ExecuteCommand $command $arguments
        $Script:CopiedFiles += $adapter
        
        if(![string]::IsNullOrEmpty($Script:ApiJarVersion)) {   
            $adapterName    = AddInputApiJarVersion $Script:AdapterAssemblyBasename "dll"
            $adapterNewPath = Join-Path $Script:OutDirProxyDll $adapterName
            if(Test-Path($adapterNewPath)){
                Remove-Item $adapterNewPath -ea Stop
            }
            Move-Item $adapter $adapterNewPath -ea Stop
            $Script:CopiedFiles += $adapterNewPath
        }
    }
    catch{
        throw "[GenerateJobSchedulerAdapterDll] $($_.Exception.ToString())"
    }
}

function AddInputApiJarVersion([string] $basename,[string] $extension){
    if([string]::IsNullOrEmpty($Script:ApiJarVersion)) {
        return $basename+"."+$extension
    }
    else{
        return $basename+"-"+$Script:ApiJarVersion+"."+$extension
    }
}

function RemoveInputApiJarVersion([string] $basename){
    if([string]::IsNullOrEmpty($Script:ApiJarVersion)) {
        return $basename
    }
    else{
        return $basename.Replace("-"+$Script:ApiJarVersion,"")
    }
}

function CopyJni4NetDlls(){
    try{
        $Script:Jni4NetDlls |foreach {
            $oldDll = Join-Path $Script:OutDirProxyDll $_
            $dll        = Join-Path $Script:DllDirectory $_
            $dllCopied  = Join-Path $Script:OutDirProxyDll $_
            if(Test-Path($oldDll)){
                Remove-Item $oldDll -ea Stop
            }
            if(Test-Path($dllCopied)){
                Remove-Item $dllCopied -ea Stop
            }
            
            Copy-Item $dll $dllCopied -ea Stop
            $Script:CopiedFiles += $dllCopied
        }
    }
    catch{
        throw "[CopyJni4NetDlls] $($_.Exception.ToString())"
    }
}

function ExecuteCommand([string] $command, [Array]$arguments){
    try
    {
        if($arguments -ne $null){
            $process = Start-Process $command -NoNewWindow -ArgumentList $arguments -Wait -PassThru 
        }
        else{
            $process = Start-Process $command -NoNewWindow -Wait -PassThru 
        }
        [Int32]$exitCode = $process.exitCode
        if($exitCode -eq 0)
        {
        }
        else
        {
            throw "exit code $exitCode"
        }
    }
    catch{
        throw "[ExecuteCommand] Command $command ends with error $($_.Exception.ToString())"
    }
}
function RemoveCopiedFiles(){
    try
    {
        $Script:CopiedFiles |foreach {
            if(Test-Path($_)){
                Remove-Item $_ -ea Stop
            }
        }
    }
    catch{
        throw "[RemoveCopiedFiles] $($_.Exception.ToString())"
    }
}
# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------
try{
    Init $OutputDirectoryProxyDll $OutputDirectoryProxyJar $DotnetJobSchedulerAdapterSourceDirectory
    CreateTempDirectory
    CreateNewApiJarForProxy $InputJobApiJar
    GenerateProxy
    GenerateJobSchedulerAdapterDll
    CopyJni4NetDlls
}
catch{
    Write-Error "Error occurred: $($_.Exception.ToString())"
    try
    {
        RemoveCopiedFiles
    }
    catch{
        Write-Error "Error occurred on remove copied files : $($_.Exception.ToString())"
    }
    exit $Script:ErrorExitCode
}
finally{
    RemoveTempDirectory
}

