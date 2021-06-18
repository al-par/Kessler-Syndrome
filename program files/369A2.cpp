#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <math.h>
#include <regex>
#include <random>
#include <vector>

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>  
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>

#include "MFonts.h"
#include "BitObject.h"

#define DIFFICULTY 3 //1 for easy, 2 for medium, 3 for hard (modifies which asteroids appear. On easy difficulty, only the small ones appear. On hard, the small, medium, and big ones appear.)

#define DEBUG_MODE false //set this to true to display hitboxes
#define ROTATION_MODE rand() % 4 //can be 0, 1, 2, 3 or a rand() % of one of those numbers //Sets the rotation of asteroids every update cycle

#define DISPLAY_WIDTH 800 //This is also used instead of al_get_display_width to save on processing time (I know these savings are minuscule, but whatever)
#define DISPLAY_HEIGHT 1000
#define FRAMERATE 60

#define TOP_EDGE_OFFSET 30
#define BOTTOM_EDGE_OFFSET 30
#define LEFT_EDGE_OFFSET 300
#define RIGHT_EDGE_OFFSET 30
#define TEXT_EDGE_OFFSET 30
#define HUD_ELEMENT_OFFSET 30
#define HUD_EDGE_OFFSET 30
#define HUD_ELEMENT_HEIGHT 150
#define HUD_TEXT_OFFSET 10
#define HUD_DYNAMIC_TEXT_OFFSET 70

#define SHIP_SCALING_FACTOR 2

#define GAME_VELOCITY 10

#define STARTING_HEALTH_POINTS 10
#define STARTING_ALTITUDE 100


using namespace std;


//functions to display the static elements of each page
void displayWelcomePage(const MFonts* fonts, ALLEGRO_DISPLAY* display);
void displayInstructionsPage(const MFonts* fonts, ALLEGRO_DISPLAY* display);
void displayGamePage(const MFonts* fonts, ALLEGRO_DISPLAY* display, int turnsPassed, int health);
void displayResultPage(const MFonts* fonts, ALLEGRO_DISPLAY* display, int turnsPassed);

//utility functions
bool collision(BitObject asteroid, BitObject spaceship);
void draw_spaceship_hitbox(BitObject spaceship);
void draw_asteroid_hitbox(BitObject asteroid);


