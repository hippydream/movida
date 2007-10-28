<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns="http://www.w3.org/TR/xhtml1/strict">

<xsl:output
   method="xml"
   indent="yes"
   encoding="utf-8"
/>

<xsl:template match="movie">
	<html>

		<head>
			<title>
				<xsl:value-of select="title"/>
				<xsl:if test="release-year and release-year != ''">
					<xsl:text> (</xsl:text>
					<xsl:value-of select="release-year"/>
					<xsl:text>)</xsl:text>
				</xsl:if>
			</title>

      <style>
        background-color:#DBECED;
      </style>
		</head>

		<body>

			<h1><xsl:value-of select="title"/></h1>

			<dt>Original title:</dt>
			<dd><xsl:value-of select="original-title"/></dd>

			<dt>Edition:</dt>
			<dd><xsl:value-of select="edition"/></dd>

			<dt>Directed by:</dt>
			<xsl:for-each select="directors">
				<xsl:for-each select="person">
					<dd><xsl:value-of select="name"/></dd>
				</xsl:for-each>
			</xsl:for-each>

			<dt>Produced by:</dt>
			<xsl:for-each select="producers">
				<xsl:for-each select="person">
					<dd><xsl:value-of select="name"/></dd>
				</xsl:for-each>
			</xsl:for-each>

			<dt>Release year:</dt>
			<dd><xsl:value-of select="release-year"/></dd>

			<dt>Production year:</dt>
			<dd><xsl:value-of select="production-year"/></dd>

			<dt>Running time:</dt>
			<dd><xsl:value-of select="running-time"/> min.</dd>

			<dt>Rating:</dt>
			<xsl:choose>
				<xsl:when test="rating and rating != ''">
					<dd><xsl:value-of select="rating"/>/5</dd>
				</xsl:when>
				<xsl:otherwise>
					<dd>N.A.</dd>
				</xsl:otherwise>
			</xsl:choose>

			<dt>Languages:</dt>
			<xsl:for-each select="languages">
				<xsl:for-each select="language">
					<dd><xsl:value-of select="."/></dd>
				</xsl:for-each>
			</xsl:for-each>

			<dt>Countries:</dt>
			<xsl:for-each select="countries">
				<xsl:for-each select="country">
					<dd><xsl:value-of select="."/></dd>
				</xsl:for-each>
			</xsl:for-each>

			<dt>Cast:</dt>
			<xsl:for-each select="cast">
				<xsl:for-each select="person">
					<dd><xsl:value-of select="name"/></dd>
				</xsl:for-each>
			</xsl:for-each>

			<dt>Other crew members:</dt>
			<xsl:for-each select="crew">
				<xsl:for-each select="person">
					<dd><xsl:value-of select="name"/></dd>
				</xsl:for-each>
			</xsl:for-each>

		</body>

	</html>

</xsl:template>

</xsl:stylesheet>