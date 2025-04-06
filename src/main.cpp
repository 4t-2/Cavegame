#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <format>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <imgui.h>
#include <math.h>
#include <string>
#include <thread>
#include <vulkan/vulkan_core.h>

#include <enkimi.h>
#include <Mat.hpp>
/*#include "../inc/CommandBox.hpp"*/
#include "../inc/Mesh.hpp"
#include "../inc/Serializer.hpp"

// b5d1ff

#define GRAVACC (-.08 / 9)
#define WALKACC (1 / 3.)
#define BLCKFRC 0.6
#define AIRFRIC 0.60
#define FLYACCE 0.049

#define BASESPEED WALKVELPERTICK

#define FOREACH(x) for (int index = 0; index < x.size(); index++)

class FancyDescriptor
{
	public:
		Descriptor descriptor;
		Buffer	   buffer;
		bool free;
};

class DescriptorAllocator
{
	public:
		int				 bufferSize;
		int poolSize = 100;
		DescriptorPool	 pool;
		DescriptorLayout *layout;
		Instance *instance;

		std::vector<FancyDescriptor> allocated;

		DescriptorAllocator(Instance &instance, DescriptorLayout &layout, int bufferSize)
		{
			this->layout = &layout;
			this->instance = &instance;
			this->bufferSize = bufferSize;

			assert(layout.type == DescriptorLayout::Type::UNIFORM_BUFFER);

			pool = instance.createDescriptorPool(poolSize, 0, 0, 0, 0, 0, 0, poolSize, 0, 0, 0, 0);

			allocated.resize(poolSize);

			for(auto &e: allocated)
			{
				e.buffer = instance.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Instance::BufferAccess::CPUGPU);
				e.descriptor = pool.createDescriptor(layout, &e.buffer, nullptr, nullptr);
				e.free = true;
			}
		}

		FancyDescriptor* allocate()
		{
			for(auto &e: allocated)
			{
				if(e.free)
				{
					e.free = false;
					return &e;
				}
			}

			return nullptr;
		}
		
		void destroy(FancyDescriptor *f)
		{
			f->free = true;
		}
};

agl::Vec<int, 2> getPointerPos(Window &win)
{
	double x;
	double y;

	glfwGetCursorPos(win.window, &x, &y);

	return {x, y};
}

enum GameState
{
	RUNNING = 1,
	CMD		= 0,
	PAUSE	= 69
};

class Config
{
	public:
		bool showFps;
		int	 fpsCap;
};

template <typename T> void recurse(T processor, Config &s, std::string name = "null")
{
	processor.process(name, s);

	RECSER(s.showFps);
	RECSER(s.fpsCap);
}

enum ListenState
{
	Hold,
	First,
	Last,
	Null
};

class Listener
{
	private:
		bool pastState = false;

	public:
		ListenState ls = ListenState::Null;
		void		update(bool state)
		{
			if (state)
			{
				if (pastState)
				{
					ls = ListenState::Hold;
				}
				else
				{
					ls = ListenState::First;

					pastState = true;
				}
			}
			else if (pastState)
			{
				ls		  = ListenState::Last;
				pastState = false;
			}
		}
};

int getMillisecond()
{
	auto timepoint = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(timepoint).count();
}

agl::Vec<float, 2> getCursorScenePosition(agl::Vec<float, 2> cursorWinPos, agl::Vec<float, 2> winSize, float winScale,
										  agl::Vec<float, 2> cameraPos)
{
	return ((cursorWinPos - (winSize * .5)) * winScale) + cameraPos;
}

// inline bool &vecToMap(bool arr[3][3][3], agl::Vec<int, 3> vec)
// {
// 	return arr[1 + vec.x][1 + vec.y][1 + vec.z];
// }
//
// inline bool &vecToArr(bool arr[3][3][3], agl::Vec<int, 3> vec)
// {
// 	return arr[vec.x][vec.y][vec.z];
// }
//
// unsigned int AmOcCalc(bool blockMap[3][3][3], agl::Vec<int, 3> norm,
// agl::Vec<int, 3> acc1, agl::Vec<int, 3> acc2)
// {
// 	bool &cornerTouch = vecToMap(blockMap, norm + acc1 + acc2);
// 	bool &lineTouch	  = vecToMap(blockMap, norm + acc1);
// 	bool &oppo		  = vecToMap(blockMap, norm + acc2);

class Player
{
	public:
		int currentPallete = 0;
		int pallete[9];

		agl::Vec<float, 3> pos = {0, 150, 0};
		agl::Vec<float, 3> rot = {0, PI / 2, 0};
		agl::Vec<float, 3> vel = {0, 0, 0};

		float friction = BLCKFRC;

		bool sneaking  = false;
		bool sprinting = false;
		bool grounded  = false;

		void update()
		{
		}
};

void hideCursor(Window &window)
{
/*#ifdef __linux__*/
/*	Cursor		invisibleCursor;*/
/*	Pixmap		bitmapNoData;*/
/*	XColor		black;*/
/*	static char noData[] = {0, 0, 0, 0, 0, 0, 0, 0};*/
/*	black.red = black.green = black.blue = 0;*/
/**/
/*	bitmapNoData	= XCreateBitmapFromData(window.baseWindow.dpy, window.baseWindow.win, noData, 8, 8);*/
/*	invisibleCursor = XCreatePixmapCursor(window.baseWindow.dpy, bitmapNoData, bitmapNoData, &black, &black, 0, 0);*/
/*	XDefineCursor(window.baseWindow.dpy, window.baseWindow.win, invisibleCursor);*/
/*	XFreeCursor(window.baseWindow.dpy, invisibleCursor);*/
/*	XFreePixmap(window.baseWindow.dpy, bitmapNoData);*/
/*#endif*/
/**/
/*#ifdef _WIN32*/
	glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
/*#endif*/
}