int main()
{
	srand(time(NULL));
	int returnCode = 0;

	//___________________________________________________________________________________________________________________________________________________________________
	//initialize allegro 5
	MFonts fonts("orbitron-light.ttf");

	ALLEGRO_DISPLAY* display;
	ALLEGRO_EVENT_QUEUE* queue;
	ALLEGRO_TIMER* timer;

	ALLEGRO_SAMPLE* sampleMainTheme = NULL;
	ALLEGRO_SAMPLE* sampleSuccess = NULL;
	ALLEGRO_SAMPLE* sampleFailure = NULL;

	al_init();
	al_init_font_addon();
	al_init_ttf_addon();
	al_install_keyboard();
	al_install_mouse();
	al_install_audio();
	al_init_acodec_addon();
	al_init_primitives_addon();
	al_init_image_addon();

	display = al_create_display(DISPLAY_WIDTH, DISPLAY_HEIGHT);
	queue = al_create_event_queue();
	timer = al_create_timer(1.0 / FRAMERATE); // 60 fps timer

	al_reserve_samples(3);
	sampleMainTheme = al_load_sample("Main Theme.wav");
	sampleSuccess = al_load_sample("Success.wav"); //not used
	sampleFailure = al_load_sample("Failure.wav");

	BitObject spaceship("apogee.png", 140, 280, 0, 0);
	ALLEGRO_BITMAP* spaceshipBitmap = al_load_bitmap(spaceship.filename);
	vector<BitObject> asteroidTypes;
	vector<ALLEGRO_BITMAP*> asteroidTypeBitmaps;

	//asteroidTypes.emplace_back(BitObject("asteroid_big00.png", 472, 378, 0, 0)); //too big. Would need to be scaled down to be included
	//asteroidTypes.emplace_back(BitObject("asteroid_big02.png", 370, 266, 0, 0)); //too big. Would need to be scaled down to be included
	//asteroidTypes.emplace_back(BitObject("asteroid_big03.png", 210, 174, 0, 0)); //too big. Would need to be scaled down to be included
	//asteroidTypes.emplace_back(BitObject("asteroid4.png", 88, 83, 0, 0)); //too big. Would need to be scaled down to be included
	if (DIFFICULTY >= 1) asteroidTypes.emplace_back(BitObject("asteroid1.png", 23, 20, 0, 0));
	if (DIFFICULTY >= 2) asteroidTypes.emplace_back(BitObject("asteroid2.png", 38, 37, 0, 0));
	if (DIFFICULTY >= 3) asteroidTypes.emplace_back(BitObject("asteroid3.png", 53, 53, 0, 0));

	for (int i = 0; i < asteroidTypes.size(); i++)
	{
		asteroidTypeBitmaps.emplace_back(al_load_bitmap(asteroidTypes[i].filename));
	}

	al_register_event_source(queue, al_get_keyboard_event_source());
	al_register_event_source(queue, al_get_display_event_source(display));
	al_register_event_source(queue, al_get_mouse_event_source());
	al_register_event_source(queue, al_get_timer_event_source(timer));

	al_start_timer(timer);

	//___________________________________________________________________________________________________________________________________________________________________
	//game update loop variable declarations

	//update loop boolean
	bool running = true;

	//audio
	bool soundsMute = false;
	bool mainThemePlaying = false;
	vector<ALLEGRO_SAMPLE_INSTANCE*>* sampleInstances = new vector< ALLEGRO_SAMPLE_INSTANCE*>;
	bool collisionHappened = false; //updated if there is a collision so that the appropriate sound can be played in the audio managment section

	//keyboard
	bool pressed_keys[ALLEGRO_KEY_MAX];
	for (int i = 0; i < ALLEGRO_KEY_MAX; i++) pressed_keys[i] = false; //set all key presses to false by default

	//pages
	enum pages { Welcome, Instructions, Game, Result };
	pages page = Welcome;

	//spaceship
	spaceship.x = ((LEFT_EDGE_OFFSET + DISPLAY_WIDTH - RIGHT_EDGE_OFFSET) / 2) - (spaceship.width / (2 * SHIP_SCALING_FACTOR)); //initial x pos
	spaceship.y = DISPLAY_HEIGHT - BOTTOM_EDGE_OFFSET - (spaceship.height / SHIP_SCALING_FACTOR) - 5; //initial y pos

	//asteroids to display
	vector<BitObject>* asteroids = new vector<BitObject>;
	vector<ALLEGRO_BITMAP*>* asteroidBitmaps = new vector<ALLEGRO_BITMAP*>;

	//turn tracking
	int turnsPassed = 0;
	bool turnPassed = false; //Only move the ships and asteroids if the player moved the ship. This bool tracks if the player moved the ship.

	//displayable info
	int health = STARTING_HEALTH_POINTS;
	int altitude = STARTING_ALTITUDE;

	//___________________________________________________________________________________________________________________________________________________________________
	//game update loop
	while (running)
	{
		//---------------------------------------------------------------------------------------------------------------------------------------------------------------
		//Audio managment

		if (!soundsMute)
		{
			if (!mainThemePlaying)
			{
				al_play_sample(sampleMainTheme, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL);
				mainThemePlaying = true;
			}
			if (collisionHappened)
			{
				sampleInstances->emplace_back(al_create_sample_instance(sampleFailure));
				al_attach_sample_instance_to_mixer(sampleInstances->back(), al_get_default_mixer());
				al_play_sample_instance(sampleInstances->back());
				collisionHappened = false;
			}
		}
		else
		{
			al_stop_samples();
			mainThemePlaying = false;
		}

		//clean up audio sample instances that are done playing
		for (int i = 0; i < sampleInstances->size(); i++)
		{
			if (!al_get_sample_instance_playing(sampleInstances->at(i))) //sample done playing
			{
				al_destroy_sample_instance(sampleInstances->at(i));
				sampleInstances->erase(sampleInstances->begin() + i);
			}
		}


		//---------------------------------------------------------------------------------------------------------------------------------------------------------------
		//Event handling
		ALLEGRO_EVENT event;
		bool waiting = true;
		while (waiting) //Make update loop wait for an event. Used to cap FPS or exit the program.
		{
			al_wait_for_event(queue, &event); 
			if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) /*Terminate Program*/
			{
				running = false;
				waiting = false;
				returnCode -= 1; // changing the return code like this instead of setting it to a new value allows the program to return with multiple return codes.
				//yes, I know logging would be 'better', but this isnt a very complex program, and there aren't many return codes.
			}
			//if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) //for some reason, this clause is occasionally being entered without the user pressing the ESCAPE key.
			//{
			//	running = false;
			//	waiting = false;
			//	returnCode -= 10;
			//}
			if (event.type == ALLEGRO_EVENT_TIMER)
			{
				al_flip_display();
				waiting = false;
			}
		}

		//---------------------------------------------------------------------------------------------------------------------------------------------------------------
		//Keyboard managment
		//A loop could manage all of these in fewer lines, but it would be less efficient, considering we don't need most key presses.
		//Key presses are managed by active polling instead of events because it was easier to code toggles for them. 
		//Also, events were being incredibly buggy for some reason. Some keyboard events were being fired despite no key presses.

		ALLEGRO_KEYBOARD_STATE keyState;
		al_get_keyboard_state(&keyState);

		//Key ESCAPE
		if (al_key_down(&keyState, ALLEGRO_KEY_ESCAPE)) pressed_keys[ALLEGRO_KEY_ESCAPE] = true;
		else if (pressed_keys[ALLEGRO_KEY_ESCAPE] == true) /*Terminate Program*/
		{
			running = false;
			waiting = false;
			returnCode -= 10;
		}

		//Key Left
		if (al_key_down(&keyState, ALLEGRO_KEY_LEFT)) pressed_keys[ALLEGRO_KEY_LEFT] = true;
		else if (pressed_keys[ALLEGRO_KEY_LEFT] == true) /*move left*/
		{ 
			pressed_keys[ALLEGRO_KEY_LEFT] = false;
			if (spaceship.x > LEFT_EDGE_OFFSET && page == Game) //spaceship.width / 2
			{
				spaceship.x -= GAME_VELOCITY;
				turnsPassed++;
				turnPassed = true;
			}
			//cout << spaceship.x << endl; //uncomment for debug purposes
		}

		//Key Right
		if (al_key_down(&keyState, ALLEGRO_KEY_RIGHT)) pressed_keys[ALLEGRO_KEY_RIGHT] = true;
		else if (pressed_keys[ALLEGRO_KEY_RIGHT] == true) /*move right*/
		{ 
			pressed_keys[ALLEGRO_KEY_RIGHT] = false;
			if (spaceship.x < (DISPLAY_WIDTH - RIGHT_EDGE_OFFSET - spaceship.width / SHIP_SCALING_FACTOR) - GAME_VELOCITY && page == Game) //the -GAME_VELOCITY is just to account for the border of the playable area
			{
				spaceship.x += GAME_VELOCITY;
				turnsPassed++;
				turnPassed = true;
			}
			//cout << spaceship.x << endl; //uncomment for debug purposes
		}
		
		//Key Up
		if (al_key_down(&keyState, ALLEGRO_KEY_UP)) pressed_keys[ALLEGRO_KEY_UP] = true;
		else if (pressed_keys[ALLEGRO_KEY_UP] == true) /*don't move*/
		{
			pressed_keys[ALLEGRO_KEY_UP] = false;
			if (page == Game)
			{
				turnsPassed++;
				turnPassed = true;
			}
		}

		//Key Down
		if (al_key_down(&keyState, ALLEGRO_KEY_DOWN)) pressed_keys[ALLEGRO_KEY_DOWN] = true;
		else if (pressed_keys[ALLEGRO_KEY_DOWN] == true) /*don't move*/
		{
			pressed_keys[ALLEGRO_KEY_DOWN] = false;
			if (page == Game)
			{
				turnsPassed++;
				turnPassed = true;
			}
		}

		//Key LCTRL
		if (al_key_down(&keyState, ALLEGRO_KEY_LCTRL))
		{
			//Key LCTRL + h
			if (al_key_down(&keyState, ALLEGRO_KEY_H)) pressed_keys[ALLEGRO_KEY_H] = true;
			else if (pressed_keys[ALLEGRO_KEY_H] == true) { pressed_keys[ALLEGRO_KEY_H] = false; page = Instructions;} /*open help menu*/

			//Key LCTRL + m
			if (al_key_down(&keyState, ALLEGRO_KEY_M)) pressed_keys[ALLEGRO_KEY_M] = true;
			else if (pressed_keys[ALLEGRO_KEY_M] == true) 
			{ 
				pressed_keys[ALLEGRO_KEY_M] = false;
				if (soundsMute) //unmute
				{
					soundsMute = false;
				}
				else if(!soundsMute) //mute
				{
					soundsMute = true;
				}
			} /*mute sound*/

			turnPassed = false;
		}

		//Key ENTER
		if (al_key_down(&keyState, ALLEGRO_KEY_ENTER)) pressed_keys[ALLEGRO_KEY_ENTER] = true;
		else if (pressed_keys[ALLEGRO_KEY_ENTER] == true) //proceed upon release so as to not skip through all the menus
		{ 
			pressed_keys[ALLEGRO_KEY_ENTER] = false;
			if (page == Welcome) page = Instructions;
			else if (page == Instructions) page = Game;

			turnPassed = false;
		}

		//---------------------------------------------------------------------------------------------------------------------------------------------------------------
		//update and display page

		switch (page)
		{
		case Welcome:
			displayWelcomePage(&fonts, display);
			break; //end of case Welcome---------------------------------------------------------------------------------------------------------------------------------
		case Instructions:
			displayInstructionsPage(&fonts, display);
			break; //end of case Instructions----------------------------------------------------------------------------------------------------------------------------
		case Game:
			displayGamePage(&fonts, display, turnsPassed, health);
			al_draw_scaled_bitmap(spaceshipBitmap, 0, 0, spaceship.width, spaceship.height, spaceship.x, spaceship.y, 
				(spaceship.width / SHIP_SCALING_FACTOR), (spaceship.height / SHIP_SCALING_FACTOR), 0);
			if (turnPassed)
			{
				//move asteroids down and check for collisions
				for (int i = 0; i < asteroids->size(); i++)
				{
					asteroids->at(i).y += GAME_VELOCITY;
					if ((asteroids->at(i).y + asteroids->at(i).height) >= (DISPLAY_HEIGHT - BOTTOM_EDGE_OFFSET)) //The asteroid has hit the bottom of the playable area
					{
						//destroy asteroid
						al_destroy_bitmap(asteroidBitmaps->at(i));
						asteroidBitmaps->erase(asteroidBitmaps->begin() + i);
						asteroids->erase(asteroids->begin() + i);
					}

					//check for collisions
					if (collision(asteroids->at(i), spaceship))
					{
						health--;
						//destroy asteroid that was subject to collision
						al_destroy_bitmap(asteroidBitmaps->at(i));
						asteroidBitmaps->erase(asteroidBitmaps->begin() + i);
						asteroids->erase(asteroids->begin() + i);
						//cout << health << endl; //uncomment for debug purposes
						collisionHappened = true; //for audio purposes
					}
				}
				//generate new asteroids
				for (int i = 0; i < (rand() % 2); i++) //(rand() % 2) gives a 50% chance of generating an asteroid per turn.
				{
					int j = rand() % asteroidTypes.size();
					//cout << j << endl;

					asteroids->emplace_back(BitObject(asteroidTypes[j].filename, asteroidTypes[j].width, asteroidTypes[j].height, 
						(rand() % (DISPLAY_WIDTH - LEFT_EDGE_OFFSET - RIGHT_EDGE_OFFSET - asteroidTypes[j].width)) + LEFT_EDGE_OFFSET, //random x position within play area
						TOP_EDGE_OFFSET));
					asteroidBitmaps->emplace_back(al_clone_bitmap(asteroidTypeBitmaps[j]));
					//cout << i << endl;
				}
			}

			//display asteroids
			for (int i = 0; i < asteroids->size(); i++)
			{
				al_draw_bitmap(asteroidBitmaps->at(i), asteroids->at(i).x, asteroids->at(i).y, ROTATION_MODE);

				if (DEBUG_MODE)
				{
					//display asteroid hitbox
					draw_asteroid_hitbox(asteroids->at(i));
				}
			}

			if (DEBUG_MODE)
			{
				//display spaceship hitbox
				draw_spaceship_hitbox(spaceship);
			}

			turnPassed = false;
			if (health <= 0) page = Result;

			break; //end of case Game------------------------------------------------------------------------------------------------------------------------------------
		case Result:
			displayResultPage(&fonts, display, turnsPassed);
			break; //end of case Result----------------------------------------------------------------------------------------------------------------------------------
		default:
			returnCode -= 100; //the program should never enter this case
			break; //end of case Default---------------------------------------------------------------------------------------------------------------------------------
		}
	}

	//___________________________________________________________________________________________________________________________________________________________________
	//destructors and cleanup

	al_destroy_display(display);
	al_uninstall_keyboard();
	al_uninstall_mouse();
	al_destroy_timer(timer);

	//Audio
	al_destroy_sample(sampleSuccess);
	al_destroy_sample(sampleFailure);
	al_destroy_sample(sampleMainTheme);

	for (int i = 0; i < sampleInstances->size(); i++)
	{
		al_destroy_sample_instance(sampleInstances->at(i));
	}
	sampleInstances->clear();
	delete sampleInstances;
	al_uninstall_audio();

	//Bitmaps and BitObjects
	al_destroy_bitmap(spaceshipBitmap);
	for (int i = 0; i < asteroidBitmaps->size(); i++)
	{
		al_destroy_bitmap(asteroidBitmaps->at(i));
	}
	asteroids->clear();
	delete asteroids;
	asteroidBitmaps->clear();
	delete asteroidBitmaps;

	//end of program
	return returnCode;
}

