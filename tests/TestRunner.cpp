#include "TestUtils.hpp"

void	testRequestParser();
void	testRouter();

int	main()
{
	testRequestParser();
	testRouter();

	std::cout << "\nPassed: " << g_testsPassed << std::endl;
	std::cout << "Failed: " << g_testsFailed << std::endl;

	return (g_testsFailed == 0 ? 0 : 1);
}
