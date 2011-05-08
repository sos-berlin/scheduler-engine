import java.io.FileReader;
import java.io.IOException;
import java.util.HashSet;
import java.util.Iterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.log4j.Logger;
import org.w3c.dom.Document;
import org.w3c.dom.Element;


public class ProjectMap extends XMLDom {

	private static final Logger logger = Logger.getLogger(ProjectMap.class);
	private final String fileName;
	private final String content;
	
	private HashSet<Project> projects = new HashSet<Project>();
	
	public static void main(String[] args) {
		
		if (args.length != 1) System.err.println("usage: ProjectMap <full name of the projectmap (e.g. scheduler.sln)>");

		// "C:/Users/schaedi/Documents/C++/scheduler/scheduler.src.release/prod/scheduler.sln"
		String filename = args[0];
		String resultfilename = filename + "/end/";
		resultfilename = resultfilename.replaceAll(".sln/end/", "_projectmap.xml");
		ProjectMap pm = new ProjectMap(filename);
		
		Document doc = ProjectMap.getDocument();
		Element root = pm.createDom(doc);
		doc.appendChild(root);
		pm.writeToFile(doc, resultfilename);
	}
	
	public ProjectMap(String fileName) {
		this.fileName = fileName;				// Name der Projektmappe
		logger.debug("the name of the project-map is " + fileName);
		String txt = null;
		try {
			txt = file2String(fileName);
		} catch (IOException e) {
			logger.error("file " + fileName + " does not exist."); 
		}
		this.content = txt;
		
		Pattern p = Pattern.compile("\nProject|\nEndProject");
		Matcher m = p.matcher(this.content);
		boolean flgProjectStart = true;
		int ixStart = 0;
		while (m.find() ) {
			if (flgProjectStart) {
				ixStart = m.start();
			} else {
				projects.add( new Project( content.substring(ixStart, m.end()) ) );
			}
			flgProjectStart = !flgProjectStart;
		}

		// Die abh�ngigen Projekte setzen
		for (Iterator<Project> iterator = projects.iterator(); iterator.hasNext();) {
			Project project = iterator.next();
			project.setDependencies(projects);
		}
	}
	
	public String getFilename() {
		return fileName;
	}
	
	public HashSet<Project> getProjects() {
		return projects;
	}

    /*
     * Datei in String einlesen
     */
    private static String file2String(String filename) throws IOException {
       FileReader in = new FileReader(filename);
       String str = file2String(in);
       in.close();
       return str;
    }


    /*
     * Datei in String einlesen
     */
    private static String file2String(FileReader in) throws IOException {
       StringBuilder str = new StringBuilder();
       int countBytes = 0;
       char[] bytesRead = new char[512];
       while( (countBytes = in.read(bytesRead)) > 0) 
          str.append(bytesRead, 0, countBytes);
       return str.toString();      
    }


	public Element createDom(Document doc) {
		Element node = doc.createElement("projects");
		for (Iterator<Project> iterator = projects.iterator(); iterator.hasNext();) {
			Project p = iterator.next();
			Element pnode = p.createDom(doc);
			node.appendChild(pnode);
		}
		return node;
	}


}
