package com.sos.scheduler.engine.plugins.guicommand;

import java.util.Collection;
import com.sos.scheduler.engine.kernel.database.DatabaseConfiguration;
import java.sql.ResultSetMetaData;
import org.w3c.dom.Document;
import java.sql.ResultSet;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.CommandPlugin3;
import com.sos.scheduler.engine.kernel.plugin.PlugInCommandCommand;
import com.sos.scheduler.engine.kernel.plugin.PlugInCommandResult;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


//TODO SQL gegen alle möglichen Datenbanktypen prüfen (vielleicht JPA oder JPQL verwenden?
//TODO Ergebnis könnte sehr groß sein. C++ kopiert XML-Elemente mit clone(). Am besten auf TCP streamen.
public class GUICommandPlugin extends AbstractPlugin implements CommandPlugin3 {
    private final DatabaseConfiguration dbConfig;
    private Connection connection = null;


    GUICommandPlugin(DatabaseConfiguration config) {
        this.dbConfig = config;
    }


    @Override public final void close() {
        try {
            if (connection != null)  connection.close();
        } catch (SQLException x) { throw new SchedulerException(x); }
    }

    
    @Override public final PlugInCommandResult executeCommand(PlugInCommandCommand c) {
        Element result;
        Collection<Element> childElements = elementsXPath(c.getElement(), "*");
        if (childElements.size() != 1)  throw new SchedulerException("Invalid command " + c.getName());
        Element element = childElements.iterator().next();
        if (element.getNodeName().equals("showTaskHistory"))
            result = executeShowTaskHistory(element);
        else
        if (element.getNodeName().equals("test"))
            result = executeTest(element);
        else
            throw new SchedulerException("Invalid command " + c.getName());
        return new PlugInCommandResult(result);
    }


    private Element executeShowTaskHistory(Element element) {
        try {
            Document doc = newDocument();
            String sql = "select * from \"" + dbConfig.getTaskHistoryTablename() + "\"";
            return xmlFromSqlQuery(doc, sql);
        } catch (SQLException x) { throw new SchedulerException(x); }
    }


    private Element xmlFromSqlQuery(Document doc, String sql) throws SQLException {
        PreparedStatement stmt = getConnection().prepareStatement(sql);
        try {
            ResultSet resultSet = stmt.executeQuery();
            return xmlFromResultSet(doc, resultSet);
        } finally {
            stmt.close();
        }
    }


    private Element xmlFromResultSet(Document doc, ResultSet resultSet) throws SQLException {
        Element result = doc.createElement("myResult");
        ResultSetMetaData meta = resultSet.getMetaData();
        while (resultSet.next()) {
            Element e = doc.createElement("row");
            result.appendChild(e);
            for (int i = 1; i <= meta.getColumnCount(); i++) {
                String s = resultSet.getString(i);
                if (!resultSet.wasNull())
                    e.setAttribute(meta.getColumnName(i), s);
            }
        }
        return result;
    }


    private Element executeTest(Element element) {
        return newDocument().createElement("testResult");
    }


    private Connection getConnection() {
        try {
            if (connection == null)
                connection = DriverManager.getConnection(dbConfig.getUrl()); //, userName, password);
            return connection;
        } catch (SQLException x) { throw new SchedulerException(x); }
    }


	public static PlugInFactory factory() {
    	return new PlugInFactory() {
            @Override public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
                DatabaseSubsystem db = scheduler.getDatabaseSubsystem();
            	return new GUICommandPlugin(db.getConfiguration());
            }
        };
    }
}
