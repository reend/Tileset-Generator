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
static int	  atlasSize;
static int	  padding;
static char	*rootDir;

static void splitNode(Node *node, int w, int h)
{
	node->used = 1;

	node->child = malloc(sizeof(Node) * 2);

	memset(node->child, 0, sizeof(Node) * 2);

	node->child[0].x = node->x + w + padding;
	node->child[0].y = node->y;
	node->child[0].w = node->w - w - padding;
	node->child[0].h = h;

	node->child[1].x = node->x;
	node->child[1].y = node->y + h + padding;
	node->child[1].w = node->w;
	node->child[1].h = node->h - h - padding;
}

static Node *findNode(Node *root, int w, int h)
{
	if (root->used)
	{
		Node *n = NULL;

		if ((n = findNode(&root->child[0], w, h)) != NULL || (n = findNode(&root->child[1], w, h)) != NULL)
		{
			return n;
		}
	}
	else if (w <= root->w && h <= root->h)
	{
		splitNode(root, w, h);

		return root;
	}

	return NULL;
}

static void freeNode(Node *node)
{
	if (node->used)
	{
		freeNode(&node->child[0]);

		freeNode(&node->child[1]);

		free(node->child);
	}
}

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
	atlasSize = 256;
	rootDir = "gfx";
	padding = 1;

	for (i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-size") == 0)
		{
			atlasSize = atoi(argv[i + 1]);
		}
		else if (strcmp(argv[i], "-dir") == 0)
		{
			rootDir = argv[i + 1];
		}
		else if (strcmp(argv[i], "-padding") == 0)
		{
			padding = atoi(argv[i + 1]);
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

static void putPixel(int x, int y, Uint32 pixel, SDL_Surface *dest)
{
	int	   bpp;
	Uint8 *p;

	bpp = dest->format->BytesPerPixel;
	p = (Uint8 *)dest->pixels + y * dest->pitch + x * bpp;

	switch (bpp)
	{
		case 1:
			*p = pixel;
			break;

		case 2:
			*(Uint16 *)p = pixel;
			break;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			{
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			}
			else
			{
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;

		case 4:
			*(Uint32 *)p = pixel;
			break;
	}
}

static int getPixel(SDL_Surface *surface, int x, int y)
{
	int	   bpp;
	Uint8 *p;

	bpp = surface->format->BytesPerPixel;
	p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch (bpp)
	{
		case 1:
			return *p;

		case 2:
			return *(Uint16 *)p;

		case 3:
			if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			{
				return p[0] << 16 | p[1] << 8 | p[2];
			}
			else
			{
				return p[0] | p[1] << 8 | p[2] << 16;
			}

		case 4:
			return *(Uint32 *)p;

		default:
			return 0;
	}
}

static void blitRotated(SDL_Surface *src, SDL_Surface *dest, int destX, int destY)
{
	int x, y, p, dx, dy;

	dy = 0;

	for (x = 0; x < src->w; x++)
	{
		dx = src->h - 1;

		for (y = 0; y < src->h; y++)
		{
			p = getPixel(src, x, y);

			putPixel(destX + dx, destY + dy, p, dest);

			dx--;
		}

		dy++;
	}
}

int main(int argc, char *argv[])
{
	Node		 *root, *n;
	int			 w, h, rotated, i, fails, rotations, numImages;
	SDL_Surface *atlas;
	SDL_Rect	 dest;

	handleCommandLine(argc, argv);

	SDL_Init(SDL_INIT_VIDEO);

	root = malloc(sizeof(Node));

	root->x = 0;
	root->y = 0;
	root->w = atlasSize;
	root->h = atlasSize;
	root->used = 0;

	fails = rotations = 0;

	numImages = initImages();

	atlas = SDL_CreateRGBSurface(0, atlasSize, atlasSize, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

	for (i = 0; i < numImages; i++)
	{
		rotated = 0;

		w = images[i].surface->w;
		h = images[i].surface->h;

		n = findNode(root, w, h);

		if (n == NULL)
		{
			rotated = 1;

			n = findNode(root, h, w);
		}

		if (n != NULL)
		{
			if (rotated)
			{
				n->h = w;
				n->w = h;
				rotations++;
			}

			dest.x = n->x;
			dest.y = n->y;
			dest.w = n->w;
			dest.h = n->h;

			if (!rotated)
			{
				SDL_BlitSurface(images[i].surface, NULL, atlas, &dest);
			}
			else
			{
				blitRotated(images[i].surface, atlas, dest.x, dest.y);
			}

			printf("[%04d / %04d] %s\n", i + 1, numImages, images[i].filename);
		}
		else
		{
			printf("[ERROR] Couldn't add '%s'\n", images[i].filename);

			fails++;
		}

		SDL_FreeSurface(images[i].surface);
		free(images[i].filename);
	}

	free(images);

	IMG_SavePNG(atlas, "atlas.png");

	SDL_FreeSurface(atlas);

	freeNode(root);

	free(root);

	printf("Images: %d, Failures: %d, Rotations: %d\n", numImages, fails, rotations);

	SDL_Quit();
}
