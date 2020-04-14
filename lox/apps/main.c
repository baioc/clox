#include <sgl/list.h>

int main(int argc, const char* argv[])
{
	list_t list;
	list_init_sized(&list, sizeof(char*), 0);
	return list_size(&list);
}
