#include "TestUtils.hpp"

void	testRequestParser();
void	testRouter();
void	testConfigParser();

int	main()
{
	testRequestParser();
	testRouter();
	testConfigParser();

	std::cout << "\nPassed: " << g_testsPassed << std::endl;
	std::cout << "Failed: " << g_testsFailed << std::endl;

	return (g_testsFailed == 0 ? 0 : 1);
}
