import org.w3c.dom.Document;
import org.w3c.dom.Element;


public class Dependency extends XMLDom {

	private Project project;

	public Dependency(Project project) {
		this.project = project;
	}
	
	public Element createDom(Document doc) {
		Element node = doc.createElement("dependency");
		node.setAttribute("name", project.getName() );
		return node;
	}
	
}
