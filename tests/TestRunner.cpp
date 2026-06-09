#include "TestUtils.hpp"

void	testRequestParser();
void	testRouter();
void	testConfigParser();
void	testResponseBuilder();

int	main()
{
	testRequestParser();
	testRouter();
	testConfigParser();
	testResponseBuilder();

	std::cout << "\nPassed: " << g_testsPassed << std::endl;
	std::cout << "Failed: " << g_testsFailed << std::endl;

	return (g_testsFailed == 0 ? 0 : 1);
}