void displayWelcomePage(const MFonts* fonts, ALLEGRO_DISPLAY* display)
{
	al_clear_to_color(al_map_rgb(255, 255, 255));
	al_draw_filled_rectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, al_map_rgba_f(0, 0, 0, 0.7));

	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 30, ALLEGRO_ALIGN_CENTRE, "The Year is 2238");

	al_draw_multiline_text(fonts->body, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), DISPLAY_WIDTH / 2, 100, DISPLAY_WIDTH - TEXT_EDGE_OFFSET, 30, ALLEGRO_ALIGN_CENTRE,
		"Careless disregard by our species has polluted the earth's orbit with spacecraft debris, and collisions between them has created a feedback cycle known as ");

	al_draw_multiline_text(fonts->title, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 220, DISPLAY_WIDTH - TEXT_EDGE_OFFSET, 80, ALLEGRO_ALIGN_CENTRE, 
		"KESSLER SYNDROME");

	al_draw_multiline_text(fonts->body, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), DISPLAY_WIDTH / 2, 400, DISPLAY_WIDTH - TEXT_EDGE_OFFSET, 30, ALLEGRO_ALIGN_CENTRE,
		"Debris from these perpetual collisions have rained down from the sky for the past decade, devastating nearly all life on earth. As a last-ditch effort,\
		 humanity created a Generation Ship, codenamed \'Project Jericho\'. As the commander of the Jericho, you are tasked with safely guiding your ship, and the rest\
		 of humanity, out of  Earth\'s orbit, and to find a new planet to call home.");

	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 700, ALLEGRO_ALIGN_CENTRE, "Godspeed, commander");

	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 900, ALLEGRO_ALIGN_CENTRE, "Press ENTER to continue");
}

