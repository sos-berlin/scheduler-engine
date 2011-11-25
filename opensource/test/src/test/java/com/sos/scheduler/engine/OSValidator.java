package com.sos.scheduler.engine;

/**
 * Methods to detect the operating system. 
 * 
 * Use the property <l>os.name</l> to detect the operating system.
 * 
 * 
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 25.11.2011 08:55:41
 * @see <a href="http://lopica.sourceforge.net/os.html">http://lopica.sourceforge.net/os.html</a> for a list of different values os.name.
 */
public class OSValidator {
	 
		public static boolean isWindows(){
			String os = System.getProperty("os.name").toLowerCase();
		    return (os.indexOf( "win" ) >= 0); 
		}
	 
		public static boolean isMac(){
			String os = System.getProperty("os.name").toLowerCase();
		    return (os.indexOf( "mac" ) >= 0); 
	 
		}
	 
		public static boolean isUnix(){
			String os = System.getProperty("os.name").toLowerCase();
		    return (os.indexOf( "nix") >=0 || os.indexOf( "nux") >=0);
	 
		}

	}