// genSphereVertices
	// generates UV sphere vertices
	// cylinderx, x resolution
	// cylindery y resolution
	void genSphereVertices(std::vector<glm::vec4> &vertex, int cylinderx, int cylindery) {
		// the vertex data (x and y)
		vertex.resize(cylinderx * cylindery * 6);

		int index = 0;
		for (int x = 0; x < cylinderx; x++) {
			for (int y = 0; y < cylindery; y++) {

				vertex[index * 6 + 0].x = 0 + x;
				vertex[index * 6 + 0].y = 0 + y;
				vertex[index * 6 + 0].z = 0;
				vertex[index * 6 + 0].w = 1;

				vertex[index * 6 + 1].x = 1 + x;
				vertex[index * 6 + 1].y = 0 + y;
				vertex[index * 6 + 1].z = 0;
				vertex[index * 6 + 1].w = 1;

				vertex[index * 6 + 2].x = 0 + x;
				vertex[index * 6 + 2].y = 1 + y;
				vertex[index * 6 + 2].z = 0;
				vertex[index * 6 + 2].w = 1;

				vertex[index * 6 + 3].x = 0 + x;
				vertex[index * 6 + 3].y = 1 + y;
				vertex[index * 6 + 3].z = 0;
				vertex[index * 6 + 3].w = 1;

				vertex[index * 6 + 4].x = 1 + x;
				vertex[index * 6 + 4].y = 0 + y;
				vertex[index * 6 + 4].z = 0;
				vertex[index * 6 + 4].w = 1;

				vertex[index * 6 + 5].x = 1 + x;
				vertex[index * 6 + 5].y = 1 + y;
				vertex[index * 6 + 5].z = 0;
				vertex[index * 6 + 5].w = 1;
				index++;
			}
		}

		for (int i = 0; i < cylinderx * cylindery * 6; i++) {
			float x = vertex[i].x;

			vertex[i].x = (float) sin(2 * PI * (x / cylinderx));
			vertex[i].y = 2 * (vertex[i].y / cylindery) - 1;
			vertex[i].z = (float) cos(2 * PI * (x / cylinderx));
		}

		for (int i = 0; i < cylinderx * cylindery * 6; i++) {
			float y = vertex[i].y;
			float scale = (float) cos(y * PI / 2);
			float height = (float) sin(y * PI / 2);

			vertex[i ].x *= scale;
			vertex[i ].y = height;
			vertex[i ].z *= scale;
		}
	}

struct Collision
{
		agl::Vec<int, 3> norm;
		float			 overlap;
};

struct Box
{
		agl::Vec<float, 3> pos;
		agl::Vec<float, 3> size;
};

Collision boxCollide(Box b1, Box b2)
{
	// Y test
	agl::Vec<float, 3> overlap;

	{
		overlap.y = (b1.pos.y + b1.size.y) - b2.pos.y;

		if (overlap.y < 0)
		{
			return {};
		}
	}

	return {{0, 1, 0}, overlap.y};
}

void correctPositionXZ(Player &player, World &world)
{
	// correct side pos

	// +X
	{
		int x = player.pos.x + 0.3;
		int z = player.pos.z;

		if (x != int(player.pos.x))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				if (world.getAtPos({x, y, z}))
				{
					player.vel.x = 0;
					player.pos.x = x - .3;
				}
			}
		}
	}
	// -X
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z;

		if (x != int(player.pos.x))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				if (world.getAtPos({x, y, z}))
				{
					player.vel.x = 0;
					player.pos.x = x + 1.3;
				}
			}
		}
	}
	// +Z
	{
		int x = player.pos.x;
		int z = player.pos.z + 0.3;

		if (z != int(player.pos.z))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				if (world.getAtPos({x, y, z}))
				{
					player.vel.z = 0;
					player.pos.z = z - .3;
				}
			}
		}
	}
	// -Z
	{
		int x = player.pos.x;
		int z = player.pos.z - 0.3;

		if (z != int(player.pos.z))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				if (world.getAtPos({x, y, z}))
				{
					player.vel.z = 0;
					player.pos.z = z + 1.3;
				}
			}
		}
	}

	// diagonal

	if (!(agl::Vec<float, 3>{int(player.pos.x - .3), (int)player.pos.y, int(player.pos.z - .3)} == player.pos))
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z - 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.vel.x) > fabs(player.vel.z))
				{
					player.vel.x = 0;
					player.pos.x = x + 1.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z + 1.3;
				}
			}
		}
	}
	if (!(agl::Vec<float, 3>{int(player.pos.x + 0.3), (int)player.pos.y, int(player.pos.z - .3)} == player.pos))
	{
		int x = player.pos.x + 0.3;
		int z = player.pos.z - 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.vel.x) > fabs(player.vel.z))
				{
					player.vel.x = 0;
					player.pos.x = x - 0.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z + 1.3;
				}
			}
		}
	}
	if (!(agl::Vec<float, 3>{int(player.pos.x - .3), (int)player.pos.y, (int)(player.pos.z + .3)} == player.pos))
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z + 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.vel.x) > fabs(player.vel.z))
				{
					player.vel.x = 0;
					player.pos.x = x + 1.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z - 0.3;
				}
			}
		}
	}
	if (!(agl::Vec<float, 3>{(int)(player.pos.x + .3), (int)player.pos.y, (int)(player.pos.z + .3)} == player.pos))
	{
		int x = player.pos.x + 0.3;
		int z = player.pos.z + 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.vel.x) > fabs(player.vel.z))
				{
					player.vel.x = 0;
					player.pos.x = x - 0.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z - 0.3;
				}
			}
		}
	}

	/*// diagonal*/
	/**/
	/*int xoff = 0;*/
	/*if (player.pos.x - (int)player.pos.x > .7)*/
	/*{*/
	/*	xoff = 1;*/
	/*}*/
	/*else if (player.pos.x - (int)player.pos.x < .3)*/
	/*{*/
	/*	xoff = -1;*/
	/*}*/
	/*int zoff = 0;*/
	/*if (player.pos.z - (int)player.pos.z > .7)*/
	/*{*/
	/*	zoff = 1;*/
	/*}*/
	/*else if (player.pos.z - (int)player.pos.z < .3)*/
	/*{*/
	/*	zoff = -1;*/
	/*}*/
	/**/
	/*if (xoff != 0 && zoff != 0)*/
	/*{*/
	/*	int x = player.pos.x + xoff;*/
	/*	int z = player.pos.z + zoff;*/
	/**/
	/*	for (int y = player.pos.y; y < player.pos.y + 1.8; y++)*/
	/*	{*/
	/*		if (world.getAtPos({x, y, z}))*/
	/*		{*/
	/*			if (fabs(player.vel.x) > fabs(player.vel.z))*/
	/*			{*/
	/*				if (player.vel.x > 0 && xoff > 0)*/
	/*				{*/
	/*					player.vel.x = 0;*/
	/*					player.pos.x = x - .3;*/
	/*				}*/
	/*				else if (player.vel.x < 0 && xoff < 0)*/
	/*				{*/
	/*					player.vel.x = 0;*/
	/*					player.pos.x = x + 1.3;*/
	/*				}*/
	/*			}*/
	/*			else*/
	/*			{*/
	/*				if (player.vel.z > 0 && zoff > 0)*/
	/*				{*/
	/*					player.vel.z = 0;*/
	/*					player.pos.z = z - .3;*/
	/*				}*/
	/*				else if (player.vel.z < 0 && zoff < 0)*/
	/*				{*/
	/*					player.vel.z = 0;*/
	/*					player.pos.z = z + 1.3;*/
	/*				}*/
	/*			}*/
	/*		}*/
	/*	}*/
	/*}*/
}

