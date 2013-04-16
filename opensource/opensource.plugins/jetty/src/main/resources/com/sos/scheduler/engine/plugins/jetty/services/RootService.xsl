<?xml version='1.0' encoding='utf-8'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <xsl:output method="html" doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"/>

    <xsl:template match="/engine">
        <html>
            <head>
                <title>
                    <xsl:text>JobScheduler</xsl:text>
                </title>
            </head>
            <body>
                <h3>
                    <xsl:text>JobScheduler</xsl:text>
                </h3>
                <p>
                    <a href="folders?type=job&amp;folder=/">Jobs</a>
                </p>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
