import java.util.HashSet;
import java.util.Iterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class Project extends XMLDom {

	private String guid;
	private String name;
	private String location;
	private String projectpath;
	private HashSet<String> depends = new HashSet<String>();
	HashSet<Dependency> projects = new HashSet<Dependency>();

	public Project(String projectdef) {
		
		Pattern p = Pattern.compile("\nProject[\\S]*[\\W]*([\\w\\\\\\.]*)[\\W]*([\\w\\\\\\.]*)[\\W]*([\\w-]*)");
		Matcher m = p.matcher(projectdef);
		while (m.find() ) {
			this.name = m.group(1);
			this.location = m.group(2);
			this.guid = m.group(3);
		}
		projectpath = location.substring(0, location.lastIndexOf('\\') );
		
		// Projektabhängigkeiten
		p = Pattern.compile("ProjectSection\\(ProjectDependencies\\) = postProject|EndProjectSection");
		m = p.matcher(projectdef);
		boolean flgStart = true;
		int ixStart = 0;
		while (m.find()) {
			if (flgStart) {
				ixStart = m.start();
			} else {
				String content = projectdef.substring(ixStart,m.end());
				p = Pattern.compile("\\{([0-9A-Z-]*)\\} = \\{([0-9A-Z-]*)\\}");
				m = p.matcher(content);
				while (m.find() ) {
					depends.add(m.group(1));
				}
			}
			flgStart = !flgStart;
		}

	}
	
	public void setDependencies(HashSet<Project> allprojects) {
		for (Iterator<Project> iterator = allprojects.iterator(); iterator.hasNext();) {
			Project p = iterator.next();
			if (depends.contains(p.getGuid())) projects.add( new Dependency(p) );
		}
	}

	public Element createDom(Document doc) {
		Element node = doc.createElement("project");
		node.setAttribute("name", getName());
		node.setAttribute("location", getLocation());
		node.setAttribute("path", getProjectPath());
		node.setAttribute("guid", getGuid());
		if (projects.size() > 0) {
			Element dnode = doc.createElement("dependencies");
			for (Iterator<Dependency> iterator = projects.iterator(); iterator.hasNext();) {
				Dependency d = iterator.next();
				dnode.appendChild(d.createDom(doc));
			}
			node.appendChild(dnode);
		}
		return node;
	}

	public String getGuid() {
		return guid;
	}

	public String getName() {
		return name;
	}

	public String getLocation() {
		return location;
	}

	public String getProjectPath() {
		return projectpath;
	}
}
