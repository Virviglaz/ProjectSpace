#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "framebuffer.h"
#include "font8x8_basic.h"

const uint16_t radius[] = { 10, 20 }; // min, max
const uint16_t speed_x10[] = { 10, 25 }; // min, max
const uint16_t weight[] = { 10, 100 }; // min, max
const uint32_t colors[] = { WHITE32, RED32, GREEN32, BLUE32, YELLOW32, CYAN32, MAGENTA32, SILVER32, GRAY32, MAROON32, OLIVE32 };
const char * names[] = { "SUN", "MERCURY", "VENUS", "EARTH", "MARS", "JUPITER", "SATURN", "URANUS", "NEPTUNE" };
const double G = 10; //gravity constant
const double C = 50; //speed of light constant

uint16_t lcd_width, lcd_heigh; //lcd properties
uint32_t lcd_backColor;
Font_StructTypeDef font = { FONT8x8_XSIZE, FONT8x8_YSIZE, (void*)font8x8_basic, 0, 0xFFFFFFFF };

/* Object properties */
typedef struct
{
	//phisical parameters
	uint32_t color;
	uint16_t r; //radius
	double weight;

	//current positions and speeds
	double x_speed, y_speed;

	//previous positions
	double x, y, px, py;

	//object name
	const char * name;
	bool isMoving;

	//after impact 2 objects become 1
	bool isAlive;
}object_t;

/* Objects */
object_t ** Objects;

/* Predefined objects */
object_t planets[] = {
	{ .color = RED32, .r = 0, .x_speed = 0, .y_speed = 0, .weight = 1000, .name = "SUN", .x = 1000, .y = 500 },
	{ .color = GRAY32, .r = 0, .x_speed = 0, .y_speed = 30, .weight = 10, .name = "EARTH", .x = 1400, .y = 500 },
	{ .color = YELLOW32, .r = 0, .x_speed = -10, .y_speed = 32, .weight = 0.1, .name = "MOON", .x = 1340, .y = 500 },
};


/* Functions */
static object_t ** create_predefined_objects (uint16_t * amount);
static object_t ** create_random_objects (uint16_t amount);
static object_t * create_random_object (void);
static double distance (object_t * o1, object_t * o2);
static double distanceSquare (object_t * o1, object_t * o2);
static bool check_impact (object_t * o1, object_t * o2);
static void move (object_t * object);
static void draw_object (object_t * object);
static void border_impact (object_t * object);
static object_t * mass_center (object_t ** objects, uint16_t object_s);
static void gravity (object_t * object, object_t * ref_object);

/**
 * Initialize and run simulation
 */
void space_init (uint16_t objects_s, uint32_t backColor)
{
	GetScreenSize(&lcd_width, &lcd_heigh);
	printf("Screen: %u x %u\n", lcd_width, lcd_heigh);

	ClearScreen(lcd_backColor = backColor);

	SetFont((void*)&font);

	//run with parameter ('./ps 2') will init 2 random objects, otherwise will use predefined ones
	Objects = objects_s ? create_random_objects(objects_s) : create_predefined_objects(&objects_s);

	while(1)
	{
		usleep(10000);
		for (uint8_t i = 0; i != objects_s; i++)
		{
			// MOVEMENT
			move(Objects[i]);

			// BORDER IMPACT
			border_impact(Objects[i]);

			// GRAVITY
			for (uint8_t i = 0; i != objects_s; i++)
				for (uint8_t j = 0; j != objects_s; j++)
						gravity(Objects[i], Objects[j]);
		}

	}
}

/**
 * Create object array from 'planets' array
 */
static object_t ** create_predefined_objects (uint16_t * amount)
{
	const uint8_t _objects_s = sizeof(planets) / sizeof(object_t); //amount of objects defined in 'planets[]' array

	object_t ** _objects = malloc(sizeof(object_t *) * _objects_s);

	*amount = _objects_s;

	printf("Memory allocated for %u objects at %p\n", _objects_s, _objects);

	for (uint8_t i = 0; i != _objects_s; i++)
	{
		_objects[i] = &planets[i];

		// automatically calculate radius if needed (r == 0)
		if (_objects[i]->r == 0) _objects[i]->r = strlen(_objects[i]->name) * font.FontXsize / 2 + 5; //5px gap (-o-)

		//initial position put to 0 (undefined)
		_objects[i]->px = 0;
		_objects[i]->py = 0;

		//object can move
		_objects[i]->isMoving = true;
		_objects[i]->isAlive = true;

		printf("Name: %s\tx = %4.0f\ty = %4.0f\tr = %u\tx speed = %3.0f\ty speed = %3.0f\tWeight = %4.0f\n",\
				_objects[i]->name, _objects[i]->x, _objects[i]->y, _objects[i]->r, _objects[i]->x_speed, _objects[i]->y_speed, _objects[i]->weight);
	}

	return _objects;
}

/**
 * Create random object array
 */
static object_t ** create_random_objects (uint16_t amount)
{
	object_t ** _objects = malloc(sizeof(object_t *) * amount);

	printf("Memory allocated for %u objects at %p\n", amount, _objects);

	for (uint8_t i = 0; i != amount; i++)
		_objects[i] = create_random_object();

	return _objects;
}

/**
 * Create random object
 */
