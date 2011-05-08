<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  xmlns:xi="http://www.w3.org/1999/XML/xinclude">
	<xsl:output method="text" encoding="ISO-8859-1" />
	<xsl:strip-space elements="*|@*|text()" />
	
   <xsl:variable name="LF" select="'%0a%'" />
   <xsl:variable name="VERSION" select="'pmwiki-2.2.0-beta63 ordered=1 urlencoded=1'" />
   <xsl:variable name="ENCODING" select="'ISO-8859-1'" />
	
	<xsl:template match="/">
      <xsl:apply-templates />
	</xsl:template>

	<xsl:template match="sosweb-users">version=<xsl:value-of select="$VERSION" />
charset=<xsl:value-of select="$ENCODING" />
author=SS
text=<xsl:apply-templates />
	</xsl:template>

	<xsl:template match="user">
	  <xsl:value-of select="concat(@name,$LF)" />
	</xsl:template>

	<xsl:template match="*|@*|text()">
	</xsl:template>
	
</xsl:stylesheet>
