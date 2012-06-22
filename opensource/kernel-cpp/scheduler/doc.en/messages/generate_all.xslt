<?xml version='1.0'?>
<!-- $Id$ -->

<!-- xsl:document scheint eine Besonderheit von Daniel Veillards xsltproc zu sein. -->

<!-- Erzeugt für jeden Meldungscode eine XML-Datei im Verzeichnis all. -->


<xsl:stylesheet xmlns:xsl   = "http://www.w3.org/1999/XSL/Transform"
            version = "1.0">

    <xsl:template match="messages">
        <xsl:for-each select=".//message">
            <xsl:document href="{concat( 'all/', @code, '.xml' )}">
                <xsl:processing-instruction name="xml-stylesheet">href="../<xsl:value-of select="/*/@base_dir"/>scheduler.xsl" type="text/xsl"</xsl:processing-instruction>

                <xsl:element name="message">
                    <xsl:attribute name="base_dir"><xsl:value-of select="/*/@base_dir"/>../</xsl:attribute>
                    <xsl:copy-of select="@code"/>
                    <xsl:copy-of select="text"/>
                </xsl:element>
            </xsl:document>
        </xsl:for-each>
    </xsl:template>

</xsl:stylesheet>
