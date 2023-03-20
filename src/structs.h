typedef struct Node Node;

struct Node
{
	int x;
	int y;
	int w;
	int h;
	int used;

	struct Node *child;
};

typedef struct
{
	char		 *filename;
	SDL_Surface *surface;
} Image;
