#include "BitObject.h"

BitObject::BitObject()
{

}

BitObject::BitObject(char const* pfilename, int pwidth, int pheight, int px, int py)
{
	filename = pfilename;

	x = px;
	y = py;

	width = pwidth;
	height = pheight;
}


//BitObject::BitObject() 
//{
//	al_init();
//	al_init_image_addon();
//
//	bitmap = nullptr;
//}
//
//BitObject::BitObject(char const* filename, int pwidth, int pheight, int px, int py)
//{
//	al_init();
//	al_init_image_addon();
//
//	bitmap = al_load_bitmap(filename);
//
//	x = px;
//	y = py;	
//
//	width = pwidth;
//	height = pheight;
//	//	width = al_get_bitmap_width(pbitmap); //memory access violation for some reason. Could not figure out why. MFonts works just fine...
//	//	height = al_get_bitmap_height(pbitmap);
//}
 

//BitObject::BitObject(ALLEGRO_BITMAP* pbitmap, int pwidth, int pheight)
//{
//	al_init();
//	al_init_image_addon();
//
//	bitmap = pbitmap;
//
//	x = 0;
//	y = 0;
//
//	width = pwidth;
//	height = pheight;
//	//width = 140;
//	//height = 280;
//}

//BitObject::BitObject(ALLEGRO_BITMAP* pbitmap, int px, int py)
//{
//	al_init();
//	al_init_image_addon();
//
//	bitmap = pbitmap;
//
//	x = px;
//	y = py;
//

//	//width = 140;
//	//height = 280;
//}

BitObject::~BitObject()
{
	//al_destroy_bitmap(bitmap);
}