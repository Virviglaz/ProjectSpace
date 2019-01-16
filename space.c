#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "framebuffer.h"
#include "font8x8_basic.h"

const uint16_t radius[] = { 2, 5 }; // min, max
const uint16_t speed_x10[] = { 1, 10 }; // min, max
const uint16_t weight[] = { 10, 50 }; // min, max
const uint32_t colors[] = { WHITE32, RED32, GREEN32, BLUE32, YELLOW32, CYAN32, MAGENTA32, SILVER32, GRAY32, MAROON32, OLIVE32 };
const char * names[] = { "SUN", "MERCURY", "VENUS", "EARTH", "MARS", "JUPITER", "SATURN", "URANUS", "NEPTUNE" };
const double G = 1; //gravity constant
const uint8_t GAP = 5;
struct 
{
	double X, Y;
}screen_center;

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

	//previous positions
	double x, y, px, py, vx, vy, ax, ay;

	//object name
	const char * name;
	bool isMoving;

	//after impact 2 objects become 1
	bool isAlive;

	bool isMassCenter;

	bool fillLastTime;
}object_t;

/* Objects */
object_t ** Objects;

/* Predefined objects */
object_t planets[] = {
	{ .color = RED32, .r = 50, .vx = 0, .vy = 0, .weight = 10000, .name = "STAR", .x = 0, .y = 0 },
	{ .color = GRAY32, .r = 10, .vx = 0, .vy = 3, .weight = 1000, .name = "PLANET", .x = 500, .y = 0 },
	{ .color = YELLOW32, .r = 5, .vx = 0, .vy = 8.7, .weight = 10, .name = "MOON", .x = 530, .y = 0 },
	{ .color = BLUE32, .r = 10, .vx = 0, .vy = -3, .weight = 1000, .name = "MOON", .x = -500, .y = 0 },
	//{ .color = GREEN32, .r = 10, .vx = -6,. vy = 0, .weight = 1000, .name = "MOON", .x = 0, .y = 500 },
};


/* Functions */
static object_t ** create_predefined_objects (uint16_t * amount);
static object_t ** create_random_objects (uint16_t amount);
static object_t * create_random_object (void);
static double distance (object_t * o1, object_t * o2);
static double distanceSquare (object_t * o1, object_t * o2);
static bool check_impact (object_t * o1, object_t * o2);
static void move (object_t ** objects, uint16_t object_s);
static void draw_object (object_t ** objects, uint16_t object_s);
static void border_impact (object_t * object);
static object_t * mass_center (object_t ** objects, uint16_t object_s);
static void gravity (object_t * object, object_t * ref_object);
static void process_impact (object_t * object, object_t * ref_object);
static void process_impact_all (object_t ** object, uint16_t object_s);
static void gravity_object_to_object (object_t ** object, uint16_t object_s);
static void gravity_oject_to_massCenter (object_t ** object, uint16_t object_s, object_t * massCenter);
static uint32_t mix_color(uint32_t c1, uint32_t c2, double w1, double w2);

/**
 * Initialize and run simulation
 */
