#include "../../include/MimeTypes.hpp"
#include "TestUtils.hpp"

// ------------------------------------------------------- text types -------------------------------------------------

static void testMimeHtml()
{
	ASSERT_EQ(std::string("text/html"), MimeTypes::getType("index.html"));
	ASSERT_EQ(std::string("text/html"), MimeTypes::getType("page.htm"));
}

static void testMimeCss()
{
	ASSERT_EQ(std::string("text/css"), MimeTypes::getType("style.css"));
}

static void testMimeJs()
{
	ASSERT_EQ(std::string("application/javascript"), MimeTypes::getType("app.js"));
}

static void testMimeJson()
{
	ASSERT_EQ(std::string("application/json"), MimeTypes::getType("data.json"));
}

static void testMimeXml()
{
	ASSERT_EQ(std::string("application/xml"), MimeTypes::getType("feed.xml"));
}

static void testMimeTxt()
{
	ASSERT_EQ(std::string("text/plain"), MimeTypes::getType("readme.txt"));
}

static void testMimeCsv()
{
	ASSERT_EQ(std::string("text/csv"), MimeTypes::getType("data.csv"));
}

// ------------------------------------------------------- image types -------------------------------------------------

static void testMimePng()
{
	ASSERT_EQ(std::string("image/png"), MimeTypes::getType("photo.png"));
}

static void testMimeJpg()
{
	ASSERT_EQ(std::string("image/jpeg"), MimeTypes::getType("photo.jpg"));
	ASSERT_EQ(std::string("image/jpeg"), MimeTypes::getType("photo.jpeg"));
}

static void testMimeGif()
{
	ASSERT_EQ(std::string("image/gif"), MimeTypes::getType("anim.gif"));
}

static void testMimeIco()
{
	ASSERT_EQ(std::string("image/x-icon"), MimeTypes::getType("favicon.ico"));
}

static void testMimeSvg()
{
	ASSERT_EQ(std::string("image/svg+xml"), MimeTypes::getType("logo.svg"));
}

static void testMimeWebp()
{
	ASSERT_EQ(std::string("image/webp"), MimeTypes::getType("img.webp"));
}

// ------------------------------------------------------- document/archive types -------------------------------------------------

static void testMimePdf()
{
	ASSERT_EQ(std::string("application/pdf"), MimeTypes::getType("doc.pdf"));
}

static void testMimeZip()
{
	ASSERT_EQ(std::string("application/zip"), MimeTypes::getType("archive.zip"));
}

// ------------------------------------------------------- unknown / no extension -------------------------------------------------

static void testMimeUnknownExtension()
{
	// Unknown extension falls back to application/octet-stream
	ASSERT_EQ(std::string("application/octet-stream"), MimeTypes::getType("file.xyz123"));
}

static void testMimeNoExtension()
{
	ASSERT_EQ(std::string("application/octet-stream"), MimeTypes::getType("Makefile"));
	ASSERT_EQ(std::string("application/octet-stream"), MimeTypes::getType("binary"));
}

static void testMimeEmptyPath()
{
	ASSERT_EQ(std::string("application/octet-stream"), MimeTypes::getType(""));
}

// ------------------------------------------------------- case-insensitivity of extension -------------------------------------------------

static void testMimeCaseInsensitiveExtension()
{
	// getType lowercases the extension before lookup
	ASSERT_EQ(std::string("text/html"), MimeTypes::getType("index.HTML"));
	ASSERT_EQ(std::string("image/png"), MimeTypes::getType("photo.PNG"));
	ASSERT_EQ(std::string("text/css"),  MimeTypes::getType("style.CSS"));
}

// ------------------------------------------------------- path with directories -------------------------------------------------

static void testMimeFullPath()
{
	ASSERT_EQ(std::string("text/html"), MimeTypes::getType("/var/www/html/index.html"));
	ASSERT_EQ(std::string("image/jpeg"), MimeTypes::getType("./static/images/hero.jpg"));
}

static void testMimeDotInDirectory()
{
	// rfind('.') should grab the last dot — the extension, not a dot in a dir name
	ASSERT_EQ(std::string("text/css"), MimeTypes::getType("/my.site.com/style.css"));
}

// ------------------------------------------------------- entry point -------------------------------------------------
void testMimeTypes()
{
	RUN_TEST(testMimeHtml);
	RUN_TEST(testMimeCss);
	RUN_TEST(testMimeJs);
	RUN_TEST(testMimeJson);
	RUN_TEST(testMimeXml);
	RUN_TEST(testMimeTxt);
	RUN_TEST(testMimeCsv);
	RUN_TEST(testMimePng);
	RUN_TEST(testMimeJpg);
	RUN_TEST(testMimeGif);
	RUN_TEST(testMimeIco);
	RUN_TEST(testMimeSvg);
	RUN_TEST(testMimeWebp);
	RUN_TEST(testMimePdf);
	RUN_TEST(testMimeZip);
	RUN_TEST(testMimeUnknownExtension);
	RUN_TEST(testMimeNoExtension);
	RUN_TEST(testMimeEmptyPath);
	RUN_TEST(testMimeCaseInsensitiveExtension);
	RUN_TEST(testMimeFullPath);
	RUN_TEST(testMimeDotInDirectory);
}
