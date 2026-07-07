#include "TestUtils.hpp"

void	testRequestParser();
void	testRouter();
void	testConfigParser();
void	testResponseBuilder();
void	testMultipartParser();
void	testSessionManager();
void	testMimeTypes();
void	testResponse();
void	testGaps();

int	main()
{
	testRequestParser();
	testRouter();
	testConfigParser();
	testResponseBuilder();
	testMultipartParser();
	testSessionManager();
	testMimeTypes();
	testResponse();
	testGaps();

	std::cout << "\nPassed: " << g_testsPassed << std::endl;
	std::cout << "Failed: " << g_testsFailed << std::endl;

	return (g_testsFailed == 0 ? 0 : 1);
}
