package com.sos.scheduler.engine.jobapi.dotnet;

import java.util.Optional;

import system.Type;
import system.Object;
import system.reflection.ConstructorInfo;
import system.reflection.MethodInfo;

public class DotnetInvoker {
	
	public static Object createInstance(Type type,Type[] paramTypes,Object[] params) throws Exception{
		ConstructorInfo ci = Optional.ofNullable(type.GetConstructor(paramTypes))
				.orElseThrow(() -> new Exception(String.format("[%s] Can't find the constructor with the specified parameters",type.getAssembly().getLocation())));
		return ci.Invoke(params);
	}
	
	public static system.Object invokeMethod(Type type,Object instance,String methodName,Type[] paramTypes,Object[] params) throws Exception{
		MethodInfo mi = Optional.ofNullable(type.GetMethod(methodName,paramTypes))
				.orElseThrow(() -> new Exception(String.format("[%s] Can't find the method %s with the specified parameters",type.getAssembly().getLocation() ,methodName)));
		
		return mi.Invoke(instance,params);
	}
	
	public static system.Object invokeMethod(Type type,Object instance,String methodName,String value) throws Exception{
		MethodInfo mi = Optional.ofNullable(type.GetMethod(methodName, new system.Type[]{system.Type.GetType("System.String")}))
				.orElseThrow(() -> new Exception(String.format("[%s] Can't find the method %s with the specified parameters",type.getAssembly().getLocation() ,methodName)));
		
		return mi.Invoke(instance, new system.Object[]{new system.String(value)});
	}
	
	public static system.Object invokeMethod(Type type,Object instance,String methodName) throws Exception{
		MethodInfo mi = Optional.ofNullable(type.GetMethod(methodName, new system.Type[]{}))
				.orElseThrow(() -> new Exception(String.format("[%s] Can't find the method %s with the specified parameters",type.getAssembly().getLocation() ,methodName)));
			
		return mi.Invoke(instance, null);
	}
	
	public static boolean invokeMethod(Type type,Object instance,String methodName,boolean defaultValue) throws Exception{
		return getReturnValue(invokeMethod(type,instance,methodName), defaultValue);
	}
	
	public static boolean getReturnValue(system.Object obj, boolean defaultValue){
		 if (obj.GetType().getName().equalsIgnoreCase("boolean")) {
             return Boolean.parseBoolean(obj.toString());
		 }
		return defaultValue; 
	}
}
