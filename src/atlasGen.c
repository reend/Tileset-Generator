#include <dirent.h>
#include <math.h>
#include <png.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "structs.h"

static Image *images;
static char	*rootDir;

static int countImages(const char *dir)
{
	DIR		   *d;
	struct dirent *ent;
	char		  *path;
	int			   i;

	i = 0;

	if ((d = opendir(dir)) != NULL)
	{
		while ((ent = readdir(d)) != NULL)
		{
			if (ent->d_type == DT_DIR)
			{
				if (ent->d_name[0] != '.')
				{
					path = malloc(strlen(dir) + strlen(ent->d_name) + 2);
					sprintf(path, "%s/%s", dir, ent->d_name);
					i += countImages(path);
					free(path);
				}
			}
			else
			{
				i++;
			}
		}

		closedir(d);
	}

	return i;
}

static void loadImageData(int *i, const char *dir)
{
	DIR		   *d;
	struct dirent *ent;
	char		  *path;

	if ((d = opendir(dir)) != NULL)
	{
		while ((ent = readdir(d)) != NULL)
		{
			path = malloc(strlen(dir) + strlen(ent->d_name) + 2);

			if (ent->d_type == DT_DIR)
			{
				if (ent->d_name[0] != '.')
				{
					sprintf(path, "%s/%s", dir, ent->d_name);

					loadImageData(i, path);
				}
			}
			else
			{
				sprintf(path, "%s/%s", dir, ent->d_name);

				images[*i].surface = IMG_Load(path);

				if (images[*i].surface)
				{
					images[*i].filename = malloc(strlen(path) + 1);

					strcpy(images[*i].filename, path);
					SDL_SetSurfaceBlendMode(images[*i].surface, SDL_BLENDMODE_NONE);
					*i = *i + 1;
				}
			}

			free(path);
		}

		closedir(d);
	}
}

static int imageComparator(const void *a, const void *b)
{
	Image *i1 = (Image *)a;
	Image *i2 = (Image *)b;

	return i2->surface->h - i1->surface->h;
}

static void handleCommandLine(int argc, char *argv[])
{
	int i;

	/* defaults */
	rootDir = "gfx";

	for (i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-dir") == 0)
		{
			rootDir = argv[i + 1];
		}
	}
}

static int initImages(void)
{
	int i;

	i = countImages(rootDir);

	images = malloc(sizeof(Image) * i);

	memset(images, 0, sizeof(Image) * i);

	i = 0;

	loadImageData(&i, rootDir);

	qsort(images, i, sizeof(Image), imageComparator);

	return i;
}

int main(int argc, char *argv[])
{
	int i, numImages;

	handleCommandLine(argc, argv);

	SDL_Init(SDL_INIT_VIDEO);

	numImages = initImages();

	for (i = 0; i < numImages; i++)
	{
		printf("[%02d / %02d] %s\n", i + 1, numImages, images[i].filename);

		SDL_FreeSurface(images[i].surface);
		free(images[i].filename);
	}

	free(images);

	SDL_Quit();
}
