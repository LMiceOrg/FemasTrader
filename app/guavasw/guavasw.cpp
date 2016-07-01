#include "guava_demo.h"

int main()
{
	printf("---%d---\n", __LINE__);

	guava_demo temp("sw-guavamd");
	printf("---%d---\n", __LINE__);
	temp.run();
	printf("---%d---\n", __LINE__);

	return 0;
}









