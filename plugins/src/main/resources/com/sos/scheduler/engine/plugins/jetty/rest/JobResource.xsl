<?xml version='1.0' encoding='utf-8'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:output method="html" doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"/>

    <xsl:template match="/job">
        <html>
            <head>
                <title>
                    <xsl:text>JobScheduler Job </xsl:text>
                    <xsl:value-of select="@job"/>
                </title>
            </head>
            <body>
                <h3>
                    <xsl:text>Job </xsl:text>
                    <xsl:value-of select="@job"/>
                </h3>
                <p>
                    <a href="{@uri}/description?job={@job}">Description</a>
                </p>
                <p>
                    <a href="{@uri}/configuration?job={@job}">Configuration</a>
                </p>
                <p>
                    <a href="{@uri}/log.snapshot?job={@job}">Log snapshot</a>
                </p>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
