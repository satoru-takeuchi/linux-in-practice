#include <sys/types.h>
#include <unistd.h>

int main(void)
{
	for (;;)
		getppid();
}
