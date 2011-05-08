<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:jobdoc="http://www.sos-berlin.com/schema/scheduler_job_documentation_v1.0" xmlns="http://www.w3.org/1999/xhtml" xmlns:xhtml="http://www.w3.org/1999/xhtml" exclude-result-prefixes="jobdoc xhtml" >
	<xsl:output method="xml" omit-xml-declaration="no" version="1.0" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd" indent="yes" encoding="ISO-8859-1" />
	<!-- Wurzel, hier: Grundgerüst der Seite/ Reihenfolge der anderen Template-Aufrufe -->
	
   <xsl:param name="P_WIKI_LANGUAGE" select="'de'"/>
   <xsl:param name="P_WIKI_LANGUAGE_ALT1" select="'en'"/>
   <xsl:param name="P_WIKI_LANGDESC_ALT1" select="'englisch'"/>
   <xsl:param name="P_WIKI_GROUP"    select="'Job'"/>
   <xsl:param name="P_WIKI_PREFIX"   select="''"/>

	<xsl:template match="/jobdoc:description">
	  <html>
			<head>
				<meta http-equiv="Content-Style-Type" content="text/css"/>
				<meta name="author" content="SOS GmbH"/>
				<meta name="publisher" content="Software- und Organisations- Service GmbH (SOS), Berlin"/>
				<meta name="copyright" content="Copyright 2006 SOFTWARE UND ORGANISATIONS-SERVICE GmbH (SOS), Berlin. All rights reserved."/>
				<xsl:choose>
				  <xsl:when test="jobdoc:job/@order">
				    <title>SOS Scheduler Job Documentation</title>
				  </xsl:when>
				  <xsl:otherwise>
				    <title>SOS Documentation</title>
				  </xsl:otherwise>
				</xsl:choose>
				<xsl:call-template name="get_css"/>
				<xsl:call-template name="get_js"/>
			</head>
			<body>
				<xsl:call-template name="main">
				  <xsl:with-param name="lang" select="$P_WIKI_LANGUAGE" />
				</xsl:call-template>
			</body>
		</html>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template MAIN -->
	<xsl:template name="main">
		<xsl:param name="lang"/>
		<div>
			<xsl:attribute name="id">lang_<xsl:value-of select="$lang"/></xsl:attribute>
			<xsl:call-template name="navigation"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
			<xsl:apply-templates select="jobdoc:job"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
			<xsl:apply-templates select="jobdoc:documentation[@language=$lang]"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
			<xsl:call-template name="genXML"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
			<xsl:apply-templates select="jobdoc:releases"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
			<xsl:apply-templates select="jobdoc:resources"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
			<xsl:apply-templates select="jobdoc:configuration"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
		</div>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Navigation -->
	<xsl:template name="navigation">
		<xsl:param name="lang"/>
		<table class="navi" width="100%">
			<tr>
				<td nowrap="nowrap">
					<xsl:if test="jobdoc:documentation">
					  <a class="navi" href="#">
					  	<xsl:attribute name="onclick">show_div('documentation','<xsl:value-of select="$lang"/>');return false;</xsl:attribute>
					  	<xsl:call-template name="get_label">
					  		<xsl:with-param name="lang" select="$lang"/>
					  		<xsl:with-param name="label_de" select="'Dokumentation'"/>
					  		<xsl:with-param name="label_en" select="'Documentation'"/>
					  	</xsl:call-template>
					  </a>
            |
          </xsl:if>
          <xsl:if test="jobdoc:releases">
            <a class="navi" href="#">
					  	<xsl:attribute name="onclick">show_div('firstRelease','<xsl:value-of select="$lang"/>');return false;</xsl:attribute>
            Releases
            </a>
            |
          </xsl:if>
          <xsl:if test="jobdoc:resources">
            <a class="navi" href="#">
					  	<xsl:attribute name="onclick">show_div('resources','<xsl:value-of select="$lang"/>');return false;</xsl:attribute>
					  	<xsl:call-template name="get_label">
					  		<xsl:with-param name="lang" select="$lang"/>
					  		<xsl:with-param name="label_de" select="'Ressourcen'"/>
					  		<xsl:with-param name="label_en" select="'Resources'"/>
					  	</xsl:call-template>
					  </a>
            |
          </xsl:if>
          <xsl:if test="jobdoc:configuration">
            <a class="navi" href="#">
					  	<xsl:attribute name="onclick">show_div('configuration','<xsl:value-of select="$lang"/>');return false;</xsl:attribute>
					  	<xsl:call-template name="get_label">
					  		<xsl:with-param name="lang" select="$lang"/>
					  		<xsl:with-param name="label_de" select="'Konfiguration'"/>
					  		<xsl:with-param name="label_en" select="'Configuration'"/>
					  	</xsl:call-template>
					  </a>
            |
          </xsl:if>
          <a class="navi" href="#">
						<xsl:attribute name="onclick">show_div('all','<xsl:value-of select="$lang"/>');return false;</xsl:attribute>
						<xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Alles anzeigen'"/>
							<xsl:with-param name="label_en" select="'Show all'"/>
						</xsl:call-template>
					</a>
				</td>
				<td style="text-align:center">
				  <xsl:if test="jobdoc:job/@order">
					  <a class="navi" href="#">
              <xsl:attribute name="onclick">genXML('<xsl:value-of select="$lang"/>');return false;</xsl:attribute>
              <xsl:call-template name="get_label">
                <xsl:with-param name="lang" select="$lang"/>
                <xsl:with-param name="label_de" select="'Job generieren'"/>
                <xsl:with-param name="label_en" select="'Generate Job'"/>
              </xsl:call-template>
            </a>
					  <xsl:text> </xsl:text>
					  <xsl:text> </xsl:text>
					</xsl:if>
				</td>
				<td style="text-align:right">
					<a class="lang"><xsl:attribute name="href"><xsl:value-of select="concat('../scheduler.',$P_WIKI_LANGUAGE_ALT1,'/index.php?n=',$P_WIKI_GROUP,'.',$P_WIKI_PREFIX,jobdoc:job/@name)" /></xsl:attribute>
					<img border="0">
                   <xsl:attribute name="src"><xsl:value-of select="concat('banner_',$P_WIKI_LANGUAGE_ALT1,'.gif')" /></xsl:attribute>
                   <xsl:attribute name="alt"><xsl:value-of select="$P_WIKI_LANGDESC_ALT1" /></xsl:attribute>
                   <xsl:attribute name="title"><xsl:value-of select="$P_WIKI_LANGDESC_ALT1" /></xsl:attribute>
               </img>                   
					</a>
				</td>
			</tr>
		</table>
		<p></p>
	</xsl:template>

	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template XML für Job-KOnfiguration generieren -->
	<xsl:template name="genXML">
		<xsl:param name="lang"/>
		<table class="box" style="display:none">
			<xsl:attribute name="id">genXML_<xsl:value-of select="$lang"/></xsl:attribute>
			<tr>
				<td class="td1"><span class="section">XML</span></td>
				<td class="td2_3" colspan="2">
				  <xsl:call-template name="get_label">
				  	<xsl:with-param name="lang" select="$lang"/>
				  	<xsl:with-param name="label_de" select="'Code zur Konfiguration des Jobs - [kann via Zwischenablage in die Datei scheduler.xml eingefügt werden]'"/>
				  	<xsl:with-param name="label_en" select="'Code to configurate the job - [can be inserted into file scheduler.xml via clipboard]'"/>
				  </xsl:call-template>
  				<hr/>
				</td>
			</tr>
			<tr>
				<td class="td1"><xsl:text> </xsl:text></td>
				<td class="td2_3" colspan="2">
        <xsl:apply-templates select="jobdoc:job" mode="genXML"/>
				</td>
			</tr>
		</table>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Job -->
	<xsl:template match="jobdoc:job">
		<xsl:param name="lang"/>
		<table class="box">
			<xsl:attribute name="id">job_<xsl:value-of select="$lang"/></xsl:attribute>
			<tr>
				<td class="td1">
				  <xsl:choose>
				    <xsl:when test="@order">
				      <span class="section">Job</span>
				    </xsl:when>
				    <xsl:otherwise>
					    <span class="section">Class</span>
					  </xsl:otherwise>
					</xsl:choose>
				</td>
				<td class="td2">
					<span class="label">
						<xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Name/ Titel'"/>
							<xsl:with-param name="label_en" select="'Name/ Title'"/>
						</xsl:call-template>
					</span>
				</td>
				<td class="td3">
					<span class="sourceNameBold"><xsl:value-of select="@name"/></span>
					<xsl:text> </xsl:text>
					<xsl:text> </xsl:text>
					<span class="desc"><xsl:value-of select="@title"/></span>
				</td>
			</tr>
			<xsl:if test="@order">
			  <tr>
			  	<td class="td1">
			  		<xsl:text> </xsl:text>
			  	</td>
			  	<td class="td2">
				    <xsl:attribute name="onmouseout">hideWMTT()</xsl:attribute>
				    <xsl:attribute name="onmouseover">showWMTT('ttorderControl_<xsl:value-of select="$lang"/>')</xsl:attribute>
  				  <xsl:call-template name="get_label">
					  	<xsl:with-param name="lang" select="$lang"/>
					  	<xsl:with-param name="label_de" select="'Auftragssteuerung'"/>
					  	<xsl:with-param name="label_en" select="'Order Control'"/>
					  </xsl:call-template>
						<span class="tooltip">
							<xsl:attribute name="id">ttorderControl_<xsl:value-of select="$lang"/></xsl:attribute>
							<xsl:call-template name="get_label">
								<xsl:with-param name="lang" select="$lang"/>
								<xsl:with-param name="label_de" select="'Ein Auftrag aktiviert die Verarbeitung einer Job-Kette. Er enthält Parameter für einen oder mehrere Jobs einer Job-Kette.'"/>
								<xsl:with-param name="label_en" select="'An order activates the processing of a job chain and contains parameters for one or more jobs in the job chain.'"/>
							</xsl:call-template>
						</span>
			  	</td>
			  	<td class="td3">
			  		<span class="desc">
			  			<xsl:if test="@order='yes'">
			  				<xsl:call-template name="get_label">
			  					<xsl:with-param name="lang" select="$lang"/>
			  					<xsl:with-param name="label_de" select="'Dieser Job läuft auftragsgesteuert ab.'"/>
			  					<xsl:with-param name="label_en" select="'This job is executed order driven, i.e. it starts automatically if an order is received.'"/>
			  				</xsl:call-template>
			  			</xsl:if>
			  			<xsl:if test="@order='no'">
			  				<xsl:call-template name="get_label">
			  					<xsl:with-param name="lang" select="$lang"/>
			  					<xsl:with-param name="label_de" select="'Dieser Job verarbeitet keine Aufträge.'"/>
			  					<xsl:with-param name="label_en" select="'This job does not process orders.'"/>
			  				</xsl:call-template>
			  			</xsl:if>
			  			<xsl:if test="@order='both'">
			  				<xsl:call-template name="get_label">
			  					<xsl:with-param name="lang" select="$lang"/>
			  					<xsl:with-param name="label_de" select="'Dieser Job kann durch Aufträge oder durch Job-Starts veranlasst ablaufen.'"/>
			  					<xsl:with-param name="label_en" select="'This job is triggered by orders or by standard job starts.'"/>
			  				</xsl:call-template>
			  			</xsl:if>
			  		</span>
			  	</td>
			  </tr>
			</xsl:if>
			<xsl:if test="@tasks">
			  <tr>
			  	<td class="td1">
			  		<xsl:text> </xsl:text>
			  	</td>
			  	<td class="td2" onmouseout="hideWMTT()" >
			  		<xsl:attribute name="onmouseover">showWMTT('tttasks_<xsl:value-of select="$lang"/>')</xsl:attribute>
              Tasks
			  			<span class="tooltip">
			  				<xsl:attribute name="id">tttasks_<xsl:value-of select="$lang"/></xsl:attribute>
			  				<xsl:call-template name="get_label">
			  					<xsl:with-param name="lang" select="$lang"/>
			  					<xsl:with-param name="label_de" select="'zeigt die maximale Anzahl der Tasks an, die ein Job parallel ausführen kann'"/>
			  					<xsl:with-param name="label_en" select="'shows the maximum number of tasks that a job can operate in parallel'"/>
			  				</xsl:call-template>
			  			</span>
			  	</td>
			  	<td class="td3">
			  		<span class="desc"><xsl:value-of select="@tasks"/></span>
			  	</td>
			  </tr>
      </xsl:if>
			<xsl:apply-templates select="jobdoc:script | jobdoc:process">
				<xsl:with-param name="lang" select="$lang"/>
			</xsl:apply-templates>
			<xsl:apply-templates select="jobdoc:monitor/jobdoc:script">
				<xsl:with-param name="lang" select="$lang"/>
			</xsl:apply-templates>
		</table>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Job, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:job" mode="genXML">
      <span class="code">
      &lt;job name  = "<xsl:value-of select="@name"/>"
      <br/>    
      title = "<xsl:value-of select="@title"/>"
      <br/>    
      <xsl:choose>
        <xsl:when test="@order='yes' or @order='both'">
          order = "yes"
			  </xsl:when>
			  <xsl:otherwise>
			    order = "no"
			  </xsl:otherwise>
			</xsl:choose>
      <!--xsl:if test="@tasks">
        <br/>    
        tasks = "<xsl:value-of select="@tasks"/>"
      </xsl:if-->
      &gt;
      <br/>
      <br/>    
      &lt;description&gt;
      <br/>      
      &lt;include file = "jobs/<xsl:value-of select="@name"/>.xml"
      /&gt;
      <br/>    
      &lt;/description&gt;
      <br/>    
      <xsl:apply-templates select="../jobdoc:configuration/jobdoc:params" mode="genXML"/>
      <xsl:apply-templates select="jobdoc:script" mode="genXML"/>
      <xsl:apply-templates select="jobdoc:process" mode="genXML"/>
      <xsl:apply-templates select="jobdoc:monitor" mode="genXML"/>
      <br/>
      &lt;/job&gt;
      </span>
  </xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Dokumentation -->
	<xsl:template match="jobdoc:documentation">
		<xsl:param name="lang"/>
		<table class="box">
			<xsl:attribute name="id">documentation_<xsl:value-of select="$lang"/></xsl:attribute>
			<tr>
				<td class="td1">
					<span class="section">
						<xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Dokumentation'"/>
							<xsl:with-param name="label_en" select="'Documentation'"/>
						</xsl:call-template>
					</span>
				</td>
				<td class="td2"><xsl:text> </xsl:text><xsl:text/>
				</td>
				<td class="td3">

						<!-- <xsl:copy-of select="xhtml:div"/> -->
						<xsl:apply-templates mode="copy"/>

				</td>
			</tr>
		</table>
	</xsl:template>

  <xsl:template match="xhtml:br" mode="copy">
   <br/>
  </xsl:template>

  <xsl:template match="*" mode="copy">
    <xsl:element name="{name()}">
        <xsl:copy-of select="@*"/>
        <xsl:apply-templates mode="copy"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="text()" mode="copy">
     <xsl:value-of select="."/>
  </xsl:template>

	<xsl:template match="jobdoc:note">
		<xsl:apply-templates mode="copy"/>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Script/ Monitor Script -->
	<xsl:template match="jobdoc:script">
		<xsl:param name="lang"/>
		<tr>
			<td class="td1">
				<xsl:text> </xsl:text>
			</td>
			<td class="td2" onmouseout="hideWMTT()" >
			  <xsl:choose>
					<xsl:when test="ancestor::jobdoc:monitor">
					  <xsl:attribute name="onmouseover">showWMTT('ttMonitorScript_<xsl:value-of select="$lang"/>')</xsl:attribute>
            <xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Monitor-Skript'"/>
							<xsl:with-param name="label_en" select="'Monitor Script'"/>
						</xsl:call-template>
						  <span class="tooltip">
							  <xsl:attribute name="id">ttMonitorScript_<xsl:value-of select="$lang"/></xsl:attribute>
							  <xsl:call-template name="get_label">
								  <xsl:with-param name="lang" select="$lang"/>
								  <xsl:with-param name="label_de" select="'Monitor-Skripte werden zur Prüfung von Voraussetzungen für den Start von Jobs und Aufträgen eingesetzt und für die Steuerung der Weiterverarbeitung, z.B. für den bedingten Start von Folge-Jobs.'"/>
								  <xsl:with-param name="label_en" select="'Monitor Scripts check the requirements before a job or order is started. Moreover, they are used to control execution results, e.g. the conditional execution of successor jobs.'"/>
							  </xsl:call-template>
						  </span>
          </xsl:when>
					<xsl:otherwise>
					  <xsl:attribute name="onmouseover">showWMTT('ttScript_<xsl:value-of select="$lang"/>')</xsl:attribute>
            <xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Skript'"/>
							<xsl:with-param name="label_en" select="'Script'"/>
						</xsl:call-template>
						  <span class="tooltip">
							  <xsl:attribute name="id">ttScript_<xsl:value-of select="$lang"/></xsl:attribute>
							  <xsl:call-template name="get_label">
								  <xsl:with-param name="lang" select="$lang"/>
								  <xsl:with-param name="label_de" select="'Ein Skript enthält die Implementierung eines Jobs. Skripte können mit den Methoden des Job Scheduler API in den Sprachen  Java, Javascript, Perlscript und VBScript implementiert werden. Alternativ zur Implementierung mit Skripten können Jobs zur Ausführung beliebiger Programme und Kommandodateien konfiguriert werden.'"/>
								  <xsl:with-param name="label_en" select="'A script contains the implementation of one or more jobs. Scripts make optional use of the Job Scheduler API methods and are implemented in Java,Javascript, PerlScript or VBScript. Jobs are an alternative to scripts, and can be configured to start programs and executable files.'"/>

							  </xsl:call-template>
						  </span>
					</xsl:otherwise>
				</xsl:choose>
			</td>
			<td class="td3">
				<xsl:if test="@language">
				  <ul>
				  	<xsl:if test="@language">
				  		<li>
				  			<xsl:call-template name="get_label">
				  				<xsl:with-param name="lang" select="$lang"/>
				  				<xsl:with-param name="label_de" select="'Sprache: '"/>
				  				<xsl:with-param name="label_en" select="'Language: '"/>
				  			</xsl:call-template>
				  			<span class="sourceName"><xsl:value-of select="@language"/></span>
				  		</li>
				  	</xsl:if>
				  	<xsl:if test="@java_class">
				  		<li>
				  			<xsl:call-template name="get_label">
				  				<xsl:with-param name="lang" select="$lang"/>
				  				<xsl:with-param name="label_de" select="'Name der Java-Klasse: '"/>
				  				<xsl:with-param name="label_en" select="'Name of Java Class: '"/>
				  			</xsl:call-template>
				  			<span class="sourceName"><xsl:value-of select="@java_class"/></span>
				  		</li>
				  	</xsl:if>
				  	<xsl:if test="@com_class">
				  		<li>
				  			<xsl:call-template name="get_label">
				  				<xsl:with-param name="lang" select="$lang"/>
				  				<xsl:with-param name="label_de" select="'Name der COM-Klasse: '"/>
				  				<xsl:with-param name="label_en" select="'Name of COM Class: '"/>
				  			</xsl:call-template>
				  			<span class="sourceName"><xsl:value-of select="@com_class"/></span>
				  		</li>
				  	</xsl:if>
				  	<xsl:if test="@file">
				  		<li>Datei: <span class="sourceName"><xsl:value-of select="@file"/></span></li>
				  	</xsl:if>
				  	<xsl:if test="@resource">
				  		<li>
				  			<xsl:call-template name="get_resource">
				  				<xsl:with-param name="lang" select="$lang"/>
				  				<xsl:with-param name="resource_id" select="@resource"/>
				  			</xsl:call-template>
				  		</li>
				  	</xsl:if>
				  </ul>
				  <xsl:apply-templates select="jobdoc:include"/>
 				</xsl:if>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Script, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:script" mode="genXML">
    <xsl:choose>
      <xsl:when test="parent::jobdoc:monitor">
        <br/>      
      </xsl:when>
      <xsl:otherwise>
        <br/><br/>    
      </xsl:otherwise>
    </xsl:choose>
    &lt;script
    language   = "<xsl:value-of select="@language"/>"
    <xsl:if test="@java_class">
      <br/>            
      java_class = "<xsl:value-of select="@java_class"/>"
    </xsl:if>
    <xsl:if test="@com_class">
      <br/>             
      com_class = "<xsl:value-of select="@com_class"/>"
    </xsl:if>
    &gt;
	  <xsl:apply-templates select="jobdoc:include" mode="genXML" />
	  <xsl:choose>
      <xsl:when test="parent::jobdoc:monitor">
        <br/>      
      </xsl:when>
      <xsl:otherwise>
        <br/>    
      </xsl:otherwise>
    </xsl:choose>
    &lt;/script&gt;
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Monitor Script, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:monitor" mode="genXML">
	  <br/>
    <br/>    
    &lt;monitor&gt;
	   <xsl:apply-templates select="jobdoc:script" mode="genXML"/>
	  <br/>    
	  &lt;/monitor&gt;
	</xsl:template>
  <!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Include -->
	<xsl:template match="jobdoc:include">
		<xsl:if test="not(@file='')">
		  <ul>
		  	<li>Include: <span class="sourceName"><xsl:value-of select="@file"/></span></li>
		  </ul>
		</xsl:if>
	</xsl:template>
  <!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Include, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:include" mode="genXML">
		<xsl:choose>
      <xsl:when test="ancestor::jobdoc:monitor">
        <br/>        
      </xsl:when>
      <xsl:otherwise>
        <br/>      
      </xsl:otherwise>
    </xsl:choose>

    &lt;include file = "<xsl:value-of select="@file"/>" /&gt;
	</xsl:template>
  <!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Process -->
	<xsl:template match="jobdoc:process">
		<xsl:param name="lang"/>
		<tr>
			<td class="td1">
				<xsl:text> </xsl:text>
			</td>
			<td class="td2">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Ausführen'"/>
					<xsl:with-param name="label_en" select="'Process'"/>
				</xsl:call-template>
			</td>
			<td class="td3">
				<ul>
					<xsl:if test="@file">
						<li>
							<xsl:call-template name="get_label">
								<xsl:with-param name="lang" select="$lang"/>
								<xsl:with-param name="label_de" select="'Datei: '"/>
								<xsl:with-param name="label_en" select="'File: '"/>
							</xsl:call-template>
							<span class="sourceName"><xsl:value-of select="@file"/></span>
						</li>
					</xsl:if>
					<xsl:if test="@param">
						<li>
							<xsl:call-template name="get_label">
								<xsl:with-param name="lang" select="$lang"/>
								<xsl:with-param name="label_de" select="'Parameter: '"/>
								<xsl:with-param name="label_en" select="'Param: '"/>
							</xsl:call-template>
							<span class="sourceName"><xsl:value-of select="@param"/></span>
						</li>
					</xsl:if>
					<xsl:if test="@log">
						<li>
							<xsl:call-template name="get_label">
								<xsl:with-param name="lang" select="$lang"/>
								<xsl:with-param name="label_de" select="'Protokolldatei: '"/>
								<xsl:with-param name="label_en" select="'Log File: '"/>
							</xsl:call-template>
							<span class="sourceName"><xsl:value-of select="@log"/></span>
						</li>
					</xsl:if>
				</ul>
				<xsl:apply-templates select="jobdoc:environment">
					<xsl:with-param name="lang" select="$lang"/>
				</xsl:apply-templates>
			</td>
		</tr>
	</xsl:template>
  <!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Process, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:process" mode="genXML">
	  <br/>
	  <br/>    
	  &lt;process file = "<xsl:value-of select="@file"/>"
	  param = "<xsl:value-of select="@param"/>"
	  log = "<xsl:value-of select="@log"/>" &gt;
	  <xsl:apply-templates select="jobdoc:environment" mode="genXML"/>
	  <br/>    
	  &lt;/process&gt;
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Environment als Tabelle -->
	<xsl:template match="jobdoc:environment">
		<xsl:param name="lang"/>
		<tr>
			<td class="td1">
				<xsl:text> </xsl:text>
			</td>
			<td class="td2">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Umgebungsvariablen'"/>
					<xsl:with-param name="label_en" select="'Environment Variables'"/>
				</xsl:call-template>
			</td>
			<td class="td3">
				<table class="resource" cellpadding="0" cellspacing="1">
					<tbody>
						  <tr>
						  	<th class="resource1"><span class="desc">Name</span></th>
						  	<th class="resource2">
						  		<span class="desc">
						  		<xsl:call-template name="get_label">
						  			<xsl:with-param name="lang" select="$lang"/>
						  			<xsl:with-param name="label_de" select="'Wert'"/>
						  			<xsl:with-param name="label_en" select="'Value'"/>
						  		</xsl:call-template>
						  		</span>
						  	</th>
						  	<th class="resource3"></th>
						  	<th class="resource4"></th>
						  </tr>
						  <xsl:apply-templates select="jobdoc:variable"/>
					</tbody>
				</table>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Environment, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:environment" mode="genXML">
	  <br/>      
	  &lt;environment&gt;
	  <xsl:apply-templates select="jobdoc:variable" mode="genXML"/>
	  <br/>      
	  &lt;/environment&gt;
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Environment/ Variable als Tabellenzeile -->
	<xsl:template match="jobdoc:variable">
		<tr>
			<td class="resource1"><span class="desc"><xsl:value-of select="@name"/></span></td>
			<td class="resource2"><span class="desc"><xsl:value-of select="@value"/></span></td>
			<td class="resource3"/>
			<td class="resource4"/>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Environment/ Variable, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:variable" mode="genXML">
	  <br/>        
	  &lt;variable name = "<xsl:value-of select="@name"/>"
	  value = "<xsl:value-of select="@value"/>"  /&gt;
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Releases -->
	<xsl:template match="jobdoc:releases">
		<xsl:param name="lang"/>
		<xsl:variable name="cnt_releases" select="count(jobdoc:release)"/>
		<xsl:if test="$cnt_releases = 1">
			<table class="box" style="display:none">
				<xsl:attribute name="id">firstRelease_<xsl:value-of select="$lang"/></xsl:attribute>
				<xsl:for-each select="jobdoc:release">
					<xsl:if test="position() = 1">
						<xsl:call-template name="jobdoc:release_x">
							<xsl:with-param name="lang" select="$lang"/>
						</xsl:call-template>
					</xsl:if>
				</xsl:for-each>
			</table>
		</xsl:if>
		<xsl:if test="$cnt_releases > 1">
			<table class="box" style="display:none">
				<xsl:attribute name="id">firstRelease_<xsl:value-of select="$lang"/></xsl:attribute>
				<xsl:for-each select="jobdoc:release">
					<xsl:if test="position() = 1">
						<xsl:call-template name="jobdoc:release_x">
							<xsl:with-param name="lang" select="$lang"/>
						</xsl:call-template>
					</xsl:if>
				</xsl:for-each>
				<tr>
					<td/>
					<td/>
					<td>
						<a class="doc">
							<xsl:attribute name="id">releaseLink_<xsl:value-of select="$lang"/></xsl:attribute>
              <xsl:attribute name="href">javascript:showPreviousReleases('<xsl:value-of select="$lang"/>');</xsl:attribute>
							<span style="font-family:Arial;font-size:12px;">&#8594;</span>
							<xsl:text> </xsl:text>
							<xsl:call-template name="get_label">
								<xsl:with-param name="lang" select="$lang"/>
								<xsl:with-param name="label_de" select="'Ältere Releases anzeigen'"/>
								<xsl:with-param name="label_en" select="'Show previous Releases'"/>
							</xsl:call-template>
						</a>
					</td>
				</tr>
			</table>
			<table class="boxRelease" style="display:none">
				<xsl:attribute name="id">previousRelease_<xsl:value-of select="$lang"/></xsl:attribute>
				<xsl:for-each select="jobdoc:release">
					<xsl:if test="position() > 1">
						<xsl:call-template name="jobdoc:release_x">
							<xsl:with-param name="lang" select="$lang"/>
						</xsl:call-template>
					</xsl:if>
				</xsl:for-each>
			</table>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Releases/ Release -->
	<xsl:template name="jobdoc:release_x">
		<xsl:param name="lang"/>
		<tr>
			<td class="td1">
				<xsl:if test="position() = 1">
					<span class="section">Releases</span>
				</xsl:if>
			</td>
			<td class="td2">
				<span class="sourceNameBold">
					<xsl:value-of select="@id"/>
				</span>
			</td>
			<td class="td3">
				<span class="desc"><xsl:value-of select="jobdoc:title"/></span>
			</td>
		</tr>
		<tr>
			<td class="td1"/>
			<td class="td2"/>
			<td class="td3">
				<span class="desc">
					<xsl:value-of select="@created"/>
          [
          <xsl:call-template name="get_label">
						<xsl:with-param name="lang" select="$lang"/>
						<xsl:with-param name="label_de" select="'letzte Änderung '"/>
						<xsl:with-param name="label_en" select="'last Changes '"/>
					</xsl:call-template>
					<xsl:value-of select="@modified"/>
          ]
        </span>
			</td>
		</tr>
		<xsl:apply-templates select="jobdoc:author"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
		<tr>
			<td class="td1">
				<xsl:text> </xsl:text>
			</td>
			<td class="td2">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Kommentar'"/>
					<xsl:with-param name="label_en" select="'Comment'"/>
				</xsl:call-template>
			</td>
			<td class="td3">
				<span class="desc">
					<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
				</span>
			</td>
		</tr>
		<xsl:apply-templates select="jobdoc:changes[@language=$lang]"/>
		<xsl:if test="position() > 1 and not( position()=last() )">
			<tr>
				<td class="td1">
					<xsl:text> </xsl:text>
				</td>
				<td class="td2_3" colspan="2">
					<hr/>
				</td>
			</tr>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Release/ Author -->
	<xsl:template match="jobdoc:author">
		<xsl:param name="lang"/>
		<tr>
			<td class="td1">
				<xsl:text> </xsl:text>
			</td>
			<td class="td2">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Autor'"/>
					<xsl:with-param name="label_en" select="'Author'"/>
				</xsl:call-template>
			</td>
			<td class="td3">
				<span class="desc">
					<xsl:value-of select="@name"/>
					<xsl:text>  </xsl:text>
					<a class="mail">
						<xsl:attribute name="href">mailto:<xsl:value-of select="@email"/></xsl:attribute>
						<xsl:value-of select="@email"/>
					</a>
				</span>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Release/ Changes -->
	<xsl:template match="jobdoc:changes">
		<tr>
			<td/>
			<td class="td2">Change Log</td>
			<td class="td3">
				<span class="desc">
					<xsl:apply-templates mode="copy"/>
				</span>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Resourcen -->
	<xsl:template match="jobdoc:resources">
		<xsl:param name="lang"/>
		<table class="box" style="display:none">
			<xsl:attribute name="id">resources_<xsl:value-of select="$lang"/></xsl:attribute>
			<tr>
				<td class="td1">
					<span class="section">
						<xsl:if test="position() = 1">
							<xsl:call-template name="get_label">
								<xsl:with-param name="lang" select="$lang"/>
								<xsl:with-param name="label_de" select="'Ressourcen'"/>
								<xsl:with-param name="label_en" select="'Resources'"/>
							</xsl:call-template>
						</xsl:if>
					</span>
				</td>
				<td class="td2">
					<xsl:call-template name="get_label">
						<xsl:with-param name="lang" select="$lang"/>
						<xsl:with-param name="label_de" select="'Dateien'"/>
						<xsl:with-param name="label_en" select="'Files'"/>
					</xsl:call-template>
				</td>
				<td class="td3">
					<xsl:if test="jobdoc:file">
						<table class="resource" cellpadding="0" cellspacing="1">
							<tr>
								<th class="resource1">
									<xsl:call-template name="get_label">
										<xsl:with-param name="lang" select="$lang"/>
										<xsl:with-param name="label_de" select="'Dateiname'"/>
										<xsl:with-param name="label_en" select="'Filename'"/>
									</xsl:call-template>
								</th>
								<th class="resource2">
									<xsl:call-template name="get_label">
										<xsl:with-param name="lang" select="$lang"/>
										<xsl:with-param name="label_de" select="'Typ'"/>
										<xsl:with-param name="label_en" select="'Type'"/>
									</xsl:call-template>
								</th>
								<th class="resource3">
									<xsl:call-template name="get_label">
										<xsl:with-param name="lang" select="$lang"/>
										<xsl:with-param name="label_de" select="'OS'"/>
										<xsl:with-param name="label_en" select="'OS'"/>
									</xsl:call-template>
								</th>
								<th class="resource4">
									<xsl:call-template name="get_label">
										<xsl:with-param name="lang" select="$lang"/>
										<xsl:with-param name="label_de" select="'Kommentar'"/>
										<xsl:with-param name="label_en" select="'Comment'"/>
									</xsl:call-template>
								</th>
							</tr>
							<xsl:apply-templates select="jobdoc:file">
								<xsl:with-param name="lang" select="$lang"/>
								<xsl:sort select="@os"/>
							</xsl:apply-templates>
						</table>
					</xsl:if>
				</td>
			</tr>
			<xsl:if test="jobdoc:database">
			  <tr>
			  	<td class="td1">
			  		<xsl:text/>
			  	</td>
			  	<td class="td2">
			  		<xsl:call-template name="get_label">
			  			<xsl:with-param name="lang" select="$lang"/>
			  			<xsl:with-param name="label_de" select="'Datenbank'"/>
			  			<xsl:with-param name="label_en" select="'Database'"/>
			  		</xsl:call-template>
			  	</td>
			  	<td class="td3">
			  		<xsl:if test="jobdoc:database">
			  			<table class="resource" cellpadding="0" cellspacing="1">
			  				<tr>
			  					<th class="resource1">
			  						<xsl:call-template name="get_label">
			  							<xsl:with-param name="lang" select="$lang"/>
			  							<xsl:with-param name="label_de" select="'Name'"/>
			  							<xsl:with-param name="label_en" select="'Name'"/>
			  						</xsl:call-template>
			  					</th>
			  					<th class="resource2_3">
			  						<xsl:call-template name="get_label">
			  							<xsl:with-param name="lang" select="$lang"/>
			  							<xsl:with-param name="label_de" select="'Typ'"/>
			  							<xsl:with-param name="label_en" select="'Type'"/>
			  						</xsl:call-template>
			  					</th>
			  					<th class="resource4">
			  						<xsl:call-template name="get_label">
			  							<xsl:with-param name="lang" select="$lang"/>
			  							<xsl:with-param name="label_de" select="'Kommentar'"/>
			  							<xsl:with-param name="label_en" select="'Comment'"/>
			  						</xsl:call-template>
			  					</th>
			  				</tr>
			  				<xsl:apply-templates select="jobdoc:database">
			  					<xsl:with-param name="lang" select="$lang"/>
			  				</xsl:apply-templates>
			  			</table>
			  		</xsl:if>
			  	</td>
			  </tr>
			</xsl:if>
      <xsl:if test="jobdoc:memory">
			  <tr>
			  	<td class="td1"><xsl:text/></td>
			  	<td class="td2">
			  		<xsl:call-template name="get_label">
			  			<xsl:with-param name="lang" select="$lang"/>
			  			<xsl:with-param name="label_de" select="'Hauptspeicher'"/>
			  			<xsl:with-param name="label_en" select="'Memory'"/>
			  		</xsl:call-template>
			  	</td>
			  	<td class="td3">
			  		<xsl:if test="jobdoc:memory">
			  			<table class="resource" cellpadding="0" cellspacing="1">
			  				<xsl:apply-templates select="jobdoc:memory">
			  					<xsl:with-param name="lang" select="$lang"/>
			  				</xsl:apply-templates>
			  			</table>
    	  		</xsl:if>
			  	</td>
			  </tr>
			</xsl:if>
			<xsl:if test="jobdoc:space">
			  <tr>
			  	<td class="td1">
			  		<xsl:text/>
			  	</td>
			  	<td class="td2">
			  		<xsl:call-template name="get_label">
			  			<xsl:with-param name="lang" select="$lang"/>
			  			<xsl:with-param name="label_de" select="'Plattenplatz'"/>
			  			<xsl:with-param name="label_en" select="'Disk Space'"/>
			  		</xsl:call-template>
			  	</td>
			  	<td class="td3">
			  		<xsl:if test="jobdoc:space">
			  			<table class="resource" cellpadding="0" cellspacing="1">
			  				<xsl:apply-templates select="jobdoc:space">
			  					<xsl:with-param name="lang" select="$lang"/>
			  				</xsl:apply-templates>
			  			</table>
			  		</xsl:if>
			  	</td>
			  </tr>
			</xsl:if>
		</table>
	</xsl:template>

	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Eine Tabellenzeile pro Resources/ File -->
	<xsl:template match="jobdoc:file">
		<xsl:param name="lang"/>
		<tr>
			<td class="resource1">
				<span class="sourceName"><xsl:value-of select="@file"/></span>
			</td>
			<td class="resource2">
				<span class="desc"><xsl:value-of select="@type"/></span>
			</td>
			<td class="resource3">
				<span class="desc"><xsl:value-of select="@os"/></span>
			</td>
			<td class="resource4">
				<xsl:if test="jobdoc:note[@language=$lang]">
					<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
				</xsl:if>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Resources/ Datbase -->
	<xsl:template match="jobdoc:database">
		<xsl:param name="lang"/>
		<!--
	  <xsl:if test="@required='true'">
	   A Database is required<br/>
	  </xsl:if>
	  <xsl:if test="@required='false'">
	    This job can be run with a database<br/>
	  </xsl:if>
	  -->
		<xsl:if test="jobdoc:resource">
			<xsl:apply-templates select="jobdoc:resource">
				<xsl:with-param name="lang" select="$lang"/>
				<xsl:sort select="@type"/>
			</xsl:apply-templates>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Eine Tabellenzeile pro Resources/ Database/ Resource -->
	<xsl:template match="jobdoc:resource">
		<xsl:param name="lang"/>
		<tr>
			<td class="resource1">
				<xsl:value-of select="@name"/>
			</td>
			<td class="resource2_3">
				<xsl:value-of select="@type"/>
			</td>
			<td class="resource4">
				<xsl:if test="jobdoc:note[@language=$lang]">
					<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
				</xsl:if>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Eine Tabellenzeile pro Resources/ memory -->
	<xsl:template match="jobdoc:memory">
		<xsl:param name="lang"/>
		<tr>
			<th class="resource1">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Einheit'"/>
					<xsl:with-param name="label_en" select="'Unit'"/>
				</xsl:call-template>
			</th>
			<th class="resource2_3">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Wert'"/>
					<xsl:with-param name="label_en" select="'Value'"/>
				</xsl:call-template>
			</th>
			<th class="resource4">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Kommentar'"/>
					<xsl:with-param name="label_en" select="'Comment'"/>
				</xsl:call-template>
			</th>
		</tr>
		<tr>
			<td class="resource1">
				<xsl:value-of select="@unit"/>
			</td>
			<td class="resource2_3">
				<xsl:value-of select="@min"/>
			</td>
			<td class="resource4">
				<xsl:if test="jobdoc:note[@language=$lang]">
					<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
				</xsl:if>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Eine Tabellenzeile pro Resources/ Disk Space -->
	<xsl:template match="jobdoc:space">
		<xsl:param name="lang"/>
		<tr>
			<th class="resource1">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Einheit'"/>
					<xsl:with-param name="label_en" select="'Unit'"/>
				</xsl:call-template>
			</th>
			<th class="resource2_3">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Wert'"/>
					<xsl:with-param name="label_en" select="'Value'"/>
				</xsl:call-template>
			</th>
			<th class="resource4">
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Kommentar'"/>
					<xsl:with-param name="label_en" select="'Comment'"/>
				</xsl:call-template>
			</th>
		</tr>
		<tr>
			<td class="resource1">
				<xsl:value-of select="@unit"/>
			</td>
			<td class="resource2_3">
				<xsl:value-of select="@min"/>
			</td>
			<td class="resource4">
				<xsl:if test="jobdoc:note[@language=$lang]">
					<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
				</xsl:if>
			</td>
		</tr>
	</xsl:template>
	<!--  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Konfiguration -->
	<xsl:template match="jobdoc:configuration">
		<xsl:param name="lang"/>
		<!-- Nur wenn es Elemente unterhalb von configuration gibt -->
		<xsl:if test="child::*">
			<table class="box" style="display:none">
				<xsl:attribute name="id">configuration_<xsl:value-of select="$lang"/></xsl:attribute>
			  <tr>
		    	<td class="td1">
		    		<span class="section">
		    			<xsl:call-template name="get_label">
		    			  <xsl:with-param name="lang" select="$lang"/>
		    				<xsl:with-param name="label_de" select="'Konfiguration'"/>
		    				<xsl:with-param name="label_en" select="'Configuration'"/>
		    			</xsl:call-template>
		    		</span>
		    	</td>
		    	<td class="td2"><xsl:text> </xsl:text></td>
		    	<td class="td3">
				    <xsl:choose>
				    	<xsl:when test="jobdoc:note[@language=$lang]">
				    		<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
				    	</xsl:when>
				    	<xsl:otherwise>
				    		<xsl:text> </xsl:text>
				    	</xsl:otherwise>
				    </xsl:choose>
		    	</td>
		    </tr>
				<xsl:apply-templates select="jobdoc:params">  <xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
				<xsl:apply-templates select="jobdoc:payload"> <xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
				<xsl:apply-templates select="jobdoc:settings"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
			</table>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Configuration/ Parameter -->
	<xsl:template match="jobdoc:params[parent::jobdoc:configuration]">
		<xsl:param name="lang"/>
		<tr>
			<td class="td1">
		    <xsl:call-template name="set_anchor"><xsl:with-param name="lang" select="$lang"/><xsl:with-param name="anchor_name" select="@id"/></xsl:call-template>
        <xsl:text> </xsl:text>
			</td>
			<td class="td2" onmouseout="hideWMTT()" >
				<xsl:attribute name="onmouseover">showWMTT('ttparams_<xsl:value-of select="$lang"/>')</xsl:attribute>
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Job-Parameter'"/>
					<xsl:with-param name="label_en" select="'Job Parameters'"/>
				</xsl:call-template>
					<span class="tooltip">
						<xsl:attribute name="id">ttparams_<xsl:value-of select="$lang"/></xsl:attribute>
						<xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Ein Job kann mehrere Parameter haben, die beim Job-Start übergeben werden. Parameter werden in der XML-Konfigurationsdatei des Jobs oder per API-Methoden bekanntgemacht. Parameter sind entweder Pflichtangaben oder optional. Für einen Parameter kann ein Default-Wert eingestellt sein.'"/>
							<xsl:with-param name="label_en" select="'A job can process multiple parameters that are specified when it starts. Parameters are defined in the XML configuration file or submitted by API methods. Parameters are optional or mandatory and can contain default values.'"/>
						</xsl:call-template>
					</span>
			</td>
			<td class="td3">
			  <xsl:choose>
					<xsl:when test="jobdoc:note[@language=$lang]">
						<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
					</xsl:when>
					<xsl:otherwise>
				    <xsl:if test="@reference and not(@reference='')">
 					    <xsl:if test="jobdoc:note[@language=$lang]">
						    <br/>
					    </xsl:if>
					    <xsl:call-template name="process_reference"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
				    </xsl:if>
            <xsl:if test="child::*">
					    <table class="section" cellpadding="0" cellspacing="1">
		  		      <xsl:apply-templates select="jobdoc:param"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
				      </table>
				    </xsl:if>
					</xsl:otherwise>
				</xsl:choose>
			</td>
		</tr>
		<xsl:if test="jobdoc:note[@language=$lang]">
			<tr>
				<td class="td1"><xsl:text> </xsl:text></td>
				<td class="td1"><xsl:text> </xsl:text></td>
				<td class="td3">
				  <xsl:if test="@reference and not(@reference='')">
				    <xsl:call-template name="process_reference"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
				  </xsl:if>
          <xsl:if test="child::*">
				    <table class="section" cellpadding="0" cellspacing="1">
		  	      <xsl:apply-templates select="jobdoc:param"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
				    </table>
				  </xsl:if>
				</td>
			</tr>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Configuration/ Parameter als Referenz! -->
	<xsl:template match="jobdoc:params" mode="reference">
		<xsl:param name="lang"/>
		<xsl:variable name="reftext">
			<xsl:choose>
				<xsl:when test="ancestor::jobdoc:payload">
			    Payload Parameter
			    <!-- xsl:call-template name="get_label">
					  <xsl:with-param name="lang" select="$lang"/>
						<xsl:with-param name="label_de" select="'Payload-Parameter'"/>
						<xsl:with-param name="label_en" select="'Payload Parameter'"/>
					</xsl:call-template -->
				</xsl:when>
				<xsl:otherwise>
				  Job Parameter
				  <!-- xsl:call-template name="get_label">
					  <xsl:with-param name="lang" select="$lang"/>
						<xsl:with-param name="label_de" select="'Job-Parameter'"/>
						<xsl:with-param name="label_en" select="'Job Parameter'"/>
					</xsl:call-template -->
				</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<a class="doc">
			<xsl:attribute name="href">#<xsl:value-of select="@id"/>_<xsl:value-of select="$lang"/></xsl:attribute>
			<span style="font-family:Arial;font-size:12px;">&#8594;</span>
			<xsl:text> </xsl:text>
			<xsl:value-of select="$reftext"/>
		</a>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für einzelnen Parameter -->
	<xsl:template match="jobdoc:param">
		<xsl:param name="lang"/>
		<tr>
			<td class="section1">
				<xsl:call-template name="set_anchor"><xsl:with-param name="lang" select="$lang"/><xsl:with-param name="anchor_name" select="@id"/></xsl:call-template>
				<span class="sourceName">
					<xsl:value-of select="@name"/>
				</span>
			</td>
			<td class="section2">
				<xsl:choose>
					<xsl:when test="@reference and not(@reference='')">
						<xsl:call-template name="process_reference"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
					</xsl:when>
					<xsl:otherwise>
						<xsl:variable name="defVal">
							<xsl:choose>
								<xsl:when test="@default_value and not(@default_value='')">
									<xsl:value-of select="@default_value"/>
								</xsl:when>
								<xsl:otherwise>---</xsl:otherwise>
							</xsl:choose>
						</xsl:variable>
						<span class="label">Default: <xsl:text> </xsl:text>
							<xsl:value-of select="$defVal"/>
						</span>
					</xsl:otherwise>
				</xsl:choose>
			</td>
		</tr>
		<xsl:if test="not(@reference)">
			<tr>
				<td class="section1">
					<xsl:if test="@required='false'">
						<span class="labelSmall">
							<xsl:text>[optional]</xsl:text>
						</span>
					</xsl:if>
					<xsl:if test="@required='true'">
						<xsl:text> </xsl:text>
					</xsl:if>
				</td>
				<td class="section2">
					<span class="desc">
						<xsl:if test="jobdoc:note[@language=$lang]">
							<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
						</xsl:if>
					</span>
				</td>
			</tr>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für einzelnen Parameter als Referenz! -->
	<xsl:template match="jobdoc:param" mode="reference">
		<xsl:param name="lang"/>
		<xsl:variable name="reftext">
			<xsl:choose>
				<xsl:when test="ancestor::jobdoc:payload">
				  Payload Parameter
				  <!-- xsl:call-template name="get_label">
					  <xsl:with-param name="lang" select="$lang"/>
						<xsl:with-param name="label_de" select="'Payload-Parameter'"/>
						<xsl:with-param name="label_en" select="'Payload Parameter'"/>
					</xsl:call-template -->
				</xsl:when>
				<xsl:otherwise>
				  Job Parameter
				  <!-- xsl:call-template name="get_label">
					  <xsl:with-param name="lang" select="$lang"/>
						<xsl:with-param name="label_de" select="'Job-Parameter'"/>
						<xsl:with-param name="label_en" select="'Job Parameter'"/>
					</xsl:call-template -->
				</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<a class="doc">
			<xsl:attribute name="href">#<xsl:value-of select="@id"/>_<xsl:value-of select="$lang"/></xsl:attribute>
			<span style="font-family:Arial;font-size:12px;">&#8594;</span>
			<xsl:text> </xsl:text>
			<xsl:value-of select="$reftext"/>
		</a>
		<span class="sourceName"><xsl:value-of select="@name"/></span>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Configuration/ Parameter, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:params[parent::jobdoc:configuration]" mode="genXML">
    <br/>    
    &lt;params&gt;
	    <xsl:apply-templates select="jobdoc:param" mode="genXML"/>
	  <br/>    
    &lt;/params&gt;
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für einzelnen Parameter, wenn XML generiert werden soll -->
	<xsl:template match="jobdoc:param" mode="genXML">
    <xsl:if test="@required='true'">
	    <br/>      
      &lt;param
      name = "<xsl:value-of select="@name"/>"
      value = "<xsl:value-of select="@default_value"/>"
      /&gt;
    </xsl:if>
    <xsl:if test="@required='false'">
	    <br/>      
      &lt;param
      name = "<xsl:value-of select="@name"/>"
      value = "<xsl:value-of select="@default_value"/>"
      /&gt;
    </xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Payload -->
	<xsl:template match="jobdoc:payload">
		<xsl:param name="lang"/>
    <xsl:call-template name="set_anchor"><xsl:with-param name="lang" select="$lang"/><xsl:with-param name="anchor_name" select="jobdoc:params/@id" /></xsl:call-template>
		<tr>
			<td class="td1">
				<xsl:text> </xsl:text>
			</td>
			<td class="td2" onmouseout="hideWMTT()" >
				<xsl:attribute name="onmouseover">showWMTT('ttpayload_<xsl:value-of select="$lang"/>')</xsl:attribute>
      Payload
					<span class="tooltip">
						<xsl:attribute name="id">ttpayload_<xsl:value-of select="$lang"/></xsl:attribute>
						<xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Auftragsgesteuerte Jobs verarbeiten Aufträge mit einer optionalen Payload für auftragsbezogene Parameter.'"/>
							<xsl:with-param name="label_en" select="'Order driven jobs execute orders with an optional payload that contains parameters for the order.'"/>
						</xsl:call-template>
					</span>
			</td>
			<td class="td3">
				<xsl:choose>
					<xsl:when test="jobdoc:note[@language=$lang]">
						<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
					</xsl:when>
					<xsl:otherwise>
						<table class="section" cellpadding="0" cellspacing="1">
							<xsl:apply-templates select="jobdoc:params"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
						</table>
					</xsl:otherwise>
				</xsl:choose>
			</td>
		</tr>
		<xsl:if test="jobdoc:note[@language=$lang]">
			<tr>
				<td class="td1">
					<xsl:text> </xsl:text>
				</td>
				<td class="td1">
					<xsl:text> </xsl:text>
				</td>
				<td class="td3">
					<table class="section" cellpadding="0" cellspacing="1">
						<xsl:apply-templates select="jobdoc:params"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
					</table>
				</td>
			</tr>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template für Payload/ Parameter -->
	<xsl:template match="jobdoc:params[parent::jobdoc:payload]">
		<xsl:param name="lang"/>
		<xsl:if test="@reference and not(@reference='')">
			<xsl:call-template name="process_reference"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
		</xsl:if>
		<xsl:if test="child::*">
		  <xsl:apply-templates select="jobdoc:param"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Settings -->
	<xsl:template match="jobdoc:settings">
		<xsl:param name="lang"/>
		<xsl:if test="child::*">
		  <xsl:if test="jobdoc:note[@language=$lang]">
			  <tr>
		    	<td class="td1"><xsl:text> </xsl:text></td>
		    	<td class="td2">
		    		<!--
		    		<span class="label">
		    			<xsl:call-template name="get_label">
		    			  <xsl:with-param name="lang" select="$lang"/>
		    				<xsl:with-param name="label_de" select="'Settings'"/>
		    				<xsl:with-param name="label_en" select="'Settings'"/>
		    			</xsl:call-template>
		    		</span>
		    		-->
		    	</td>
		    	<td class="td3">
		    		<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
		    	</td>
		    </tr>
		  </xsl:if>
		  <xsl:apply-templates select="jobdoc:profile"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
		  <xsl:apply-templates select="jobdoc:connection"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Settings/ Profile -->
	<xsl:template match="jobdoc:profile">
		<xsl:param name="lang"/>
		<tr>
			<td class="td1">
				<xsl:text> </xsl:text>
			</td>
			<td class="td2" onmouseout="hideWMTT()" >
				<xsl:attribute name="onmouseover">showWMTT('ttsettingsIni_<xsl:value-of select="$lang"/>')</xsl:attribute>
        Profile
					<span class="tooltip">
						<xsl:attribute name="id">ttsettingsIni_<xsl:value-of select="$lang"/></xsl:attribute>
						<xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Zentrale Einstellungen für den Job in einer Konfigurationsdatei. Per Default ist der Name der Konfigurationsdatei factory.ini .'"/>
							<xsl:with-param name="label_en" select="'Central job settings given in a profile: the default profile is held in the factory.ini configuration file .'"/>
						</xsl:call-template>
					</span>
			</td>
			<td class="td3">
				<xsl:if test="jobdoc:note[@language=$lang]">
				  <xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
				  <br/>
				</xsl:if>
  			<xsl:call-template name="get_label">
  				<xsl:with-param name="lang" select="$lang"/>
  				<xsl:with-param name="label_de" select="'Name der Konfigurationsdatei: '"/>
  				<xsl:with-param name="label_en" select="'Name of Configuration File: '"/>
  			</xsl:call-template>
  			<xsl:if test="@name">
  				<span class="sourceName">
  					<xsl:value-of select="@name"/>
  				</span>
				</xsl:if>
				<xsl:apply-templates select="jobdoc:section"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Settings/ Connection -->
	<xsl:template match="jobdoc:connection">
		<xsl:param name="lang"/>
		<tr>
			<td class="td1">
				<xsl:text> </xsl:text>
			</td>
			<td class="td2" onmouseover="showWMTT('ttsettingsDB')" onmouseout="hideWMTT()" >
				<xsl:attribute name="onmouseover">showWMTT('ttsettingsDB_<xsl:value-of select="$lang"/>')</xsl:attribute>
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Datenbank'"/>
					<xsl:with-param name="label_en" select="'Database'"/>
				</xsl:call-template>
					<span class="tooltip">
						<xsl:attribute name="id">ttsettingsDB_<xsl:value-of select="$lang"/></xsl:attribute>
						<xsl:call-template name="get_label">
							<xsl:with-param name="lang" select="$lang"/>
							<xsl:with-param name="label_de" select="'Zentrale Einstellungen für den Job in einer Datenbank. Alternativ zu den Einstellungen über XML-Konfiguration oder Profile können Parameter auch aus logischen Tabellen in einer Datenbank gelesen werden. Ein Parameter-Eintrag liegt innerhalb der logischen Struktur der Attribute application/section/entry der Tabelle SETTINGS. Per Default werden Einstellungen aus der Datenbank gelesen, die in der Konfigurationsdatei factory.ini angegeben wurde.'"/>
							<xsl:with-param name="label_en" select="'Central job settings read from a database. As an alternative to the XML configuration file or the profile, settings can be read from database tables. Parameter entries must use the attributes structure of the SETTINGS table - i.e. application/section/entry. By default values are read from Job Scheduler Database whose connection data is specified in the factory.ini configuration file.'"/>
						</xsl:call-template>
					</span>
			</td>
			<td class="td3">
			  <xsl:if test="jobdoc:note[@language=$lang]">
				  <xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
				  <p></p>
				</xsl:if>
				<xsl:call-template name="get_label">
					<xsl:with-param name="lang" select="$lang"/>
					<xsl:with-param name="label_de" select="'Datenbank-Name: '"/>
					<xsl:with-param name="label_en" select="'Database Name: '"/>
				</xsl:call-template>
				<xsl:if test="@name">
					<span class="sourceName">
						<xsl:value-of select="@name"/>
					</span>
				</xsl:if>
				<table cellpadding="0" cellspacing="0" >
					<xsl:apply-templates select="jobdoc:application"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
				</table>
			</td>
		</tr>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Application -->
	<xsl:template match="jobdoc:application">
		<xsl:param name="lang"/>
		  <xsl:if test="@reference and not(@reference='')">
		    <xsl:call-template name="process_reference"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
		  </xsl:if>
		  <xsl:if test="child::*">
		    <tr>
		    	<td class="td31_32">
		    		<xsl:call-template name="set_anchor">
		    		  <xsl:with-param name="lang" select="$lang"/>
		    		  <xsl:with-param name="anchor_name" select="@id"/>
		    		</xsl:call-template>
		    		<span class="label">Application: </span>
		    		<span class="sourceName"><xsl:value-of select="@name"/></span>
		    		<xsl:text> </xsl:text>
		    		<xsl:apply-templates select="jobdoc:section"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
		    	</td>
		    </tr>
		  </xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Application als Referenz! -->
	<xsl:template match="jobdoc:application" mode="reference">
		<xsl:param name="lang"/>
		<a class="doc">
		  <xsl:attribute name="href">#<xsl:value-of select="@id"/>_<xsl:value-of select="$lang"/></xsl:attribute>
			<span style="font-family:Arial;font-size:12px;">&#8594;</span>
			<xsl:text> </xsl:text>
      Database Settings: Application
    </a>
		<xsl:text> </xsl:text>
		<span class="sourceName"><xsl:value-of select="@name"/></span>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Section -->
	<xsl:template match="jobdoc:section">
		<xsl:param name="lang"/>
		<xsl:if test="@reference and not(@reference='')">
		  <br/>
		  <xsl:call-template name="process_reference"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
		</xsl:if>
		<xsl:if test="child::*">
		  <table class="section" cellpadding="0" cellspacing="1">
		  	<tr>
		  		<td class="td31_32">
		        <xsl:call-template name="set_anchor"><xsl:with-param name="lang" select="$lang"/><xsl:with-param name="anchor_name" select="@id"/></xsl:call-template>
		        <span class="label">Section: </span>
		        <span class="sourceName"><xsl:value-of select="@name"/></span>
		  		</td>
		  	</tr>
		  </table>
		  <table class="section" cellpadding="0" cellspacing="1">
		    <xsl:apply-templates select="jobdoc:setting"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
		  </table>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Section als Referenz! -->
	<xsl:template match="jobdoc:section" mode="reference">
		<xsl:param name="lang"/>
		<a class="doc">
      <xsl:attribute name="href">#<xsl:value-of select="@name"/>_<xsl:value-of select="$lang"/></xsl:attribute>
			<span style="font-family:Arial;font-size:12px;">&#8594;</span>
			<xsl:text> </xsl:text>Profile Settings: Section<xsl:text> </xsl:text>
		</a>
		<span class="sourceName"><xsl:value-of select="@name"/></span>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Setting -->
	<xsl:template match="jobdoc:setting">
		<xsl:param name="lang"/>
		<tr>
			<td class="section1">
				<xsl:call-template name="set_anchor"><xsl:with-param name="lang" select="$lang"/><xsl:with-param name="anchor_name" select="@id"/></xsl:call-template>
				<span class="sourceName"><xsl:value-of select="@name"/></span>
			</td>
			<td class="section2">
				<xsl:choose>
					<xsl:when test="@reference and not(@reference='')">
						<xsl:call-template name="process_reference"><xsl:with-param name="lang" select="$lang"/></xsl:call-template>
					</xsl:when>
					<xsl:otherwise>
						<xsl:variable name="defVal">
							<xsl:choose>
								<xsl:when test="@default_value and not(@default_value='')">
									<xsl:value-of select="@default_value"/>
								</xsl:when>
								<xsl:otherwise>---</xsl:otherwise>
							</xsl:choose>
						</xsl:variable>
						<span class="label">Default: <xsl:text> </xsl:text><xsl:value-of select="$defVal"/></span>
					</xsl:otherwise>
				</xsl:choose>
			</td>
		</tr>
		<xsl:if test="not(@reference)">
			<tr>
				<td class="section1">
					<xsl:if test="@required='false'">
						<span class="labelSmall">
							<xsl:text>[optional]</xsl:text>
						</span>
					</xsl:if>
					<xsl:if test="@required='true'">
						<xsl:text> </xsl:text>
					</xsl:if>
				</td>
				<td class="section2">
					<span class="desc">
						<xsl:if test="jobdoc:note[@language=$lang]">
							<xsl:apply-templates select="jobdoc:note[@language=$lang]"/>
						</xsl:if>
					</span>
				</td>
			</tr>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Setting als Referenz! -->
	<xsl:template match="jobdoc:setting" mode="reference">
		<xsl:param name="lang"/>
		<xsl:variable name="reftext">
			<xsl:choose>
				<xsl:when test="ancestor::jobdoc:profile">Profile-Setting</xsl:when>
				<xsl:otherwise>Connection Setting</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<a class="doc">
      <xsl:attribute name="href">#<xsl:value-of select="@id"/>_<xsl:value-of select="$lang"/></xsl:attribute>
			<span style="font-family:Arial;font-size:12px;">&#8594;</span>
			<xsl:text> </xsl:text>
			<xsl:value-of select="$reftext"/>
			<xsl:text> </xsl:text>
		</a>
		<span class="sourceName">
			<xsl:value-of select="@name"/>
		</span>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template Process Reference -->
	<xsl:template name="process_reference">
		<xsl:param name="lang"/>
		<xsl:variable name="reference" select="@reference"/>
		<xsl:apply-templates select="//*[@id=$reference]" mode="reference"><xsl:with-param name="lang" select="$lang"/></xsl:apply-templates>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template set_anchor -->
	<xsl:template name="set_anchor">
		<xsl:param name="lang"/>
		<xsl:param name="anchor_name"/>
		<a>
			<xsl:attribute name="name"><xsl:value-of select="$anchor_name"/>_<xsl:value-of select="$lang"/></xsl:attribute>
		</a>
	</xsl:template>
	<xsl:template match="jobdoc:note | jobdoc:changes | jobdoc:documentation" mode="reference">
		<xsl:apply-templates select="."/>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template get_resource -->
	<xsl:template name="get_resource">
		<xsl:param name="lang"/>
		<xsl:param name="resource_id"/>
		<a class="doc">
		  <xsl:attribute name="onclick">show_div('resources','<xsl:value-of select="$lang"/>');return false;</xsl:attribute>
			<xsl:attribute name="href">#<xsl:value-of select="$resource_id"/></xsl:attribute>
			<span style="font-family:Arial;font-size:12px;">&#8594;</span>
			<xsl:text> </xsl:text>
			<xsl:call-template name="get_label">
				<xsl:with-param name="lang" select="$lang"/>
				<xsl:with-param name="label_de" select="'Ressource: '"/>
				<xsl:with-param name="label_en" select="'Resource: '"/>
			</xsl:call-template>
			<span class="sourceName">
				<xsl:value-of select="//*[@id=$resource_id]/@file"/>
			</span>
		</a>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template get_label -->
	<xsl:template name="get_label">
		<xsl:param name="lang"/>
		<xsl:param name="label_de"/>
		<xsl:param name="label_en"/>
		<xsl:if test="$lang='de'">
			<xsl:value-of select="$label_de"/>
		</xsl:if>
		<xsl:if test="$lang='en'">
			<xsl:value-of select="$label_en"/>
		</xsl:if>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template get_css -->
	<xsl:template name="get_css">
		<style type="text/css"><![CDATA[

      body { background-color:#FFFFFF; margin-left:20px; font-family:verdana,arial,sans-serif;font-size:10pt; }

      ul    { margin-top: 0px; margin-bottom: 10px; margin-left: 0; padding-left: 1em;
              font-weight:300; }
      li    { margin-bottom: 10px; }

      h1    { font-family:verdana,arial,sans-serif;font-size:10pt; font-weight:600; }

      /* table { width:100%; background-color:#F3F4F4; } dunkler */
      /* table { width:100%; background-color:#F5FAFA; } heller */

      table.navi {background-color:#FFFFFF }

      table { font-family:verdana,arial,sans-serif;font-size:10pt;
              width:100%;
              background-color:#F9FCFC;
            }

      td {
        padding: 2px 2px 2px 2px;
        vertical-align:top; text-align: left;
        font-family:verdana, arial, sans-serif;font-size:10pt;
      }

      table.box        { border-width:2px; border-style:solid; border-color:#C0C0C0; }
      table.boxRelease { border-color:#C0C0C0;
                         border-left-width:2px;   border-left-style:solid;
                         border-right-width:2px;  border-right-style:solid;
                         border-top-width:1px;    border-top-style:none;
                         border-bottom-width:2px; border-bottom-style:solid;
                       }

      td.td1     {width:11%; }
      td.td2     {width:12%; }
      td.td3     {width:79%; }
      td.td2_3   {width:87%; }

      td.td31    {width:20%; }
      td.td32    {width:80%; }

      td.td31_32 {width:100%; }

      table.resource   { background-color:#DCDCDC; }

      th.resource1     { background-color:#F3F4F4;
                         text-align: left;
                         font-family:verdana,arial,sans-serif;font-size:10pt;
                         font-weight:300;
                         width:20%;
                       }
      th.resource2     { background-color:#F3F4F4;
                         text-align: left;
                         font-family:verdana,arial,sans-serif;font-size:10pt;
                         font-weight:300;
                         width:10%;
                       }
      th.resource3     { background-color:#F3F4F4;
                         text-align: left;
                         font-family:verdana,arial,sans-serif;font-size:10pt;
                         font-weight:300;
                         width:10%;
                       }
      th.resource2_3   { background-color:#F3F4F4;
                         text-align: left;
                         font-family:verdana,arial,sans-serif;font-size:10pt;
                         font-weight:300;
                         width:20%;
                       }
      th.resource4     { background-color:#F3F4F4;
                         text-align: left;
                         font-family:verdana,arial,sans-serif;font-size:10pt;
                         font-weight:300;
                         width:60%;
                       }

      td.resource1     { color:#009900;
                         background-color:#F3F4F4;
                         width:25%;
                       }
      td.resource2     { color:#336699;
                         background-color:#F3F4F4;
                         width:10%;
                       }
      td.resource3     { color:#336699;
                         background-color:#F3F4F4;
                         width:10%;
                       }
      td.resource2_3   { color:#336699;
                         background-color:#F3F4F4;
                         width:20%;
                       }
      td.resource4     { color:#336699;
                         background-color:#F3F4F4;
                         width:60%;
                       }

      table.section   { background-color:#DCDCDC; }

      td.section1     { color:#009900;
                        background-color:#F3F4F4;
                        width:20%;
                      }
      td.section2     { color:#336699;
                        font-weight:300;
                        background-color:#F3F4F4;
                        width:80%;
                      }

      .section        {color:#000000; font-weight:600; }                         /* schwarze Schrift, fett */
      .label          {color:#000000; font-weight:300; }
      .labelSmall     {color:#000000; font-weight:300; font-size:8pt; }          /* schwarze Schrift */
      .sourceName     {color:#009900; font-weight:300; }                         /* grüne Schrift */
      .sourceNameBold {color:#009900; font-weight:600; }                         /* grüne Schrift */
      .desc           {color:#336699; font-weight:300; }                         /* blaue Schrift */

      .code           {color:#000000; font-weight:300; font-family:"Courier New",sans-serif;font-size:10pt; }      /* Schrift für XML-Code */

      font.section        {color:#000000; font-weight:600; }                      /* schwarze Schrift, fett */
      font.label          {color:#000000; font-weight:300; }                      /* schwarze Schrift */
      font.labelSmall     {color:#000000; font-weight:300; font-size:8pt; }       /* schwarze Schrift */
      font.sourceName     {color:#009900; font-weight:300; }                      /* grüne Schrift */
      font.sourceNameBold {color:#009900; font-weight:600; }                      /* grüne Schrift */
      font.desc           {color:#336699; font-weight:300; }                      /* blaue Schrift */

      font.code           {color:#000000; font-weight:300; font-family:"Courier New",sans-serif;font-size:10pt; }      /* Schrift für XML-Code */
      .tooltip
       {
        position:absolute;
        width:400px; height:120px;
        display:none;
        background-color:#FFFFFF;
        border:1px solid;
        border-color:#FF6347;
        color:#000000;
        font-weight:300
       }

      /*** LINK Formatierungen ***/

      	a													{ font-weight:600; text-decoration:none; font-size:10pt; }

      /* Links Navigation */
	      a.navi:link								{ color:#FF9900; font-weight:600; font-size:12pt;}
	      a.navi:visited						{ color:#FF9900; font-weight:600; font-size:12pt;}
	      a.navi:hover							{ color:#FFCC00; font-weight:600; font-size:12pt;}
	      a.navi:active							{ color:#FF6347; font-weight:600; font-size:12pt;}

      /* Links für Sprachumschaltung */
	      a.lang								    { color:#660066; font-weight:600; font-size:10pt; text-decoration:underline; }

      /* Links im Doku-Text */
      /*a.doc:link								    { color:#663333; font-weight:300;}
      	a.doc:visited									{ color:#663333; font-weight:300;} */
      	a.doc:link								    { color:#FF9900; font-weight:300;}
      	a.doc:visited									{ color:#CC3300; font-weight:300;}
      	a.doc:hover										{ color:#FF9900; font-weight:300;}
      	a.doc:active									{ color:#FF6347; font-weight:300;}

      /* Mail-Verweis */
      	a.mail    						    { color:#FF9900; font-weight:300;}

    ]]></style>
	</xsl:template>
	<!-- ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Template get_js -->
	<xsl:template name="get_js">
		<script type="text/javascript"><xsl:text disable-output-escaping="yes"><![CDATA[

  var wmtt = null;
      document.onmousemove = updateWMTT;

      function updateWMTT(e) {
    var x = (document.all) ? window.event.x + document.body.scrollLeft : e.pageX;
    var y = (document.all) ? window.event.y + document.body.scrollTop  : e.pageY;
        if (wmtt != null) {
          wmtt.style.left = (x + 20) + "px";
          wmtt.style.top 	= (y + 20) + "px";
        }
      }

      function showWMTT(id) {
        wmtt = document.getElementById(id);
        wmtt.style.display = "block"
       }

      function hideWMTT() {
        wmtt.style.display = "none";
      }

      function switch_lang( lang ) {
        if ( lang == 'de' ) {
          document.getElementById('lang_de').style.display = '';
          document.getElementById('lang_en').style.display = 'none';
        } else {
          document.getElementById('lang_en').style.display = '';
          document.getElementById('lang_de').style.display = 'none';
        }
      }

      function show_div(id, lang) {
        if ( lang == 'de' ) {
          documentation_id = 'documentation_de';
          firstRelease_id  = 'firstRelease_de';
          resources_id     = 'resources_de';
          configuration_id = 'configuration_de';
          genXML_id        = 'genXML_de';
        } else {
          documentation_id = 'documentation_en';
          firstRelease_id  = 'firstRelease_en';
          resources_id     = 'resources_en';
          configuration_id = 'configuration_en';
          genXML_id        = 'genXML_en';
        }

        try {
          if ( id == 'all' ) {
            if ( document.getElementById(documentation_id)) { document.getElementById(documentation_id).style.display  = ''; }
            if ( document.getElementById(firstRelease_id))  { document.getElementById(firstRelease_id).style.display   = ''; }
            if ( document.getElementById(resources_id))     { document.getElementById(resources_id).style.display      = ''; }
            if (document.getElementById(configuration_id))  { document.getElementById(configuration_id).style.display  = ''; }
            hide_div(genXML_id);
          } else {
            switch (id) {
              case  'documentation':
                if ( document.getElementById('previousRelease_' + lang) ) { hide_div('previousRelease_' + lang); }
                hide_div(firstRelease_id);
                hide_div(resources_id);
                hide_div(configuration_id);
                hide_div(genXML_id);
                if (document.getElementById(documentation_id)) {
                  document.getElementById(documentation_id).style.display = '';
                } else {
                  if ( lang == 'de' ) {
                    alert( 'Für diesen Job gibt es keine Dokumentation' );
                  } else {
                    alert( 'There is no documentation for this job.' );
                  }
                }
                break;
              case  'firstRelease':
                hide_div(documentation_id);
                hide_div(resources_id);
                hide_div(configuration_id);
                hide_div(genXML_id);
                if (document.getElementById(firstRelease_id)) {
                  document.getElementById(firstRelease_id).style.display = '';
                } else {
                  if ( lang == 'de' ) {
                    alert( 'Für diesen Job gibt es keine Releases.' );
                  } else {
                    alert( 'There are no releases for this job.' );
                  }
                }
                break;
              case  'resources':
                hide_div(documentation_id);
                if ( document.getElementById('previousRelease_' + lang) ) { hide_div('previousRelease_' + lang); }
                hide_div(firstRelease_id);
                hide_div(configuration_id);
                hide_div(genXML_id);
                if (document.getElementById(resources_id)) {
                  document.getElementById(resources_id).style.display = '';
                } else {
                  if ( lang == 'de' ) {
                    alert( 'Für diesen Job gibt es keine Ressourcen.' );
                  } else {
                    alert( 'There are no resources for this job.' );
                  }
                }
                break;
              case  'configuration':
                hide_div(documentation_id);
                if ( document.getElementById('previousRelease_' + lang) ) { hide_div('previousRelease_' + lang); }
                hide_div(firstRelease_id);
                hide_div(resources_id);
                hide_div(genXML_id);
                if (document.getElementById(configuration_id)) {
                  document.getElementById(configuration_id).style.display = '';
                } else {
                  if ( lang == 'de' ) {
                    alert( 'Für diesen Job gibt es keine Konfiguration' );
                  } else {
                    alert( 'There is no configuration for this job.' );
                  }
                }
                break;
              case  'genXML':
                hide_div(documentation_id);
                if ( document.getElementById('previousRelease_' + lang) ) { hide_div('previousRelease_' + lang); }
                hide_div(firstRelease_id);
                hide_div(resources_id);
                hide_div(configuration_id);
                document.getElementById(genXML_id).style.display = '';
                break;
            }
          }
        }
        catch(x){
          alert('show_div : '+x.message);
         }
      }

      function hide_div( id ) {
        if ( document.getElementById(id) ) {
          try {
            document.getElementById(id).style.display = 'none';
          }
          catch(x) {
            alert('hide_div : '+x.message);
          }
        }
      }

      function showPreviousReleases( lang ) {

        ref = "javascript:hidePreviousReleases('" + lang + "')";
        if ( lang == "de" ) {
          linkTxt = 'Nur letztes Release anzeigen';
        } else {
          linkTxt = 'Show most recent Release';
        }

        document.getElementById('previousRelease_' + lang).style.display = '';
        document.getElementById('releaseLink_' + lang).innerHTML = linkTxt;
        document.getElementById('releaseLink_' + lang).href = ref;
      }

      function hidePreviousReleases( lang ) {

        document.getElementById('previousRelease_' + lang).style.display = 'none';
        ref = "javascript:showPreviousReleases('" + lang + "')";
        if ( lang == "de" ) {
          linkTxt = 'Ältere Releases anzeigen';
        } else {
          linkTxt = 'Show previous Releases';
        }

        document.getElementById('releaseLink_' + lang).innerHTML = linkTxt;
        document.getElementById('releaseLink_' + lang).href = ref;
      }

      function genXML( lang ) {
         show_div('genXML',lang);
      }

      function check_banner_gifs() {

        var gifs = document.getElementsByTagName("img");
        var gifs_complete = true;
        var gifs_length   = gifs.length;
        var i = 0;
        while( i != gifs_length ) {
          if( !gifs[i].complete ) {
            gifs_complete = false;
            break;
          }
          i++;
        }
        if( !gifs_complete ) {
          i = 0;
          while( i != gifs_length ) {

            try {
              gifs[i].parentNode.innerHTML = gifs[i].title;
            }
            catch(E) { break; }
          }
          i++;
        }
      }

      ]]></xsl:text></script>
	</xsl:template>
</xsl:stylesheet>
