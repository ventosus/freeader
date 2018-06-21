#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef __unix__
#	include <dirent.h>
#	include <unistd.h>
#endif

#define MAX_PATH_LEN 1024

static int
_iterate(const char *dir)
{
	char buffer[MAX_PATH_LEN];

	strncpy(buffer, dir, MAX_PATH_LEN);
	const size_t n = strlen(buffer);

	DIR *z = opendir(dir);
	if(!z)
	{
		return EXIT_FAILURE;
	}

	for(struct dirent *data = readdir(z); data; data = readdir(z) )
	{
		if(  (data->d_name[0] == '.')
			&& ( ( (data->d_name[1] == '\0') || (data->d_name[1] == '.'))) )
		{
			continue;
		}

#if 0
		char *point = strrchr(data->d_name, '.');
		if(!point) // no suffix
		{
			continue;
		}

		if(strcmp(point, ".pig")) // no *.pig suffix
		{
			continue;
		}
#endif

		strncpy(buffer + n, data->d_name, MAX_PATH_LEN-n);

		fprintf(stdout, "%s\n", buffer);

		DIR *y = opendir(buffer);
		const bool is_subdir = (y != NULL);
		if (y != NULL)
		{
			closedir(y);
		}

		if(is_subdir)
		{
			_iterate(buffer);
		}
	}

	closedir(z);


	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	return _iterate(argv[1]);
}