void space_init (uint16_t objects_s, uint32_t backColor)
{
	GetScreenSize(&lcd_width, &lcd_heigh);
	printf("Screen: %u x %u\n", lcd_width, lcd_heigh);
	screen_center.X = lcd_width / 2;
	screen_center.Y = lcd_heigh / 2;

	ClearScreen(lcd_backColor = backColor);

	SetFont((void*)&font);

	//run with parameter ('./ps 2') will init 2 random objects, otherwise will use predefined ones
	Objects = objects_s ? create_random_objects(objects_s) : create_predefined_objects(&objects_s);

	while(1)
	{
		usleep(10000);
		//for (uint8_t i = 0; i != objects_s; i++)
		{
			mass_center(Objects, objects_s);
			//gravity_oject_to_massCenter(Objects, objects_s, _mass_center);

			// MOVEMENT
			move(Objects, objects_s);

			draw_object(Objects, objects_s);

			// BORDER IMPACT
			//border_impact(Objects[i]);

			// IMPACT PROCESS
			process_impact_all(Objects, objects_s);

			// GRAVITY FOR EACH OBJECT OR TO MASS CENTER
			gravity_object_to_object(Objects, objects_s);
		}
		FrameBufferUpdate();
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
		_objects[i]->isMassCenter = false;
		_objects[i]->fillLastTime = false;

		printf("Name: %s\tx = %4.0f\ty = %4.0f\tr = %u\tx speed = %3.0f\ty speed = %3.0f\tWeight = %4.0f\n",\
				_objects[i]->name, _objects[i]->x, _objects[i]->y, _objects[i]->r, _objects[i]->vx, _objects[i]->vy, _objects[i]->weight);
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
	object->vx = rand() % (speed_x10[1] - speed_x10[0]) + speed_x10[0];
	object->vy = rand() % (speed_x10[1] - speed_x10[0]) + speed_x10[0];
	object->color = colors[rand() % (sizeof(colors) / sizeof(colors[0]))];
	object->px = 0;
	object->py = 0;
	object->weight = rand() % (weight[1] - weight[0]) + weight[0];
	object->isMoving = true;
	object->isAlive = true;
	object->isMassCenter = false;
	object->fillLastTime = false;

	if (i < (sizeof(names) / sizeof(*names)))
		object->name = names[i];
	else
	{
		object->name = malloc(20);
		sprintf((void*)object->name, "Object %u", i);
	}
	i++;

	object->vx *= rand() > rand() ? 1 : -1;
	object->vy *= rand() > rand() ? 1 : -1;

	printf("Name: %s\tx = %4.0f\ty = %4.0f\tr = %u\tx speed = %3.0f\ty speed = %3.0f\tWeight = %3.0f\n",\
			object->name, object->x, object->y, object->r, object->vx, object->vy, object->weight);

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

static void move (object_t ** objects, uint16_t object_s)
{
	for (uint16_t i = 0; i != object_s; i++)
	{
		if (!objects[i]->isMoving)
			continue;

		/* acceleration -> speed */
		objects[i]->vx += objects[i]->ax;
		objects[i]->vy += objects[i]->ay;

		/* speed -> position */
		objects[i]->x += objects[i]->vx;
		objects[i]->y += objects[i]->vy;
	}
}

static void draw_object (object_t ** objects, uint16_t object_s)
{
	for (uint16_t i = 0; i != object_s; i++)
	{
		if (objects[i]->fillLastTime)
		{
			objects[i]->fillLastTime = false;
			DrawFilledCircle32(screen_center.X + objects[i]->px, screen_center.Y + objects[i]->py, objects[i]->r, lcd_backColor); //remove object last time
		}

		if (!objects[i]->isAlive)
			continue;

		if (objects[i]->px || objects[i]->py)
			DrawFilledCircle32(screen_center.X + objects[i]->px, screen_center.Y + objects[i]->py, objects[i]->r, lcd_backColor); //remove object before redraw

		if (screen_center.X + objects[i]->x < objects[i]->r + GAP || screen_center.Y + objects[i]->y < objects[i]->r + GAP || \
			screen_center.X + objects[i]->x > lcd_width - objects[i]->r - GAP || screen_center.Y + objects[i]->y > lcd_heigh - objects[i]->r - GAP)
			continue;

		objects[i]->px = objects[i]->x;
		objects[i]->py = objects[i]->y;

		if (!objects[i]->isAlive) continue;

		DrawFilledCircle32(screen_center.X + objects[i]->x, screen_center.Y + objects[i]->y, objects[i]->r, objects[i]->color);

		if (objects[i]->name)
		{
			if (strlen(objects[i]->name) * font.FontXsize / 2 > objects[i]->r) continue;
			font.BackColor = objects[i]->color;
			font.FontColor = ~objects[i]->color;
			PrintText(screen_center.X + objects[i]->x - strlen(objects[i]->name) * font.FontXsize / 2, screen_center.Y + objects[i]->y - font.FontYsize / 2, objects[i]->name);
		}
	}
}

static void border_impact (object_t * object)
{
	if ((object->x <= (object->r + 5)) || (object->x >= (lcd_width - object->r - 5))) object->vx *= -1;
	if ((object->y <= (object->r + 5)) || (object->y >= (lcd_heigh - object->r - 5))) object->vy *= -1;
}

static object_t * mass_center (object_t ** objects, uint16_t object_s)
{
	const uint8_t cross_size = 10;
	static object_t _mass_center = { .x = 100, .y = 100, .px = 100, .py = 100, .weight = 0, .isAlive = true, .isMassCenter = true };

	//we calculate total mass only once. Total mass should be constant
	if (_mass_center.weight == 0)
		for (uint16_t i = 0; i != object_s; i++)
			_mass_center.weight += objects[i]->weight;

	_mass_center.x = 0;
	_mass_center.y = 0;

	for (uint16_t i = 0; i != object_s; i++)
		if (objects[i]->isAlive)
		{
			_mass_center.x += objects[i]->x * objects[i]->weight;
			_mass_center.y += objects[i]->y * objects[i]->weight;
		}

	_mass_center.x /= _mass_center.weight;
	_mass_center.y /= _mass_center.weight;

	//screen_center.X += _mass_center.x;
	//screen_center.Y += _mass_center.y;

	DrawCross(screen_center.X + _mass_center.px, screen_center.Y + _mass_center.py, cross_size, lcd_backColor);
	DrawCross(screen_center.X + _mass_center.x, screen_center.Y + _mass_center.y, cross_size, 0xFFFFFF00);

	_mass_center.px = _mass_center.x;
	_mass_center.py = _mass_center.y;

	return &_mass_center;
}

static void gravity (object_t * object, object_t * ref_object)
{
	if (object == ref_object) return; //object cannot be compared to himself
	if (!object->isAlive || !ref_object->isAlive) return; //dead object (after impact)

	/* positions */
	double dx = object->x - ref_object->x;
	double dy = object->y - ref_object->y;

	/* distance ^2 */
	double r2 = dx * dx + dy * dy;

	/* gravity law */
	double a = - G * ref_object->weight / r2;
	//double a = F / object->weight;

	/* distance */
	double r = sqrt(r2);

	/* acceleration */
	object->ax += a * dx / r;
	object->ay += a * dy / r;
}

static void process_impact (object_t * object, object_t * ref_object)
{
	if (object == ref_object) return; //object cannot be compared to himself

	if (!object->isAlive || !ref_object->isAlive) return; //dead object (after impact)

	if (!ref_object->isMassCenter) //we don't check impact with mass center - it is not a phisical object
		if (check_impact(object, ref_object)) //impact
		{
			object_t * o1, * o2;

			if (object->weight > ref_object->weight)
				o1 = object, o2 = ref_object;
			else
				o2 = object, o1 = ref_object;

			o2->isAlive = false; //kill first object
			o1->isAlive = true; //second object survive
			o2->fillLastTime = true;

			o1->vx = (o1->vx * o1->weight + o2->vx * o2->weight)\
					/ (o1->weight + o2->weight);

			o1->vy = (o1->vy * o1->weight + o2->vy * o2->weight)\
					/ (o1->weight + o2->weight);

			o1->weight += o2->weight; //add his mass to reference object

			o1->r = sqrt(pow(o2->r, 2) + pow(o1->r, 2)); //and increase size

			o1->color = mix_color(o1->color, o2->color, o1->weight, o2->weight);

			printf("Impact between %s and %s\n", o1->name, o2->name);

			return;
		}
}

static void process_impact_all (object_t ** object, uint16_t object_s)
{
	for (uint8_t i = 0; i != object_s; i++)
		for (uint8_t j = 0; j != object_s; j++)
			process_impact(object[i], object[j]);
}

static void gravity_object_to_object (object_t ** object, uint16_t object_s)
{
	for (uint8_t i = 0; i != object_s; i++)
	{
		object[i]->ax = 0;
		object[i]->ay = 0;
	}

	for (uint8_t i = 0; i != object_s; i++)
		for (uint8_t j = 0; j != object_s; j++)
			gravity(object[i], object[j]);
}

static void gravity_oject_to_massCenter (object_t ** object, uint16_t object_s, object_t * massCenter)
{
	for (uint8_t i = 0; i != object_s; i++)
		gravity(object[i], massCenter);
}

static uint32_t mix_color(uint32_t c1, uint32_t c2, double w1, double w2)
{
	union
	{
		uint8_t r, g, b, a;
		uint32_t rgb32;
	}rgb[3];

	rgb[0].rgb32 = c1;
	rgb[1].rgb32 = c2;

	rgb[2].r = (rgb[0].r * w1 + rgb[1].r * w2) / (w1 + w2);
	rgb[2].g = (rgb[0].g * w1 + rgb[1].g * w2) / (w1 + w2);
	rgb[2].b = (rgb[0].b * w1 + rgb[1].b * w2) / (w1 + w2);
	rgb[2].a = 0xFF;

	return rgb[2].rgb32;
}