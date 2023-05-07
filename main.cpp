#include "DiskDriver.h"
#include "TestAPI.h"

int main()
{
	DiskDriver dd;
	dd.Initial();
	TestAPI ta;
	ta.Menu();
}