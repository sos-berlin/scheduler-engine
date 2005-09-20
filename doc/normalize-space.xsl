<?xml version='1.0'?>
<!-- $Id$      Zschimmer GmbH -->


<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">


<!-- Entfernt viele Blanks aus einem XML-Dokument (für die Scheduler-Doku) -->
            

<xsl:output method="xml" encoding="utf-8"/>

    
<xsl:template match="node()">
    <xsl:copy>
        <xsl:for-each select="@*">
            <xsl:copy><xsl:value-of select="."/></xsl:copy>
        </xsl:for-each>

        <xsl:apply-templates/>
    </xsl:copy>
</xsl:template>


<xsl:template match="text() [ normalize-space( . ) = '' ]" >
    <!-- Leeren Text unterdrücken -->
</xsl:template>


<!--
<xsl:template 
    match="description        / text() |
           description.style  / text() |
           event        / text() |
           example      / text() |
           title        / text() |
            
           dd           / text() |
           div          / text() |
           h1           / text() |
           h2           / text() |
           h3           / text() |
           h4           / text() |
           li           / text() |
           p            / text() |
           span         / text() |
           td           / text() |"
    >
-->
<xsl:template match="* / text() [ not( ancestor::pre ) ] ">
    
    <xsl:if test="normalize-space(.) != ''">

        <xsl:if test="contains( ' &#9;&#10;&#13;', substring( ., 1, 1 ) )">
            <xsl:text> </xsl:text>
        </xsl:if>

        <xsl:value-of select="normalize-space(.)" />

        <xsl:if test="contains( ' &#9;&#10;&#13;', substring( ., string-length( . ) ) )">
            <xsl:text> </xsl:text>
        </xsl:if>

    </xsl:if>

</xsl:template>



<xsl:template match="comment()" >
</xsl:template>

</xsl:stylesheet>
