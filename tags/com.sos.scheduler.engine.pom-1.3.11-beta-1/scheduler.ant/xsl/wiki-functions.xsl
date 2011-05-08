<?xml version="1.0" encoding="iso-8859-1"?>
<xsl:stylesheet version="2.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fn="http://www.w3.org/2004/07/xpath-functions"
	xmlns:xs="http://www.w3.org/2001/XMLSchema" xmlns:wiki="http://www.sos-berlin.com/xpath-functions">
	
	<xsl:import href="global-functions.xsl" />
	
	<xsl:param name="WIKI_VERSION"
		select="'pmwiki-2.2.0-beta63 ordered=1 urlencoded=1'" />
		
	<xsl:variable name="WLF" select="'%0a'" />
	<xsl:variable name="WLT" select="'%3c'" />
	<xsl:variable name="WGT" select="'%3e'" />

	<xsl:function name="wiki:meta">
		<xsl:param name="version" as="xs:string" />
		<xsl:param name="encoding" as="xs:string" />
		<xsl:param name="author" as="xs:string" />
		<xsl:param name="title" as="xs:string" />
		<xsl:text>version=</xsl:text>
		<xsl:value-of select="concat($version,$LF)" />
		<xsl:text>charset=</xsl:text>
		<xsl:value-of select="concat($encoding,$LF)" />
		<xsl:text>author=</xsl:text>
		<xsl:value-of select="concat($author,$LF)" />
		<xsl:text>title=</xsl:text>
		<xsl:value-of select="$title" />
		<xsl:value-of select="'&#010;'" />
	</xsl:function>

	<xsl:function name="wiki:copyright">
		<xsl:param name="copyright" as="xs:string" />
		<xsl:text />
		<xsl:value-of select="concat($WLF,'%right%%green%[--',$copyright,'--]',$WLF)" />
	</xsl:function>

	<xsl:function name="wiki:gettitle">
		<xsl:param name="title" as="xs:string" />
		<xsl:value-of select="concat('(:title ',$title,' :)',$WLF)" />
	</xsl:function>

	<xsl:function name="wiki:text">
		<xsl:param name="text" as="xs:string" />
		<xsl:text>text=</xsl:text>
		<xsl:value-of select="$text" />
	</xsl:function>

	<xsl:function name="wiki:pagename">
		<xsl:param name="group" as="xs:string" />
		<xsl:param name="name" as="xs:string" />
		<xsl:value-of select="concat(wiki:name($group),'.',wiki:name($name))" />
	</xsl:function>

	<xsl:function name="wiki:name">
		<xsl:param name="name" as="xs:string" />
		<xsl:variable name="iname" select="replace($name,'-','')" />
		<xsl:variable name="oname">
			<xsl:for-each select="tokenize($iname,'\.|\s')">
				<xsl:value-of select="concat(upper-case(substring(.,1,1)),substring(.,2))" />
			</xsl:for-each>
		</xsl:variable>
		<xsl:value-of select="$oname" />
	</xsl:function>

	<xsl:function name="wiki:sourcecode">
		<xsl:param name="source" as="xs:string" />
		<xsl:param name="language" as="xs:string" />
		<xsl:text>(:source lang=</xsl:text><xsl:value-of select="$language" /><xsl:text> -getcode  :)</xsl:text>
		<xsl:value-of select="$WLF" />
		<xsl:value-of select="$source" />
		<xsl:text>(:sourceend:)</xsl:text>
	</xsl:function>

</xsl:stylesheet>
