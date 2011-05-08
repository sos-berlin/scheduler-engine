<?xml version='1.0'?>

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

    <xsl:template match="/">

        <service_request url="http://localhost:4444/webdienst">
            <content>
                <xsl:copy-of select="/" />
            </content>
        </service_request>

    </xsl:template>

</xsl:stylesheet>