void displayInstructionsPage(const MFonts* fonts, ALLEGRO_DISPLAY* display)
{
	al_clear_to_color(al_map_rgb(255, 255, 255));
	al_draw_filled_rectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, al_map_rgba_f(0, 0, 0, 0.7));

	al_draw_text(fonts->title, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 30, ALLEGRO_ALIGN_CENTRE, "Instructions");

	//how to play
	al_draw_multiline_text(fonts->body, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), DISPLAY_WIDTH / 2, 150, DISPLAY_WIDTH - TEXT_EDGE_OFFSET, 30, ALLEGRO_ALIGN_CENTRE,
		"Move your ship left or right to dodge debris in your path. Each debris collision lowers your ship\'s health. You may find yourself in situations where \
debris is impossible to dodge, so be mindful of your health. ");

	//controls
	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), TEXT_EDGE_OFFSET, 300, ALLEGRO_ALIGN_LEFT, "Controls:");
	al_draw_multiline_text(fonts->body, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), TEXT_EDGE_OFFSET, 370, DISPLAY_WIDTH - TEXT_EDGE_OFFSET, 30, ALLEGRO_ALIGN_LEFT,
		"Left Arrow: Move left\n\
		Right Arrow: Move right\n\
		Up Arrow: Stay in current position\n\
		Down Arrow: Stay in current position\n\
		LCTRL + h: Open help menu\n\
		LCTRL + m: Toggle mute\n\
		ESC : Close game");

	//what they can expect at the end of the game
	al_draw_multiline_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), DISPLAY_WIDTH / 2, 600, DISPLAY_WIDTH - TEXT_EDGE_OFFSET, 50, ALLEGRO_ALIGN_CENTRE,
		"Kessler Syndrome is an endless struggle. Survive for as long as you can!");

	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 900, ALLEGRO_ALIGN_CENTRE, "Press ENTER to continue");
}

