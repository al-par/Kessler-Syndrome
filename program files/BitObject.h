#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>


class BitObject
{
public:
	//BitObject(ALLEGRO_BITMAP* pbitmap);
	//BitObject(ALLEGRO_BITMAP* pbitmap, int px, int py);
	BitObject();
	BitObject(char const* filename, int pwidth, int pheight, int px, int py);
	~BitObject();

	char const* filename;

	int x;
	int y;
	int width;
	int height;
};