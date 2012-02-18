<?xml version='1.0' encoding='utf-8'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:output method="html" doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"/>

    <xsl:template match="/names">
        <html>
            <head>
                <title>
                    <xsl:text>JobScheduler </xsl:text>
                    <xsl:value-of select="@type"/>
                    <xsl:text> </xsl:text>
                    <xsl:value-of select="@folder"/>
                </title>
            </head>
            <body>
                <h3>
                    <xsl:text>Folder </xsl:text>
                    <xsl:value-of select="@folder"/>
                </h3>
                <xsl:for-each select="name">
                    <p>
                        <a href="{@uri}">
                            <xsl:value-of select="@name"/>
                        </a>
                    </p>
                </xsl:for-each>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