void displayGamePage(const MFonts* fonts, ALLEGRO_DISPLAY* display, int turnsPassed, int health)
{
	int altitude = STARTING_ALTITUDE;
	for (int i = turnsPassed; i > 0; i--)
	{
		if (i > 2808) altitude += 5000;
		else if (i > 880) altitude += 500;
		else if (i > 200) altitude += 50;
		else if (i > 20) altitude += 10;
		else altitude += 5;
	}
	string trajectory;
	if (altitude > 1000000) trajectory = "Escape";
	else if (altitude > 36000) trajectory = "High Earth orbit";
	else if (altitude > 2000) trajectory = "Medium Earth orbit";
	else if (altitude > 200) trajectory = "Low Earth orbit";
	else trajectory = "Sub-orbital";

	//draw playable area
	al_clear_to_color(al_map_rgb_f(0.1, 0.075, 0.275));
	al_draw_rectangle(LEFT_EDGE_OFFSET, TOP_EDGE_OFFSET, DISPLAY_WIDTH - RIGHT_EDGE_OFFSET, DISPLAY_HEIGHT - BOTTOM_EDGE_OFFSET, al_map_rgb(0, 0, 0), 5);
	al_draw_filled_rectangle(LEFT_EDGE_OFFSET, TOP_EDGE_OFFSET, DISPLAY_WIDTH - RIGHT_EDGE_OFFSET, DISPLAY_HEIGHT - BOTTOM_EDGE_OFFSET, al_map_rgba_f(0, 0, 0, 0.5));

	//draw HUD elements
	//health
	al_draw_rectangle(
		HUD_EDGE_OFFSET, //x1
		HUD_EDGE_OFFSET, //y1
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET, //x2
		HUD_EDGE_OFFSET + HUD_ELEMENT_HEIGHT, //y2
		al_map_rgb(0, 0, 0), 5);
	al_draw_filled_rectangle(HUD_EDGE_OFFSET, 
		HUD_EDGE_OFFSET, 
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET, 
		HUD_EDGE_OFFSET + HUD_ELEMENT_HEIGHT, 
		al_map_rgba_f(0, 1.0, 0, 1.0));
	al_draw_filled_rectangle(HUD_EDGE_OFFSET, 
		HUD_EDGE_OFFSET, 
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET, 
		HUD_EDGE_OFFSET + HUD_ELEMENT_HEIGHT, 
		al_map_rgba_f(0, 0, 0, 0.95));
	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 1.0, 0.0, 1.0), 
		HUD_EDGE_OFFSET + HUD_TEXT_OFFSET, //x
		HUD_EDGE_OFFSET + HUD_TEXT_OFFSET, //y
		ALLEGRO_ALIGN_LEFT, "Health:");
	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 1.0, 0.0, 1.0),
		HUD_EDGE_OFFSET + HUD_TEXT_OFFSET, //x
		HUD_EDGE_OFFSET + HUD_TEXT_OFFSET + HUD_DYNAMIC_TEXT_OFFSET, //y
		ALLEGRO_ALIGN_LEFT, (to_string(health) + " / " + to_string(STARTING_HEALTH_POINTS)).c_str());

	//altitude
	al_draw_rectangle(
		HUD_EDGE_OFFSET, 
		HUD_EDGE_OFFSET + HUD_ELEMENT_HEIGHT + HUD_ELEMENT_OFFSET, 
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET, 
		HUD_EDGE_OFFSET + (2 * HUD_ELEMENT_HEIGHT) + HUD_ELEMENT_OFFSET,
		al_map_rgb(0, 0, 0), 5);
	al_draw_filled_rectangle(
		HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + HUD_ELEMENT_HEIGHT + HUD_ELEMENT_OFFSET,
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + (2 * HUD_ELEMENT_HEIGHT) + HUD_ELEMENT_OFFSET, 
		al_map_rgba_f(0, 1.0, 0, 1.0));
	al_draw_filled_rectangle(
		HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + HUD_ELEMENT_HEIGHT + HUD_ELEMENT_OFFSET,
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + (2 * HUD_ELEMENT_HEIGHT) + HUD_ELEMENT_OFFSET, 
		al_map_rgba_f(0, 0, 0, 0.95));
	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 1.0, 0.0, 1.0), 
		HUD_EDGE_OFFSET + HUD_TEXT_OFFSET, 
		HUD_EDGE_OFFSET + HUD_ELEMENT_HEIGHT + HUD_ELEMENT_OFFSET + HUD_TEXT_OFFSET, 
		ALLEGRO_ALIGN_LEFT, "Altitude:");
	al_draw_text(fonts->body, al_map_rgba_f(0.0, 1.0, 0.0, 1.0),
		HUD_EDGE_OFFSET + HUD_TEXT_OFFSET,
		HUD_EDGE_OFFSET + HUD_ELEMENT_HEIGHT + HUD_ELEMENT_OFFSET + HUD_TEXT_OFFSET + HUD_DYNAMIC_TEXT_OFFSET,
		ALLEGRO_ALIGN_LEFT, (to_string(altitude) + " Km").c_str());

	//trajectory
	al_draw_rectangle(
		HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + (2 * HUD_ELEMENT_HEIGHT) + (2 * HUD_ELEMENT_OFFSET),
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + (3 * HUD_ELEMENT_HEIGHT) + (2 * HUD_ELEMENT_OFFSET),
		al_map_rgb(0, 0, 0), 5);
	al_draw_filled_rectangle(
		HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + (2 * HUD_ELEMENT_HEIGHT) + (2 * HUD_ELEMENT_OFFSET),
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + (3 * HUD_ELEMENT_HEIGHT) + (2 * HUD_ELEMENT_OFFSET),
		al_map_rgba_f(0, 1.0, 0, 1.0));
	al_draw_filled_rectangle(
		HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + (2 * HUD_ELEMENT_HEIGHT) + (2 * HUD_ELEMENT_OFFSET),
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET,
		HUD_EDGE_OFFSET + (3 * HUD_ELEMENT_HEIGHT) + (2 * HUD_ELEMENT_OFFSET),
		al_map_rgba_f(0, 0, 0, 0.95));
	al_draw_text(fonts->subHeading, al_map_rgba_f(0.0, 1.0, 0.0, 1.0),
		HUD_EDGE_OFFSET + HUD_TEXT_OFFSET,
		HUD_EDGE_OFFSET + (2 * HUD_ELEMENT_HEIGHT) + (2 * HUD_ELEMENT_OFFSET) + HUD_TEXT_OFFSET,
		ALLEGRO_ALIGN_LEFT, "Trajectory:");
	al_draw_multiline_text(fonts->body, al_map_rgba_f(0.0, 1.0, 0.0, 1.0),
		HUD_EDGE_OFFSET + HUD_TEXT_OFFSET,
		HUD_EDGE_OFFSET + (2 * HUD_ELEMENT_HEIGHT) + (2 * HUD_ELEMENT_OFFSET) + HUD_TEXT_OFFSET + HUD_DYNAMIC_TEXT_OFFSET,
		LEFT_EDGE_OFFSET - HUD_EDGE_OFFSET - 30,
		30,
		ALLEGRO_ALIGN_LEFT, trajectory.c_str());
}

