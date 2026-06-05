#include "TestUtils.hpp"

void	testRequestParser();

int	main()
{
	testRequestParser();

	std::cout << "\nPassed: " << g_testsPassed << std::endl;
	std::cout << "Failed: " << g_testsFailed << std::endl;

	return (g_testsFailed == 0 ? 0 : 1);
}