void correctPositionY(Player &player, World &world)
{
	player.grounded = false;

	for (int x = player.pos.x - 0.29; x < player.pos.x + 0.29; x++)
	{
		for (int z = player.pos.z - 0.29; z < player.pos.z + 0.29; z++)
		{
			if (world.getAtPos({x, player.pos.y, z}) && player.vel.y < 0)
			{
				if (player.pos.y - (int)player.pos.y < .5)
				{
					continue;
				}
				player.vel.y = 0;
				player.pos.y = (int)player.pos.y + 1;

				player.grounded = true;
			}

			if (world.getAtPos({x, player.pos.y + 1.8, z}) && player.vel.y > 0)
			{
				player.vel.y = 0;
				player.pos.y = (int)(player.pos.y + 1.8) - 1.8;
			}
		}
	}
}

void correctPositionX(Player &player, World &world)
{
	// +X
	{
		int x = player.pos.x + 0.3;

		if (x != int(player.pos.x))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				for(int z = player.pos.z - 0.3; z < player.pos.z + 0.3; z++)
				{
					if (world.getAtPos({x, y, z}))
					{
						player.vel.x = 0;
						player.pos.x = x - .3;
					}
					break;
				}
			}
		}
	}
	// -X
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z;

		if (x != int(player.pos.x))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				for(int z = player.pos.z - 0.3; z < player.pos.z + 0.3; z++)
				{
					if (world.getAtPos({x, y, z}))
					{
						player.vel.x = 0;
						player.pos.x = x + 1.3;
					}
					break;
				}
			}
		}
	}
}

void correctPositionZ(Player &player, World &world)
{
	// +Z
	{
		int z = player.pos.z + 0.3;

		if (z != int(player.pos.z))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				for(int x = player.pos.x - 0.3; x < player.pos.x + 0.3; x++)
				{
					if (world.getAtPos({x, y, z}))
					{
						player.vel.z = 0;
						player.pos.z = z - .3;
					}
					break;
				}
			}
		}
	}
	// -Z
	{
		int x = player.pos.x;
		int z = player.pos.z - 0.3;

		if (z != int(player.pos.z))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				for(int x = player.pos.x - 0.3; x < player.pos.x + 0.3; x++)
				{
					if (world.getAtPos({x, y, z}))
					{
						player.vel.z = 0;
						player.pos.z = z + 1.3;
					}
					break;
				}
			}
		}
	}
}

void correctPositionDiagonal(Player &player, World &world)
{
	// diagonal

	if (!(agl::Vec<float, 3>{int(player.pos.x - .3), (int)player.pos.y, int(player.pos.z - .3)} == player.pos))
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z - 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.pos.x - x) < fabs(player.pos.z - z))
				{
					player.vel.x = 0;
					player.pos.x = x + 1.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z + 1.3;
				}
			}
		}
	}
	if (!(agl::Vec<float, 3>{int(player.pos.x + 0.3), (int)player.pos.y, int(player.pos.z - .3)} == player.pos))
	{
		int x = player.pos.x + 0.3;
		int z = player.pos.z - 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.pos.x - x) < fabs(player.pos.z - z))
				{
					player.vel.x = 0;
					player.pos.x = x - 0.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z + 1.3;
				}
			}
		}
	}
	if (!(agl::Vec<float, 3>{int(player.pos.x - .3), (int)player.pos.y, (int)(player.pos.z + .3)} == player.pos))
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z + 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.pos.x - x) < fabs(player.pos.z - z))
				{
					player.vel.x = 0;
					player.pos.x = x + 1.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z - 0.3;
				}
			}
		}
	}
	if (!(agl::Vec<float, 3>{(int)(player.pos.x + .3), (int)player.pos.y, (int)(player.pos.z + .3)} == player.pos))
	{
		int x = player.pos.x + 0.3;
		int z = player.pos.z + 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.pos.x - x) < fabs(player.pos.z - z))
				{
					player.vel.x = 0;
					player.pos.x = x - 0.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z - 0.3;
				}
			}
		}
	}
}

void movePlayer(Player &player, agl::Vec<float, 3> acc, World &world)
{
	/*player.vel.y *= 0.98;*/
	/*player.vel.y += GRAVACC;*/

	player.vel *= AIRFRIC * 0.91;
	acc *= FLYACCE * 0.98;

	float mod = 1;
	if (player.sneaking)
	{
		mod *= .3;
	}
	if (player.sprinting)
	{
		mod *= 2;
	}

	acc *= mod;
	// if (acc.length() > std::max(mod, 1.f) / 3)
	// {
	// 	acc = acc.normalized() * mod / 3;
	// }

	player.vel += acc;

	player.pos.y += player.vel.y;

	/*correctPositionY(player, world);*/

	player.pos.x += player.vel.x;
	
	/*correctPositionX(player, world);*/

	player.pos.z += player.vel.z;

	/*correctPositionZ(player, world);*/

	/*correctPositionDiagonal(player, world);*/
}