void displayResultPage(const MFonts* fonts, ALLEGRO_DISPLAY* display, int turnsPassed)
{
	int altitude = STARTING_ALTITUDE;
	for (int i = turnsPassed; i > 0; i--)
	{
		if (i > 2808) altitude += 5000;
		else if (i > 880) altitude += 500;
		else if (i > 200) altitude += 50;
		else if (i > 20) altitude += 10;
		else altitude += 5;
	}
	string trajectory;
	if (altitude > 1000000) trajectory = "Escape";
	else if (altitude > 36000) trajectory = "High Earth orbit";
	else if (altitude > 2000) trajectory = "Medium Earth orbit";
	else if (altitude > 200) trajectory = "Low Earth orbit";
	else trajectory = "Sub-orbital";

	al_clear_to_color(al_map_rgb(255, 255, 255));
	al_draw_filled_rectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, al_map_rgba_f(0, 0, 0, 0.7));

	//death message
	al_draw_multiline_text(fonts->body, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), DISPLAY_WIDTH / 2, 100, DISPLAY_WIDTH - TEXT_EDGE_OFFSET, 30, ALLEGRO_ALIGN_CENTRE,
		"Your ship buckles from the continuous debris impacts. Critical systems malfunction and fail. Your engines sputter, but ultimately flame out.\
		 As you drift helplessly through the field of debris, you wonder why you were sent on an impossible mission. Was it a lack of preparedness?\
		 Was it politicians desperately trying to cling to power by selling hope to the people? Was it humanity\'s hubris at work yet again?\
		 As a chunk of space debris finally collides with the bridge, you realize that whatever the answer is, you will never know.\
		 Just as Jericho was humanity\'s first city, it now becomes its last. ");

	//final score
	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 500, ALLEGRO_ALIGN_CENTRE, "Final Altitude:");
	al_draw_text(fonts->subHeading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 570, ALLEGRO_ALIGN_CENTRE, (to_string(altitude) + " Km").c_str());

	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 650, ALLEGRO_ALIGN_CENTRE, "Final Trajectory:");
	al_draw_text(fonts->subHeading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 720, ALLEGRO_ALIGN_CENTRE, trajectory.c_str());

	al_draw_text(fonts->heading, al_map_rgba_f(0.0, 0.0, 0.0, 1.0), (DISPLAY_WIDTH / 2), 900, ALLEGRO_ALIGN_CENTRE, "Press ESC to exit game");
}

