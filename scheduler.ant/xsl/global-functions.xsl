<?xml version="1.0" encoding="iso-8859-1"?>
<xsl:stylesheet version="2.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fn="http://www.w3.org/2004/07/xpath-functions"
	xmlns:xs="http://www.w3.org/2001/XMLSchema" xmlns:sos="http://www.sos-berlin.com/xpath-functions">

	<xsl:variable name="LF" select="'&#010;'" />
	<xsl:variable name="CR" select="'&#013;'" />
	<xsl:variable name="CRLF" select="concat($CR,$LF)" />

	<xsl:function name="sos:fileandextension">
	<xsl:param name="fullname" as="xs:string" />
		<xsl:value-of select="tokenize($fullname, '/')[last()]" />
	</xsl:function>

	<xsl:function name="sos:filename">
	<xsl:param name="fullname" as="xs:string" />
		<xsl:value-of
			select="replace( sos:fileandextension($fullname), sos:extension($fullname), '')" />
	</xsl:function>

	<xsl:function name="sos:extension">
	<xsl:param name="fullname" as="xs:string" />
		<xsl:value-of
			select="tokenize(sos:fileandextension($fullname), '\.')[last()]" />
	</xsl:function>

</xsl:stylesheet>