void updateSelected(Player &player, agl::Vec<int, 3> &selected, agl::Vec<int, 3> &front, World &world)
{
	agl::Vec<float, 3> dir = {-sin(player.rot.y) * cos(player.rot.x), -sin(player.rot.x),
							  -cos(player.rot.y) * cos(player.rot.x)};

	agl::Vec<float, 3> blockPos = player.pos + agl::Vec<float, 3>{0, 1.8, 0};

	selected = blockPos;
	front	 = blockPos;

	float dist = 0;

	while (true)
	{
		if (world.getAtPos(blockPos))
		{
			selected = blockPos;
			break;
		}

		blockPos += dir / 100;

		dist += dir.length() / 100;

		if (selected == agl::Vec<int, 3>(blockPos))
		{
		}
		else
		{
			front	 = selected;
			selected = blockPos;
		}

		if (dist > 10)
		{
			selected = player.pos;
			front	 = player.pos;
			break;
		}
	}
}

/*void toggleFullscreen(Display *display, Window window)*/
/*{*/
	/*Atom wmState	= XInternAtom(display, "_NET_WM_STATE", False);*/
	/*Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);*/
	/**/
	/*XEvent xev				 = {0};*/
	/*xev.type				 = ClientMessage;*/
	/*xev.xclient.window		 = window;*/
	/*xev.xclient.message_type = wmState;*/
	/*xev.xclient.format		 = 32;*/
	/*xev.xclient.data.l[0]	 = 2; // _NET_WM_STATE_ADD*/
	/*xev.xclient.data.l[1]	 = fullscreen;*/
	/*xev.xclient.data.l[2]	 = 0; // no second property to toggle*/
	/**/
	/*XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &xev);*/
/*}*/

void save(std::string path, World &world)
{
	std::fstream fs(path, std::ios::out);

	for (auto e : world.loadedChunks)
	{
		fs.write(reinterpret_cast<char *>((int *)&e.first.x), sizeof(int));
		fs.write(reinterpret_cast<char *>((int *)&e.first.y), sizeof(int));
		fs.write(reinterpret_cast<char *>((int *)&e.first.z), sizeof(int));

		for (int x = 0; x < 16; x++)
		{
			for (int y = 0; y < 384; y++)
			{
				for (int z = 0; z < 16; z++)
				{
					unsigned int id = e.second.blocks[x][y][z].type;
					fs.write(reinterpret_cast<char *>(&id), sizeof(id));
				}
			}
		}
	}

	fs.close();
}

void load(std::string path, World &world)
{
	std::fstream fs(path, std::ios::in);

	while (!fs.eof())
	{
		int x = 0;
		int y = 0;
		int z = 0;
		fs.read(reinterpret_cast<char *>(&x), sizeof(int));
		fs.read(reinterpret_cast<char *>(&y), sizeof(int));
		fs.read(reinterpret_cast<char *>(&z), sizeof(int));

		auto &chunk = world.loadedChunks[{x, y, z}];

		for (int x = 0; x < 16; x++)
		{
			for (int y = 0; y < 384; y++)
			{
				for (int z = 0; z < 16; z++)
				{
					unsigned int id = 0;
					fs.read(reinterpret_cast<char *>(&id), sizeof(id));

					chunk.blocks[x][y][z].type = id;
				}
			}
		}
	}

	fs.close();
}

