<?xml version="1.0" encoding="iso-8859-1"?>
	<!--
		====================================================================================================
		Dieses Template generiert die Dokumentation von ant buildfiles im
		PMWiki-Format
		====================================================================================================
	-->
<xsl:stylesheet version="2.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fn="http://www.w3.org/2004/07/xpath-functions"
	xmlns:xs="http://www.w3.org/2001/XMLSchema" xmlns:sos="http://www.sos-berlin.com/xpath-functions"
	xmlns:wiki="http://www.sos-berlin.com/xpath-functions">

	<xsl:import href="wiki-functions.xsl" />
	<xsl:output method="xml" version="1.0" encoding="iso-8859-1"
		omit-xml-declaration="yes" />

	<xsl:strip-space elements="*" />

	<xsl:param name="P_AUTHOR" select="'SS'" />
	<xsl:param name="P_COPYRIGHT" select="''" />
	<xsl:param name="P_VERSION" select="$WIKI_VERSION" />
	<xsl:param name="P_ENCODING" select="'ISO-8859-1'" />

	<xsl:template match="project">
		<xsl:variable name="docname"
			select="wiki:pagename('Ant',sos:filename(document-uri(/)))" />
		<xsl:result-document href="{$docname}">
			<xsl:variable name="description" select="sos:getDescription(current())" />
			<xsl:variable name="title"
				select="concat(@name,' - ',sos:getTitle(current()))" />
			<xsl:variable name="text">
				<xsl:value-of select="wiki:gettitle($title)" />
				<xsl:value-of select="$WLF" />
				<xsl:value-of select="sos:getBody(current(),$description)" />
				<xsl:value-of select="$WLF" />
				<xsl:value-of select="sos:outputImports(name(),current())" />
				<xsl:value-of select="$WLF" />
				<xsl:value-of select="sos:outputProperties(name(),current())" />
				<xsl:value-of select="$WLF" />

				<xsl:value-of select="'!!! targets in diesem buildfile:'" />
				<xsl:value-of select="$WLF" />
				<xsl:choose>
					<xsl:when test="count(target)!=0">
						<xsl:text>(:pagelist group=</xsl:text>
						<xsl:value-of select="wiki:name(@name)" />
						<xsl:text> list=normal fmt=#title :)</xsl:text>
					</xsl:when>
					<xsl:otherwise>
						<xsl:text>* &lt;none&gt;</xsl:text>
					</xsl:otherwise>
				</xsl:choose>

				<xsl:value-of select="wiki:copyright($P_COPYRIGHT)" />
			</xsl:variable>
			<xsl:value-of select="wiki:meta($P_VERSION,$P_ENCODING,$P_AUTHOR,$title)" />
			<xsl:value-of select="wiki:text($text)" />
		</xsl:result-document>
		<xsl:apply-templates />
	</xsl:template>

	<xsl:template match="target">
		<xsl:variable name="docname" select="wiki:pagename(../@name,@name)" />
		<xsl:result-document href="{$docname}">
			<xsl:variable name="description" select="sos:getDescription(current())" />
			<xsl:variable name="title"
				select="concat(@name,' - ',sos:getTitle(current()))" />
			<xsl:variable name="text">
				<xsl:value-of select="wiki:gettitle($title)" />
				<xsl:variable name="visible"
					select="if (substring(@name,1,1)='-') then 'no' else 'yes'" />
				<xsl:value-of select="$WLF" />
				<xsl:value-of select="sos:getBody(current(),$description)" />
				<xsl:value-of select="$WLF" />
				<xsl:value-of select="concat('* visible: .:', $visible , ':.' ,$WLF)" />
				<xsl:value-of select="$WLF" />
				<xsl:value-of select="$WLF" />

				<xsl:variable name="target" select="@name" />
				<xsl:variable name="source">
					<xsl:apply-templates select="//target[@name=$target]"
						mode="formatted.output" />
				</xsl:variable>
				<xsl:value-of select="'!!! sourcecode'" />
				<xsl:value-of select="wiki:sourcecode($source,'xml')" />
				<xsl:value-of select="$WLF" />

				<xsl:value-of select="$WLF" />
				<xsl:value-of select="sos:outputImports(name(),current())" />

				<xsl:value-of select="$WLF" />
				<xsl:value-of select="sos:outputProperties(name(),current())" />

				<xsl:text>!!! properties need by this target</xsl:text>
				<xsl:value-of select="$WLF" />
				<xsl:variable name="regex" select="'\$\{([^{}]*)\}'" />
				<xsl:choose>
					<xsl:when test="matches($source,$regex,'m')">
						<xsl:variable name="properties">
							<xsl:for-each select="descendant::*/@*">
								<xsl:analyze-string select="." regex="{$regex}">
									<xsl:matching-substring>
										<xsl:element name="property">
											<xsl:attribute name="name">
									<xsl:value-of select="regex-group(1)" />
									</xsl:attribute>
										</xsl:element>
									</xsl:matching-substring>
								</xsl:analyze-string>
							</xsl:for-each>
						</xsl:variable>
						<xsl:for-each-group select="$properties/property"
							group-by="@name">
							<xsl:sort select="@name" />
							<xsl:variable name="name" select="@name" />
							<xsl:value-of
								select="concat('* ',$name,' (',count($properties/property[@name=$name]),')',$WLF)" />
						</xsl:for-each-group>
					</xsl:when>
					<xsl:otherwise>
						<xsl:text>* &lt;none&gt;</xsl:text>
					</xsl:otherwise>
				</xsl:choose>

				<xsl:value-of select="wiki:copyright($P_COPYRIGHT)" />
			</xsl:variable>
			<xsl:value-of select="wiki:meta($P_VERSION,$P_ENCODING,$P_AUTHOR,$title)" />
			<xsl:value-of select="wiki:text($text)" />
		</xsl:result-document>
	</xsl:template>

	<xsl:template match="property[@name!='']" mode="single">
		<xsl:text>* </xsl:text>
		<xsl:value-of select="@name" />
		<xsl:text> %green%= </xsl:text>
		<xsl:value-of select="concat(@value,'%%',$WLF)" />
	</xsl:template>

	<xsl:template match="property[@file!='']|propertyxml[@file!='']|import"
		mode="file">
		<xsl:text>* </xsl:text>
		<xsl:value-of select="concat(@file,$WLF)" />
	</xsl:template>

	<xsl:function name="sos:getTitle">
		<xsl:param name="node" />
		<xsl:variable name="rawdescription">
			<xsl:choose>
				<xsl:when test="$node/@description!=''">
					<xsl:value-of select="normalize-space($node/@description)" />
				</xsl:when>
				<xsl:otherwise>
					<xsl:choose>
						<xsl:when
							test="normalize-space($node/preceding::comment()[1])!='' and $node/preceding::comment()[1] = $node/preceding::node()[1]">
							<xsl:value-of select="normalize-space($node/preceding::comment()[1])" />
						</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="concat($WLT,'no description',$WGT)" />
						</xsl:otherwise>
					</xsl:choose>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<xsl:variable name="description"
			select="replace(replace($rawdescription,'&lt;',$WLT),'&gt;',$WGT)" />
		<xsl:value-of select="normalize-space( tokenize($description,'\\\\')[1] )" />
	</xsl:function>

	<xsl:function name="sos:getBody">
		<xsl:param name="node" />
		<xsl:param name="description" as="xs:string" />
		<xsl:variable name="lines" select="tokenize($description,'\\\\')" />
		<xsl:for-each select="$lines">
			<xsl:if test="position()!=1 or $node/@description!=''">
				<xsl:value-of select="concat(normalize-space(.),$WLF)" />
			</xsl:if>
		</xsl:for-each>
	</xsl:function>

	<xsl:function name="sos:getDescription">
		<xsl:param name="node" />
		<xsl:variable name="description">
			<xsl:value-of select="normalize-space($node/preceding::comment()[1])" />
		</xsl:variable>
		<xsl:value-of select="replace(replace($description,'&lt;',$WLT),'&gt;',$WGT)" />
	</xsl:function>

	<xsl:function name="sos:outputProperties">
		<xsl:param name="context" />
		<xsl:param name="node" />

		<xsl:text>!!! properties set by this </xsl:text>
		<xsl:value-of select="concat($context,':')" />
		<xsl:value-of select="$WLF" />
		<xsl:choose>
			<xsl:when test="count($node/property[@name!=''])!=0">
				<xsl:apply-templates select="$node/property"
					mode="single" />
			</xsl:when>
			<xsl:otherwise>
				<xsl:text>* &lt;none&gt;</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:value-of select="$WLF" />

		<xsl:text>!!! property-files included by this </xsl:text>
		<xsl:value-of select="concat($context,':')" />
		<xsl:value-of select="$WLF" />
		<xsl:choose>
			<xsl:when
				test="(count($node/property[@file!=''])+count($node/propertyxml[@file!='']))!=0">
				<xsl:apply-templates select="$node/property"
					mode="file" />
				<xsl:apply-templates select="$node/propertyxml"
					mode="file" />
			</xsl:when>
			<xsl:otherwise>
				<xsl:text>* &lt;none&gt;</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:value-of select="$WLF" />

	</xsl:function>

	<xsl:function name="sos:outputImports">
		<xsl:param name="context" />
		<xsl:param name="node" />

		<xsl:text>!!! files imported by this </xsl:text>
		<xsl:value-of select="concat($context,':')" />
		<xsl:value-of select="$WLF" />
		<xsl:choose>
			<xsl:when test="count($node/import)!=0">
				<xsl:apply-templates select="$node/import" mode="file" />
			</xsl:when>
			<xsl:otherwise>
				<xsl:text>* &lt;none&gt;</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:value-of select="$WLF" />

	</xsl:function>

	<!--
		====================================================================================================
	-->
	<!--
		templates für die formatierte Ausgabe der targets als xml
	-->
	<!--
		====================================================================================================
	-->
	<xsl:template match="*" mode="formatted.output">
		<xsl:param name="spaces" select="''" />
		<xsl:value-of select="concat($spaces,$WLT,node-name(.))" />
		<xsl:apply-templates select="@*" mode="formatted.output" />

		<xsl:choose>
			<xsl:when test="count(child::*)=0">
				<xsl:choose>
					<xsl:when test="current()=''">
						<xsl:value-of select="concat('/',$WGT,$WLF)" />
					</xsl:when>
					<xsl:otherwise>
						<xsl:value-of
							select="concat($WGT,current(),$WLT,'/',node-name(.),$WGT,$WLF)" />
					</xsl:otherwise>
				</xsl:choose>
			</xsl:when>
			<xsl:otherwise>
				<xsl:if test="text()!=''">
					<xsl:value-of select="concat($WGT,text(),$WLF)" />
				</xsl:if>
				<xsl:value-of select="concat($WGT,$WLF)" />
				<xsl:apply-templates select="*" mode="formatted.output">
					<xsl:with-param name="spaces" select="concat($spaces,'   ')" />
				</xsl:apply-templates>
				<xsl:value-of select="concat($spaces,$WLT,'/',node-name(.),$WGT,$WLF)" />
			</xsl:otherwise>
		</xsl:choose>

	</xsl:template>

	<xsl:template match="text()" mode="formatted.output">
		<xsl:value-of select="current()" />
	</xsl:template>

	<xsl:template match="@*" mode="formatted.output">
		<xsl:value-of select="concat(' ',node-name(.),'=&quot;',.,'&quot;')" />
	</xsl:template>

</xsl:stylesheet>