static object_t * create_random_object (void)
{
	static uint8_t i = 0;
	object_t * object = malloc (sizeof(object_t));

	object->r = rand() % (radius[1] - radius[0]) + radius[0];
	object->x = rand() % (lcd_width - 2 * object->r) + object->r;
	object->y = rand() % (lcd_heigh - 2 * object->r) + object->r;
	object->x_speed = (float)((int16_t)rand() % (speed_x10[1] - speed_x10[0]) + speed_x10[0]) / 10;
	object->y_speed = (float)((int16_t)rand() % (speed_x10[1] - speed_x10[0]) + speed_x10[0]) / 10;
	object->color = colors[rand() % (sizeof(colors) / sizeof(colors[0]))];
	object->px = 0;
	object->py = 0;
	object->weight = rand() % (weight[1] - weight[0]) + weight[0];
	object->name = i < (sizeof(names) / sizeof(*names)) ? names[i++] : "Undefined";
	object->isMoving = true;
	object->isAlive = true;

	printf("Name: %s\tx = %4.0f\ty = %4.0f\tr = %u\tx speed = %3.0f\ty speed = %3.0f\tWeight = %3.0f\n",\
			object->name, object->x, object->y, object->r, object->x_speed, object->y_speed, object->weight);

	return object;
}

static double distance (object_t * o1, object_t * o2)
{
	double dx = o1->x - o2->x;
	double dy = o1->y - o2->y;
	return sqrt(dx * dx + dy * dy);
}

static double distanceSquare (object_t * o1, object_t * o2)
{
	double dx = o1->x - o2->x;
	double dy = o1->y - o2->y;
	return dx * dx + dy * dy;
}

static bool check_impact (object_t * o1, object_t * o2)
{
	return distance(o1, o2) < (o1->r + o2->r) ? true : false;
}

static void move (object_t * object)
{
	if (!object->isMoving)
		return;

	object->x += object->x_speed / 10;
	object->y += object->y_speed / 10;

	draw_object(object);

	object->px = object->x;
	object->py = object->y;
}

static void draw_object (object_t * object)
{
	if ((uint16_t)object->px == (uint16_t)object->x && (uint16_t)object->py == (uint16_t)object->y) return;

	if (object->px && object->py)
		DrawFilledCircle32(object->px, object->py, object->r, lcd_backColor); //remove object before redraw

	if (!object->isAlive) return;

	DrawFilledCircle32(object->x, object->y, object->r, object->color);

	if (object->name)
	{
		if (strlen(object->name) * font.FontXsize / 2 > object->r) return;
		font.BackColor = object->color;
		font.FontColor = ~object->color;
		PrintText(object->x - strlen(object->name) * font.FontXsize / 2, object->y - font.FontYsize / 2, object->name);
	}
}

static void border_impact (object_t * object)
{
	if ((object->x <= (object->r + 5)) || (object->x >= (lcd_width - object->r - 5))) object->x_speed *= -1;
	if ((object->y <= (object->r + 5)) || (object->y >= (lcd_heigh - object->r - 5))) object->y_speed *= -1;
}

static object_t * mass_center (object_t ** objects, uint16_t object_s)
{
	const uint8_t cross_size = 10;
	static object_t _mass_center = { .x = 100, .y = 100, .px = 100, .py = 100, .weight = 0 };

	if (_mass_center.weight == 0)
		for (uint16_t i = 0; i != object_s; i++)
			_mass_center.weight += objects[i]->weight;

	_mass_center.x = 0;
	_mass_center.y = 0;

	for (uint16_t i = 0; i != object_s; i++)
	{
		_mass_center.x += objects[i]->x * objects[i]->weight;
		_mass_center.y += objects[i]->y * objects[i]->weight;
	}

	_mass_center.x /= _mass_center.weight;
	_mass_center.y /= _mass_center.weight;

	DrawCross(_mass_center.px, _mass_center.py, cross_size, lcd_backColor);
	DrawCross(_mass_center.x, _mass_center.y, cross_size, 0xFFFFFF00);

	_mass_center.px = _mass_center.x;
	_mass_center.py = _mass_center.y;

	return &_mass_center;
}

static void gravity (object_t * object, object_t * ref_object)
{
	if (object == ref_object) return; //object cannot be compared to himself
	if (!object->isAlive || !ref_object->isAlive) return; //dead object (after impact)

	double _distanceSquare = distanceSquare(object, ref_object);

	if (check_impact(object, ref_object)) //impact
	{
		object->isAlive = false; //kill object
		ref_object->isAlive = true; //reference object survive

		ref_object->x_speed = (ref_object->x_speed * ref_object->weight + object->x_speed * object->weight)\
				/ (ref_object->weight + object->weight);

		ref_object->y_speed = (ref_object->y_speed * ref_object->weight + object->y_speed * object->weight)\
				/ (ref_object->weight + object->weight);

		ref_object->weight += object->weight; //add his mass to reference object

		return;
	}

	double _vectorSpeed = G * ref_object->weight / _distanceSquare;

	double _distance = sqrt(_distanceSquare);

	object->x_speed += (ref_object->x - object->x) * _vectorSpeed / _distance;
	object->y_speed += (ref_object->y - object->y) * _vectorSpeed / _distance;

	if (object->x_speed > C || object->x_speed < -C) //speed of light limit for x
		object->x_speed = 0;

	if (object->y_speed > C || object->y_speed < -C) //speed of light limit for y
		object->y_speed = 0;
}

