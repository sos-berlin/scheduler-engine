<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:s="http://www.sos-berlin.com/schema/scheduler_job_documentation_v1.0" >
	<xsl:output method="text" encoding="ISO-8859-1" />
	<xsl:strip-space elements="*"/>
	
   <xsl:param name="P_WIKI_LANGUAGE" select="'de'"/>
   <xsl:param name="P_WIKI_FOLDER"   select="'htm/job'"/>
	<xsl:param name="P_WIKI_GROUP"    select="'Job'"/>
	<xsl:param name="P_WIKI_PREFIX"   select="''"/>
   <xsl:param name="P_WIKI_VERSION"  select="'pmwiki-2.2.0-beta63 ordered=1 urlencoded=1'" />
   <xsl:param name="P_WIKI_ENCODING" select="'ISO-8859-1'" />
   
   <!--  Konstanten --> 
   <xsl:variable name="LF" select="'%0a'" />
		
	<xsl:template match="/s:description">version=<xsl:value-of select="$P_WIKI_VERSION" />
charset=<xsl:value-of select="$P_WIKI_ENCODING" />
encoding=<xsl:value-of select="$P_WIKI_ENCODING" />
author=<xsl:value-of select="s:releases/s:release/s:author/@name" />
text=<xsl:apply-templates />
title=<xsl:value-of select="concat(s:job/@name,': ',s:job/@title)" />
	</xsl:template>
   
   <xsl:template match="s:job">(:title <xsl:value-of select="concat(@name,': ',@title)" /> :)<xsl:value-of select="$LF" />(:includeurl <xsl:value-of select="concat($P_WIKI_FOLDER,'/', @name,'.html')" /> :)  
   </xsl:template>

	<xsl:template match="*|@*|text()">
	</xsl:template>

</xsl:stylesheet>
