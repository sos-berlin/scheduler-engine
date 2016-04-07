package com.sos.scheduler.engine.jobapi.dotnet;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public class DotnetAssemblySeacher {
	private final static int    DLL_CORE_SEARCH_MAX_DEPTH = 1;
	private final static String DLL_CORE_PREFIX = "com.sos-berlin.jobscheduler.";
	private final static String DLL_CORE_SUFFIX_JOB_API_PROXY = ".j4n.dll";
	private final static String DLL_CORE_SUFFIX_ADAPTER = ".adapter.dll";
	
	private Optional<Path> apiProxyDll = Optional.empty();
	private Optional<Path> adapterDll = Optional.empty();
	
	public void findAndSetCoreAssemblies(Path path) throws Exception{
		List<Path> list = findCoreFiles(path);
		if(list == null || list.size() < 2){
			throw new Exception(String.format("[%s] Not found JobScheduler .NET dlls: files like %s<xxx>",path.toString(),DLL_CORE_PREFIX));
		}
		
		apiProxyDll = list.stream().filter(p-> p.getFileName().toString().endsWith(DLL_CORE_SUFFIX_JOB_API_PROXY)).findFirst();
		if(!apiProxyDll.isPresent()){
			throw new Exception(String.format("[%s] Not found JobScheduler .NET api proxy dll: file like %s<xxx>%s",path.toString(),DLL_CORE_PREFIX,DLL_CORE_SUFFIX_JOB_API_PROXY));
		}
		adapterDll = list.stream().filter(p-> p.getFileName().toString().endsWith(DLL_CORE_SUFFIX_ADAPTER)).findFirst();
		if(!adapterDll.isPresent()){
			throw new Exception(String.format("[%s] Not found JobScheduler .NET adapter dll: file like %s<xxx>%s",path.toString(),DLL_CORE_PREFIX,DLL_CORE_SUFFIX_ADAPTER));
		}
	}
	
	private List<Path> findCoreFiles(Path path) throws Exception{
		try (Stream<Path> stream = Files.find(path, DLL_CORE_SEARCH_MAX_DEPTH, (p, attr) ->
		       p.getFileName().toString().startsWith(DLL_CORE_PREFIX) 
		       && 
		       (p.getFileName().toString().endsWith(DLL_CORE_SUFFIX_JOB_API_PROXY) 
		       || p.getFileName().toString().endsWith(DLL_CORE_SUFFIX_ADAPTER)))) {
			return stream.collect(Collectors.toList());
		}
	}
	
	public Optional<Path> getApiProxyDll(){
		return apiProxyDll;
	}
	
	public Optional<Path> getAdapterDll(){
		return adapterDll;
	}
}
