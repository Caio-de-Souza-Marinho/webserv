#include "../../include/MultipartParser.hpp"
#include "TestUtils.hpp"

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string makePart(const std::string &name, const std::string &filename,
	const std::string &content)
{
	std::string part;
	part += "Content-Disposition: form-data; name=\"" + name + "\"; filename=\"" + filename + "\"\r\n";
	part += "Content-Type: application/octet-stream\r\n";
	part += "\r\n";
	part += content;
	return (part);
}

static std::string buildBody(const std::string &boundary,
	const std::vector<std::string> &rawParts)
{
	std::string body;
	for (size_t i = 0; i < rawParts.size(); ++i)
	{
		body += "--" + boundary + "\r\n";
		body += rawParts[i];
		body += "\r\n";
	}
	body += "--" + boundary + "--\r\n";
	return (body);
}

// ---------------------------------------------------------------------------
// isMultipart
// ---------------------------------------------------------------------------

static void testIsMultipartTrue()
{
	MultipartParser parser;

	ASSERT_TRUE(parser.isMultipart("multipart/form-data; boundary=abc123"));
	ASSERT_TRUE(parser.isMultipart("multipart/form-data; boundary=\"quoted\""));
	// Note: implementation is case-sensitive; uppercase media type does NOT match
	ASSERT_TRUE(!parser.isMultipart("MULTIPART/form-data; boundary=x"));
}

static void testIsMultipartFalse()
{
	MultipartParser parser;

	ASSERT_TRUE(!parser.isMultipart("application/json"));
	ASSERT_TRUE(!parser.isMultipart("text/plain"));
	ASSERT_TRUE(!parser.isMultipart(""));
	ASSERT_TRUE(!parser.isMultipart("application/x-www-form-urlencoded"));
}

// ---------------------------------------------------------------------------
// parse – single file
// ---------------------------------------------------------------------------

static void testParseSingleFile()
{
	MultipartParser parser;

	std::string boundary = "----WebKitFormBoundary7MA4YWxk";
	std::string ct = "multipart/form-data; boundary=" + boundary;

	std::vector<std::string> parts;
	parts.push_back(makePart("file", "hello.txt", "Hello, world!"));
	std::string body = buildBody(boundary, parts);

	std::vector<MultipartPart> result = parser.parse(ct, body);

	ASSERT_EQ(1u, result.size());
	ASSERT_EQ(std::string("hello.txt"), result[0].filename);
	ASSERT_EQ(std::string("Hello, world!"), result[0].content);
}

// ---------------------------------------------------------------------------
// parse – multiple files
// ---------------------------------------------------------------------------

static void testParseMultipleFiles()
{
	MultipartParser parser;

	std::string boundary = "boundary42";
	std::string ct = "multipart/form-data; boundary=" + boundary;

	std::vector<std::string> parts;
	parts.push_back(makePart("file1", "a.txt", "aaa"));
	parts.push_back(makePart("file2", "b.txt", "bbb"));
	parts.push_back(makePart("file3", "c.bin", "ccc"));
	std::string body = buildBody(boundary, parts);

	std::vector<MultipartPart> result = parser.parse(ct, body);

	ASSERT_EQ(3u, result.size());
	ASSERT_EQ(std::string("a.txt"), result[0].filename);
	ASSERT_EQ(std::string("aaa"),   result[0].content);
	ASSERT_EQ(std::string("b.txt"), result[1].filename);
	ASSERT_EQ(std::string("bbb"),   result[1].content);
	ASSERT_EQ(std::string("c.bin"), result[2].filename);
	ASSERT_EQ(std::string("ccc"),   result[2].content);
}

// ---------------------------------------------------------------------------
// parse – empty body / missing boundary
// ---------------------------------------------------------------------------

static void testParseEmptyBody()
{
	MultipartParser parser;

	std::string ct = "multipart/form-data; boundary=abc";
	std::vector<MultipartPart> result = parser.parse(ct, "");

	ASSERT_EQ(0u, result.size());
}