int main()
{
	printf("Starting AGL\n");

	Instance instance;
	glfwInit();
	instance.bootstrap(1);
	Window window = instance.createWindow(1920, 1080, "CaveGame", true);

	agl::Vec<int, 2> windowSize;

	std::cout << "Shader Compilation" << '\n';

	DescriptorPool genericDescriptorPool = instance.createDescriptorPool(100);


	std::vector<glm::vec4> triangleData = {
		{0, 1, 0, 1}, 
		{1, 0, 0, 1}, 
		{0, 0, 0, 1},
	};

	std::vector<glm::vec4> unitSquareData = {
		{0, 0, 0, 1},
		{1, 0, 0, 1},
		{0, 1, 0, 1},
		{1, 0, 0, 1},
		{0, 1, 0, 1},
		{1, 1, 0, 1}
	};

	std::vector<glm::vec4> uvSphereData;
	genSphereVertices(uvSphereData, 20, 20);

	Buffer triangleBuffer = instance.createBufferWrite(unitSquareData, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	Buffer unitSquareBuffer = instance.createBufferWrite(unitSquareData, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	Buffer sphereBuffer = instance.createBufferWrite(uvSphereData, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

	/*ax::Program blockShader(ax::Shader("./shader/blockVert.glsl", GL_VERTEX_SHADER),*/
	/*						ax::Shader("./shader/blockFrag.glsl", GL_FRAGMENT_SHADER));*/
	/**/
	/*ax::Program uiShader(ax::Shader("./shader/frag.glsl", GL_FRAGMENT_SHADER),*/
	/*					 ax::Shader("./shader/uivert.glsl", GL_VERTEX_SHADER));*/

	struct {
		DescriptorLayout vertex;
		DescriptorLayout mvp;
		DescriptorLayout textureSampler;
	} blockShaderLayout = {
		instance.createLayout(0, DescriptorLayout::Type::STORAGE_BUFFER, DescriptorLayout::Stage::VERTEX),
		instance.createLayout(0, DescriptorLayout::Type::UNIFORM_BUFFER, DescriptorLayout::Stage::VERTEX),
		instance.createLayout(0, DescriptorLayout::Type::COMBINED_IMAGE_SAMPLER, DescriptorLayout::Stage::FRAGMENT),
	};

	Buffer mvpBuffer = instance.createBuffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Instance::BufferAccess::CPUGPU);
	Descriptor mvpDescriptor = genericDescriptorPool.createDescriptor(blockShaderLayout.mvp, &mvpBuffer, nullptr, nullptr);

	Pipeline blockShader = instance.createGraphicsPipeline("./shader/blockVert.spv", "./shader/blockFrag.spv", {blockShaderLayout.vertex.layout, blockShaderLayout.mvp.layout, blockShaderLayout.textureSampler.layout}, window.renderPass);

	DescriptorLayout skyShaderVertex = instance.createLayout(0, DescriptorLayout::STORAGE_BUFFER, DescriptorLayout::Stage::VERTEX);
	DescriptorLayout skyShaderTransform = instance.createLayout(0, DescriptorLayout::UNIFORM_BUFFER, DescriptorLayout::Stage::VERTEX);
	DescriptorLayout skyShaderData= instance.createLayout(0, DescriptorLayout::UNIFORM_BUFFER, DescriptorLayout::Stage::VERTEX);

	Pipeline skyShaderPipeline = instance.createGraphicsPipeline("./shader/skyVert.spv", "./shader/skyFrag.spv", {skyShaderVertex.layout, skyShaderTransform.layout, skyShaderData.layout}, window.renderPass);

	Descriptor skyShaderVertexDescriptor = genericDescriptorPool.createDescriptor(skyShaderVertex, &unitSquareBuffer, nullptr, nullptr);

	Descriptor skyShaderUVSphere = genericDescriptorPool.createDescriptor(skyShaderVertex, &sphereBuffer, nullptr, nullptr);

	struct {
		glm::mat4 transform;
	} transformData;

	Buffer transformBuffer = instance.createBuffer(sizeof(transformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Instance::BufferAccess::CPUGPU);
	Descriptor transformDesc = genericDescriptorPool.createDescriptor(skyShaderTransform, &transformBuffer, nullptr, nullptr);

	struct {
		float time;
		float rotx;
		float roty;
	} shaderData;

	DescriptorLayout layout1 =
		instance.createLayout(0, DescriptorLayout::Type::STORAGE_BUFFER, DescriptorLayout::Stage::VERTEX);

	Descriptor desc1 = genericDescriptorPool.createDescriptor(layout1, &triangleBuffer, nullptr, nullptr);

	Pipeline trianglePipeline=
		instance.createGraphicsPipeline("./shader/triangleVert.spv", "./shader/triangleFrag.spv", {layout1.layout}, window.renderPass);

	Buffer shaderBuffer = instance.createBuffer(sizeof(shaderData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, Instance::BufferAccess::CPUGPU);
	Descriptor shaderDesc = genericDescriptorPool.createDescriptor(skyShaderData, &shaderBuffer, nullptr, nullptr);

	std::cout << "Loading Assets And Textures" << '\n';

	Atlas atlas("./resources/java/assets/minecraft/textures/block/", &instance);

	Sampler sampler = instance.createSampler(VK_FILTER_NEAREST);

	Descriptor textureSamplerDescriptor = genericDescriptorPool.createDescriptor(blockShaderLayout.textureSampler, nullptr, &atlas.texture, &sampler);

	cg::Image tintTextureGrass;
	tintTextureGrass.load("./resources/java/assets/minecraft/textures/colormap/grass.png");

	cg::Image tintTextureFoliage;
	tintTextureFoliage.load("./resources/java/assets/minecraft/textures/colormap/foliage.png");

	std::vector<Block>		   blockDefs;
	std::map<std::string, unsigned int> blockNameToDef;
	std::vector<std::string>   blockList;

	/*agl::Texture blank;*/
	/*blank.setBlank();*/
	/**/
	/*agl::Font font;*/
	/*font.setup("./font/font.ttf", 24);*/

	std::cout << "Loading Config" << '\n';

	Config config;

	{
		std::fstream fs("./config", std::ios::in);
		recurse(Input(fs), config, "config");
	}

	/*window.setFPS(config.fpsCap);*/

	recurse(Output(std::cout), config, "config");

	std::cout << "Loading Block Data" << '\n';

	blockDefs.reserve(atlas.blockMap.size() + 1);
	{
		std::map<std::string, Json::Value> jsonPairs;

		for (auto &entry :
			 std::filesystem::recursive_directory_iterator("./resources/java/assets/minecraft/models/block/"))
		{
			std::fstream fs(entry.path(), std::ios::in);

			Json::Value	 root;
			Json::Reader reader;
			reader.parse(fs, root, false);

			fs.close();

			auto s = std::filesystem::path(entry).filename().string();

			jsonPairs[s.substr(0, s.length() - 5)] = root;
		}

		std::vector<float> databuf;
		databuf.reserve(128 * 128 * 4);

		for (auto &e : jsonPairs)
		{
			blockNameToDef["minecraft:" + e.first] = blockDefs.size();
			blockDefs.emplace_back(atlas, e.first, jsonPairs, tintTextureGrass, tintTextureFoliage);
		}
	}

	for (auto &e : blockNameToDef)
	{
		blockList.emplace_back(e.first.substr(10));
	}

	std::cout << "Misc Work" << '\n';

	/*agl::Rectangle blankRect;*/
	/*blankRect.setTexture(&blank);*/

	/*MCText text(blankRect);*/
	/*text.scale = 2;*/

	World world(&blockNameToDef, &blockDefs);

	Player player;

	GameState gamestate = GameState::RUNNING;

	/*CommandBox cmdBox(blankRect, text, blank, windowSize);*/

	agl::Vec<int, 3> selected;
	agl::Vec<int, 3> front;

	Listener lclis;
	Listener rclis;

	for (auto &e : player.pallete)
	{
		e = world.errorBlock;
	}

	hideCursor(window);

	WorldMesh wm(world, blockDefs);

	world.blockNameToDef = &blockNameToDef;

	bool closeThread = false;

	std::thread *thread = new std::thread(buildThread, std::ref(wm), std::ref(closeThread));

	/*cmdBox.functions = {*/
	/*	CommandFunction{"set",*/
	/*					{&blockList},*/
	/*					[&](std::vector<std::string> v) {*/
	/*						std::string &name = v[1];*/
	/**/
	/*						if (blockNameToDef.count("minecraft:" + name) != 0)*/
	/*						{*/
	/*							player.pallete[player.currentPallete] = blockNameToDef["minecraft:" + name];*/
	/*						}*/
	/**/
	/*						return;*/
	/*					}},*/
	/*	CommandFunction{"resetpos",*/
	/*					{},*/
	/*					[&](std::vector<std::string> v) {*/
	/*						player.pos = {16 * 16, 150, 16 * 16};*/
	/**/
	/*						return;*/
	/*					}},*/
	/*	CommandFunction{"togglefullscreen",*/
	/*					{},*/
	/*					[&](std::vector<std::string> v) {*/
	/*						toggleFullscreen(window.baseWindow.dpy, window.baseWindow.win);*/
	/**/
	/*						return;*/
	/*					}},*/
	/*	CommandFunction{"save",*/
	/*					{},*/
	/*					[&](std::vector<std::string> v) {*/
	/*						save(v[1], world);*/
	/**/
	/*						return;*/
	/*					}},*/
	/*	CommandFunction{"load",*/
	/*					{},*/
	/*					[&](std::vector<std::string> v) {*/
	/*						closeThread = true;*/
	/*						thread->join();*/
	/*						closeThread = false;*/
	/*						wm.clear();*/
	/*						world.loadedChunks.clear();*/
	/*						player.pos = {16 * 16, 150, 16 * 16};*/
	/*						load(v[1], world);*/
	/**/
	/*						thread = new std::thread(buildThread, std::ref(wm), std::ref(closeThread));*/
	/**/
	/*						return;*/
	/*					}},*/
	/*};*/
	/**/
	/*cmdBox.setCommands();*/

	/*{*/
	/*	int id = blockShader.getUniformLocation("textureSampler");*/
	/**/
	/*	glUniform1i(id, 0);*/
	/*}*/

	std::cout << "entering" << '\n';

	float currentFrame = 0;

	bool windowFocus = true;

	auto lastFrameTime = std::chrono::system_clock::now();
	while (!window.shouldClose())
	{
		{
			std::this_thread::sleep_until(lastFrameTime + std::chrono::milliseconds(1000 / 60));
			lastFrameTime = std::chrono::system_clock::now();
		}

		{
			/*int	   revert = 0;*/
			/*Window win;*/
			/*XGetInputFocus(window.baseWindow.dpy, &win, &revert);*/
			/**/
			/*windowFocus = win == window.baseWindow.win;*/
		}

		if (config.showFps)
		{
			static Timer t;
			t.stop();
			std::cout << "FPS: " << 1000. / (t.get<std::chrono::milliseconds>()) << '\n';
			t.start();
		}
		static int milliDiff = 0;
		int		   start	 = getMillisecond();

		glfwPollEvents();

		window.getFrameBufferSize(&windowSize.x, &windowSize.y);

		window.startDraw();

		/*{*/
			{
				transformData.transform = glm::perspectiveRH_ZO<float>(PI / 2, (float)windowSize.x / windowSize.y, 0.1, 10000) * glm::rotate(glm::mat4(1), -player.rot.x, {1, 0, 0}) * glm::scale(glm::mat4(1), {1000, 1000, 1000});

				shaderData.time = currentFrame;
				shaderData.rotx = player.rot.x;
				shaderData.roty = player.rot.y;

				transformBuffer.singleCopy(&transformData);
				shaderBuffer.singleCopy(&shaderData);

				window.draw(uvSphereData.size(), {skyShaderUVSphere, transformDesc, shaderDesc}, skyShaderPipeline);
			}

			{
				auto offset = player.pos * -1 - agl::Vec{0.f, 1.62f, 0.f};
				glm::mat4 tran = glm::translate(glm::mat4(1), {offset.x, offset.y, offset.z});

				glm::mat4 rot = glm::rotate(glm::mat4(1), -player.rot.x, {1, 0, 0}) * glm::rotate(glm::mat4(1), -player.rot.y, {0, 1, 0});// * glm::rotate<float>(glm::mat4(1), PI, {0, 0, 1});

				glm::mat4 proj = glm::perspectiveRH_ZO<float>(PI / 2, (float)windowSize.x / windowSize.y, 0.1, 10000);

				glm::mat4 mvp =  proj * rot * glm::scale(glm::mat4(1), {1, -1, 1}) * tran;

				mvpBuffer.singleCopy(&mvp);
			}

			wm.mutPos.lock();
			wm.playerChunkPos	= player.pos / 16;
			wm.playerChunkPos.y = 0;
			wm.mutPos.unlock();

			wm.draw(window, instance, blockShaderLayout.vertex, genericDescriptorPool, mvpDescriptor, textureSamplerDescriptor, blockShader);
		/*ImGui::End();*/
		/**/
		/*	glDisable(GL_DEPTH_TEST);*/
		/**/
		/*	agl::Mat4f proj;*/
		/*	agl::Mat4f trans;*/
		/*	proj.ortho(0, windowSize.x, windowSize.y, 0, 0.1, 100);*/
		/*	trans.lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});*/
		/**/
		/*	uiShader.use();*/
		/*	window.getShaderUniforms(uiShader);*/
		/*	window.updateMvp(proj * trans);*/
		/**/
		/*	if (!event.isKeyPressed(agl::Key::F1))*/
		/*	{*/
		/*		blankRect.setTextureScaling({1, 1, 1});*/
		/*		blankRect.setTextureTranslation({0, 0, 0});*/
		/*		blankRect.setTexture(&blank);*/
		/*		blankRect.setColor(agl::Color::White);*/
		/*		blankRect.setRotation({0, 0, 0});*/
		/*		blankRect.setSize({3, 3});*/
		/*		blankRect.setPosition(windowSize / 2 - blankRect.getSize() / 2);*/
		/*		window.drawShape(blankRect);*/
		/**/
		/*		blankRect.setColor(agl::Color{0, 0, 0, 127});*/
		/*		blankRect.setPosition({4, 4});*/
		/*		blankRect.setSize({200, 216});*/
		/**/
		/*		window.drawShape(blankRect);*/
		/**/
		/*		int i = 0;*/
		/**/
		/*		for (auto &e : player.pallete)*/
		/*		{*/
		/*			std::string name = blockDefs[e].name;*/
		/**/
		/*			if (i == player.currentPallete)*/
		/*			{*/
		/*				text.draw(window, std::to_string(i + 1) + ") " + name, {10, 10 + 0 + (i * 24)},*/
		/*						  agl::Color{0x3c, 0x3c, 0x00, 0xFF});*/
		/*				text.draw(window, std::to_string(i + 1) + ") " + name, {8, 8 + 0 + (i * 24)},*/
		/*						  agl::Color{0xfd, 0xfe, 0x00, 0xFF});*/
		/*			}*/
		/*			else*/
		/*			{*/
		/*				text.draw(window, std::to_string(i + 1) + ") " + name, {10, 10 + 0 + (i * 24)},*/
		/*						  agl::Color{0x34, 0x34, 0x34, 0xFF});*/
		/*				text.draw(window, std::to_string(i + 1) + ") " + name, {8, 8 + 0 + (i * 24)},*/
		/*						  agl::Color{0xDE, 0xDE, 0xDE, 0xFF});*/
		/*			}*/
		/**/
		/*			i++;*/
		/*		}*/
		/*	}*/
		/**/
		/*	if (gamestate == GameState::PAUSE)*/
		/*	{*/
		/*		blankRect.setTexture(&blank);*/
		/*		blankRect.setColor({0, 0, 0, 127});*/
		/*		blankRect.setSize(windowSize);*/
		/*		blankRect.setPosition({0, 0, 0});*/
		/*		window.drawShape(blankRect);*/
		/*	}*/
		/*	if (gamestate == GameState::CMD)*/
		/*	{*/
		/*		if (cmdBox.commit)*/
		/*		{*/
		/*			std::vector<std::string> array	= splitString(cmdBox.cmd, ' ');*/
		/*			int						 funcid = -1;*/
		/**/
		/*			if (array.size() != 0)*/
		/*			{*/
		/*				for (int i = 0; i < cmdBox.functions.size(); i++)*/
		/*				{*/
		/*					if (array[0] == cmdBox.functions[i].name)*/
		/*					{*/
		/*						funcid = i;*/
		/*						break;*/
		/*					}*/
		/*				}*/
		/*			}*/
		/**/
		/*			if (funcid != -1)*/
		/*			{*/
		/*				cmdBox.functions[funcid].func(array);*/
		/*			}*/
		/**/
		/*			cmdBox.cmd	  = "";*/
		/*			cmdBox.commit = false;*/
		/*			gamestate	  = GameState::RUNNING;*/
		/*		}*/
		/*		else*/
		/*		{*/
		/*			window.draw(cmdBox);*/
		/*		}*/
		/*	}*/
		/*}*/
		/**/
		/*glEnable(GL_DEPTH_TEST);*/

			ImGui::Begin("Player Info");
			ImGui::Text("%s", std::format("position : {} {} {}", player.pos.x, player.pos.y, player.pos.z).c_str());
			ImGui::End();

			ImGui::Begin("Log");
		
			Log::iterate([](auto&e){
				ImGui::Text("%s", std::format("[{}] - {}", e.time, e.data).c_str());
					});

			ImGui::End();

		window.endDraw();

		static int frame = 0;
		frame++;

		if(!glfwGetKey(window.window, GLFW_KEY_ESCAPE))
		{
			windowFocus = true;
		} else {
			windowFocus = false;
		}

		if (gamestate == GameState::RUNNING)
		{
			if (windowFocus)
			{
				static agl::Vec<int, 2> oldMousePos = getPointerPos(window);

				agl::Vec<int, 2> mousePos = getPointerPos(window);

				agl::Vec<int, 2> deltaPos = mousePos - oldMousePos;

				constexpr float sensitivity = .5;

				agl::Vec<float, 2> rotDelta =
					(agl::Vec<float, 3>(deltaPos.y, -deltaPos.x, 0) * 1.2 * std::pow(sensitivity * 0.6 + 0.2, 3));

				player.rot += rotDelta * PI / 180;

				if (player.rot.x > PI / 2)
				{
					player.rot.x = PI / 2;
				}
				else if (player.rot.x < -PI / 2)
				{

					player.rot.x = -PI / 2;
				}

				oldMousePos = mousePos;

/*#ifdef __linux__*/
/*				Window win = 0;*/
/*				int	   i   = 0;*/
/*				XGetInputFocus(window.baseWindow.dpy, &win, &i);*/
/*				if (win == window.baseWindow.win)*/
/*				{*/
/*					if ((mousePos - (windowSize / 2)).length() > std::min(windowSize.x / 2, windowSize.y / 2))*/
/*					{*/
/*						XWarpPointer(window.baseWindow.dpy, None, window.baseWindow.win, 0, 0, 0, 0, windowSize.x / 2,*/
/*									 windowSize.y / 2);*/
/**/
/*						oldMousePos = windowSize / 2;*/
/*					}*/
/*				}*/
/*#endif*/

/*#ifdef _WIN32*/
				{
					int focused = glfwGetWindowAttrib(window.window, GLFW_FOCUSED);

					if (focused)
					{
						if ((mousePos - (windowSize / 2)).length() > std::min(windowSize.x / 2, windowSize.y / 2))
						{
							glfwSetCursorPos(window.window, windowSize.x / 2., windowSize.y / 2.);

							oldMousePos = windowSize / 2;
						}
					}
				}
/*#endif*/
			}

			updateSelected(player, selected, front, world);

			agl::Vec<float, 3> acc;

			if (windowFocus)
			{
				if (glfwGetKey(window.window, GLFW_KEY_W))
				{
					acc.x += -sin(player.rot.y);
					acc.z += -cos(player.rot.y);
				}
				if (glfwGetKey(window.window, GLFW_KEY_A))
				{
					acc.x += -cos(player.rot.y);
					acc.z += sin(player.rot.y);
				}
				if (glfwGetKey(window.window, GLFW_KEY_S))
				{
					acc.x += sin(player.rot.y);
					acc.z += cos(player.rot.y);
				}
				if (glfwGetKey(window.window, GLFW_KEY_D))
				{
					acc.x += cos(player.rot.y);
					acc.z += -sin(player.rot.y);
				}

				if (glfwGetKey(window.window, GLFW_KEY_SPACE))
				{
					acc.y = 1;
				}

				if(glfwGetKey(window.window, GLFW_KEY_LEFT_CONTROL))
				{
					acc.y = -1;
				}

				if (glfwGetKey(window.window, GLFW_KEY_LEFT_SHIFT))
				{
					player.sprinting = true;
				}
				else
				{
					player.sprinting = false;
				}

				if (glfwGetKey(window.window, GLFW_KEY_1))
				{
					player.currentPallete = 0;
				}
				if (glfwGetKey(window.window, GLFW_KEY_2))
				{
					player.currentPallete = 1;
				}
				if (glfwGetKey(window.window, GLFW_KEY_3))
				{
					player.currentPallete = 2;
				}
				if (glfwGetKey(window.window, GLFW_KEY_4))
				{
					player.currentPallete = 3;
				}
				if (glfwGetKey(window.window, GLFW_KEY_5))
				{
					player.currentPallete = 4;
				}
				if (glfwGetKey(window.window, GLFW_KEY_6))
				{
					player.currentPallete = 5;
				}
				if (glfwGetKey(window.window, GLFW_KEY_7))
				{
					player.currentPallete = 6;
				}
				if (glfwGetKey(window.window, GLFW_KEY_8))
				{
					player.currentPallete = 7;
				}
				if (glfwGetKey(window.window, GLFW_KEY_8))
				{
					player.currentPallete = 8;
				}
				if (glfwGetKey(window.window, GLFW_KEY_9))
				{
					player.currentPallete = 9;
				}
			}

			player.grounded = false;

			movePlayer(player, acc, world);

			if (windowFocus)
			{
				lclis.update(glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_LEFT));
				rclis.update(glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_RIGHT));

				if (rclis.ls == ListenState::First && !(front == agl::Vec<int, 3>{player.pos}))
				{
					world.setBlock(front, BlockData{(unsigned int)player.pallete[player.currentPallete]});

					for (int x = -1; x < 2; x++)
					{
						for (int y = -1; y < 2; y++)
						{
							for (int z = -1; z < 2; z++)
							{
								auto global = front + agl::Vec<int, 3>{x, y, z};
								auto cpos	= global / 16;
								cpos.y		= 0;

								auto it = world.loadedChunks.find(cpos);

								if (it == world.loadedChunks.end())
								{
									continue;
								}

								auto local = global - (cpos * 16);

								it->second.blocks[local.x][local.y][local.z].update = true;
							}
						}
					}

					std::vector<agl::Vec<int, 3>> polluted;
					polluted.push_back({front.x / 16, 0, front.z / 16});

					if (front.x % 16 == 15)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z});
					}
					if (front.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x, polluted[0].y, polluted[0].z + 1});
					}
					if (front.x % 16 == 0)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z});
					}
					if (front.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x, polluted[0].y, polluted[0].z - 1});
					}
					if (front.x % 16 == 15 && front.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z + 1});
					}
					if (front.x % 16 == 0 && front.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z + 1});
					}
					if (front.x % 16 == 15 && front.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z - 1});
					}
					if (front.x % 16 == 0 && front.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z - 1});
					}

					for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
					{
						for (auto i = polluted.begin(); i != polluted.end(); i++)
						{
							if (it->pos == *i)
							{
								it->update = true;
								polluted.erase(i);
								break;
							}
						}
					}
				}
				if (lclis.ls == ListenState::First && gamestate)
				{
					world.setBlock(selected, BlockData{(unsigned int)world.air});

					for (int x = -1; x < 2; x++)
					{
						for (int y = -1; y < 2; y++)
						{
							for (int z = -1; z < 2; z++)
							{
								auto global = selected + agl::Vec<int, 3>{x, y, z};
								auto cpos	= global / 16;
								cpos.y		= 0;

								auto it = world.loadedChunks.find(cpos);

								if (it == world.loadedChunks.end())
								{
									continue;
								}

								auto local = global - (cpos * 16);

								it->second.blocks[local.x][local.y][local.z].update = true;
							}
						}
					}

					std::vector<agl::Vec<int, 3>> polluted;
					polluted.push_back({selected.x / 16, 0, selected.z / 16});

					if (selected.x % 16 == 15)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z});
					}
					if (selected.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x, polluted[0].y, polluted[0].z + 1});
					}
					if (selected.x % 16 == 0)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z});
					}
					if (selected.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x, polluted[0].y, polluted[0].z + 1});
					}
					if (selected.x % 16 == 15 && selected.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z + 1});
					}
					if (selected.x % 16 == 0 && selected.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z + 1});
					}
					if (selected.x % 16 == 15 && selected.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z - 1});
					}
					if (selected.x % 16 == 0 && selected.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z - 1});
					}

					for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
					{
						for (auto i = polluted.begin(); i != polluted.end(); i++)
						{
							if (it->pos == *i)
							{
								it->update = true;
								polluted.erase(i);
								break;
							}
						}
					}
				}

				if (glfwGetKey(window.window, GLFW_KEY_T))
				{
					gamestate = GameState::CMD;
				}
			}
		}
		else if (gamestate == GameState::CMD)
		{
			if (windowFocus)
			{
				if (glfwGetKey(window.window, GLFW_KEY_ESCAPE))
				{
					gamestate = GameState::RUNNING;
				}
				else
				{
					/*cmdBox.update(event.keybuffer);*/
				}
			}
		}
		else if (gamestate == GameState::PAUSE)
		{
		}

		/*window.setViewport(0, 0, windowSize.x, windowSize.y);*/

		agl::Vec<int, 3> chunkPos = player.pos / 16;
		chunkPos.y				  = 0;
		/*std::cout << player.pos << '\n';*/

		currentFrame++;
	}

	closeThread = true;
	thread->join();
	delete thread;

	wm.clear();

	/*font.deleteFont();*/

	/*blank.deleteTexture();*/

	tintTextureGrass.free();
	tintTextureFoliage.free();

	window.destroy();

	std::cout << "end" << '\n';

	return 0;
}