bool collision(BitObject asteroid, BitObject spaceship)
{
	if (((asteroid.x + asteroid.width)	> (spaceship.x + spaceship.width / (3 * SHIP_SCALING_FACTOR)))			&& //left hitbox
		((asteroid.x)					< (spaceship.x + 2 * spaceship.width / (3 * SHIP_SCALING_FACTOR)))		&& //right hitbox
		((asteroid.y + asteroid.width)	> (spaceship.y + 10))													&& //top hitbox
		((asteroid.y)					< (spaceship.y + 10 + spaceship.height / (3 * SHIP_SCALING_FACTOR))))	   //bottom hitbox
	{
		return true;
	}
	else
		return false;
}

void draw_spaceship_hitbox(BitObject spaceship)
{
	al_draw_circle(spaceship.x + spaceship.width / (3 * SHIP_SCALING_FACTOR), spaceship.y + 10, 3, al_map_rgb(255, 0, 0), 2); //top left
	al_draw_circle(spaceship.x + 2 * spaceship.width / (3 * SHIP_SCALING_FACTOR), spaceship.y + 10, 3, al_map_rgb(255, 0, 0), 2); //top right
	al_draw_circle(spaceship.x + spaceship.width / (3 * SHIP_SCALING_FACTOR), spaceship.y + 10 + spaceship.height / (3 * SHIP_SCALING_FACTOR), 3, al_map_rgb(255, 0, 0), 2); //bottom left
	al_draw_circle(spaceship.x + 2 * spaceship.width / (3 * SHIP_SCALING_FACTOR), spaceship.y + 10 + spaceship.height / (3 * SHIP_SCALING_FACTOR), 3, al_map_rgb(255, 0, 0), 2); //bottom right
}

void draw_asteroid_hitbox(BitObject asteroid)
{
	al_draw_circle(asteroid.x, asteroid.y, 3, al_map_rgb(255, 0, 0), 2); //top left
	al_draw_circle(asteroid.x + asteroid.width, asteroid.y, 3, al_map_rgb(255, 0, 0), 2); //top right
	al_draw_circle(asteroid.x, asteroid.y + asteroid.height, 3, al_map_rgb(255, 0, 0), 2); //bottom left
	al_draw_circle(asteroid.x + asteroid.width, asteroid.y + asteroid.height, 3, al_map_rgb(255, 0, 0), 2); //bottom right
}