static void testParseMissingBoundaryInContentType()
{
	MultipartParser parser;

	std::string ct = "multipart/form-data";
	std::string body = "--abc\r\nContent-Disposition: form-data; name=\"f\"; filename=\"x.txt\"\r\n\r\ndata\r\n--abc--\r\n";

	std::vector<MultipartPart> result = parser.parse(ct, body);
	ASSERT_EQ(0u, result.size());
}

static void testParseWrongContentType()
{
	MultipartParser parser;

	std::string ct = "application/json";
	std::string body = "--abc\r\nContent-Disposition: form-data; name=\"f\"; filename=\"x.txt\"\r\n\r\ndata\r\n--abc--\r\n";

	// isMultipart is false; parse still returns empty since boundary extraction fails
	std::vector<MultipartPart> result = parser.parse(ct, body);
	ASSERT_EQ(0u, result.size());
}

// ---------------------------------------------------------------------------
// parse – parts without filename are ignored
// ---------------------------------------------------------------------------

static void testParseFieldWithoutFilenameIgnored()
{
	MultipartParser parser;

	std::string boundary = "bound";
	std::string ct = "multipart/form-data; boundary=" + boundary;

	// A plain text field (no filename=)
	std::string textField =
		"Content-Disposition: form-data; name=\"username\"\r\n"
		"\r\n"
		"caio";

	std::vector<std::string> parts;
	parts.push_back(textField);
	parts.push_back(makePart("upload", "file.txt", "data"));
	std::string body = buildBody(boundary, parts);

	std::vector<MultipartPart> result = parser.parse(ct, body);

	// Only the part with filename= should be returned
	ASSERT_EQ(1u, result.size());
	ASSERT_EQ(std::string("file.txt"), result[0].filename);
}

// ---------------------------------------------------------------------------
// parse – binary / null-byte content
// ---------------------------------------------------------------------------

static void testParseBinaryContent()
{
	MultipartParser parser;

	std::string boundary = "binbound";
	std::string ct = "multipart/form-data; boundary=" + boundary;

	// Build a binary payload with embedded null and non-printable bytes
	std::string binaryContent;
	binaryContent += '\x00';
	binaryContent += '\xFF';
	binaryContent += '\x89';
	binaryContent += "PNG";

	std::vector<std::string> parts;
	parts.push_back(makePart("img", "image.png", binaryContent));
	std::string body = buildBody(boundary, parts);

	std::vector<MultipartPart> result = parser.parse(ct, body);

	ASSERT_EQ(1u, result.size());
	ASSERT_EQ(std::string("image.png"), result[0].filename);
	ASSERT_EQ(binaryContent.size(), result[0].content.size());
}

// ---------------------------------------------------------------------------
// parse – quoted boundary in Content-Type
// ---------------------------------------------------------------------------

static void testParseQuotedBoundary()
{
	MultipartParser parser;

	std::string boundary = "myBound";
	std::string ct = "multipart/form-data; boundary=\"" + boundary + "\"";

	std::vector<std::string> parts;
	parts.push_back(makePart("f", "q.txt", "quoted boundary test"));
	std::string body = buildBody(boundary, parts);

	std::vector<MultipartPart> result = parser.parse(ct, body);

	ASSERT_EQ(1u, result.size());
	ASSERT_EQ(std::string("q.txt"), result[0].filename);
	ASSERT_EQ(std::string("quoted boundary test"), result[0].content);
}

// ---------------------------------------------------------------------------
// parse – filename with spaces and special chars
// ---------------------------------------------------------------------------

static void testParseFilenameWithSpaces()
{
	MultipartParser parser;

	std::string boundary = "sbound";
	std::string ct = "multipart/form-data; boundary=" + boundary;

	std::vector<std::string> parts;
	parts.push_back(makePart("doc", "my document (2024).pdf", "pdf content"));
	std::string body = buildBody(boundary, parts);

	std::vector<MultipartPart> result = parser.parse(ct, body);

	ASSERT_EQ(1u, result.size());
	ASSERT_EQ(std::string("my document (2024).pdf"), result[0].filename);
}

// ---------------------------------------------------------------------------
// parse – body with no delimiter at all
// ---------------------------------------------------------------------------

static void testParseBodyWithNoDelimiter()
{
	MultipartParser parser;

	std::string ct = "multipart/form-data; boundary=BOUND";
	std::string body = "this body does not contain any boundary marker";

	std::vector<MultipartPart> result = parser.parse(ct, body);
	ASSERT_EQ(0u, result.size());
}

// ---------------------------------------------------------------------------
// parse – large content
// ---------------------------------------------------------------------------

static void testParseLargeContent()
{
	MultipartParser parser;

	std::string boundary = "largebound";
	std::string ct = "multipart/form-data; boundary=" + boundary;

	std::string largeContent(1024 * 1024, 'A'); // 1 MB of 'A'
	std::vector<std::string> parts;
	parts.push_back(makePart("bigfile", "large.bin", largeContent));
	std::string body = buildBody(boundary, parts);

	std::vector<MultipartPart> result = parser.parse(ct, body);

	ASSERT_EQ(1u, result.size());
	ASSERT_EQ(std::string("large.bin"), result[0].filename);
	ASSERT_EQ(largeContent.size(), result[0].content.size());
}

// ---------------------------------------------------------------------------
// parse – mixed fields and files
// ---------------------------------------------------------------------------

static void testParseMixedFieldsAndFiles()
{
	MultipartParser parser;

	std::string boundary = "mixbound";
	std::string ct = "multipart/form-data; boundary=" + boundary;

	std::string field1 =
		"Content-Disposition: form-data; name=\"title\"\r\n"
		"\r\n"
		"My Upload";
	std::string field2 =
		"Content-Disposition: form-data; name=\"tags\"\r\n"
		"\r\n"
		"c++ webserv";

	std::vector<std::string> parts;
	parts.push_back(field1);
	parts.push_back(makePart("avatar", "photo.jpg", "\xFF\xD8\xFF")); // JPEG magic bytes
	parts.push_back(field2);
	parts.push_back(makePart("resume", "cv.pdf", "%PDF-"));
	std::string body = buildBody(boundary, parts);

	std::vector<MultipartPart> result = parser.parse(ct, body);

	// Only parts with filename should be returned
	ASSERT_EQ(2u, result.size());
	ASSERT_EQ(std::string("photo.jpg"), result[0].filename);
	ASSERT_EQ(std::string("cv.pdf"),    result[1].filename);
}

// ---------------------------------------------------------------------------
// parse – only the closing boundary (no parts)
// ---------------------------------------------------------------------------

static void testParseOnlyClosingBoundary()
{
	MultipartParser parser;

	std::string boundary = "solo";
	std::string ct = "multipart/form-data; boundary=" + boundary;
	std::string body = "--" + boundary + "--\r\n";

	std::vector<MultipartPart> result = parser.parse(ct, body);
	ASSERT_EQ(0u, result.size());
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

void testMultipartParser()
{
	std::cout << "\n=== MultipartParser Tests ===" << std::endl;

	RUN_TEST(testIsMultipartTrue);
	RUN_TEST(testIsMultipartFalse);
	RUN_TEST(testParseSingleFile);
	RUN_TEST(testParseMultipleFiles);
	RUN_TEST(testParseEmptyBody);
	RUN_TEST(testParseMissingBoundaryInContentType);
	RUN_TEST(testParseWrongContentType);
	RUN_TEST(testParseFieldWithoutFilenameIgnored);
	RUN_TEST(testParseBinaryContent);
	RUN_TEST(testParseQuotedBoundary);
	RUN_TEST(testParseFilenameWithSpaces);
	RUN_TEST(testParseBodyWithNoDelimiter);
	RUN_TEST(testParseLargeContent);
	RUN_TEST(testParseMixedFieldsAndFiles);
	RUN_TEST(testParseOnlyClosingBoundary);
}
