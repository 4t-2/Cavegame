#include <AXIS/ax.hpp>

#include <atomic>
#include <bitset>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <math.h>
#include <mutex>
#include <string>
#include <thread>

#include "../inc/Atlas.hpp"
#include "../inc/Block.hpp"
#include "../inc/Serializer.hpp"
#include "../inc/World.hpp"

// b5d1ff

#define GRAVACC (-.08 / 9)
#define WALKACC (1 / 3.)
#define BLCKFRC 0.6

#define BASESPEED WALKVELPERTICK

#define FOREACH(x) for (int index = 0; index < x.size(); index++)

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

struct BlockMap
{
		bool data[3][3][3];

		inline bool get(agl::Vec<int, 3> pos)
		{
			return data[pos.x + 1][pos.y + 1][pos.z + 1];
		}
};

unsigned int AmOcCalc(agl::Vec<int, 3> pos, agl::Vec<int, 3> norm, agl::Vec<int, 3> acc1, agl::Vec<int, 3> acc2,
					  BlockMap &map)
{
	bool cornerTouch = !map.get(norm + acc1 + acc2);
	bool lineTouch	 = !map.get(norm + acc1);
	bool oppo		 = !map.get(norm + acc2);

	if (lineTouch && oppo)
	{
		return 3;
	}
	else if ((lineTouch && cornerTouch) || (oppo && cornerTouch))
	{
		return 2;
	}
	else if (lineTouch || oppo || cornerTouch)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

class Player
{
	public:
		agl::Vec<float, 3> pos = {16 * 16, 150, 16 * 16};
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

void hideCursor(agl::RenderWindow &window)
{
#ifdef __linux__
	Cursor		invisibleCursor;
	Pixmap		bitmapNoData;
	XColor		black;
	static char noData[] = {0, 0, 0, 0, 0, 0, 0, 0};
	black.red = black.green = black.blue = 0;

	bitmapNoData	= XCreateBitmapFromData(window.baseWindow.dpy, window.baseWindow.win, noData, 8, 8);
	invisibleCursor = XCreatePixmapCursor(window.baseWindow.dpy, bitmapNoData, bitmapNoData, &black, &black, 0, 0);
	XDefineCursor(window.baseWindow.dpy, window.baseWindow.win, invisibleCursor);
	XFreeCursor(window.baseWindow.dpy, invisibleCursor);
	XFreePixmap(window.baseWindow.dpy, bitmapNoData);
#endif

#ifdef _WIN32
	glfwSetInputMode(window.baseWindow.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif
}

void movePlayer(Player &player, agl::Vec<float, 3> acc)
{
	player.vel.x *= BLCKFRC * 0.91;
	player.vel.z *= BLCKFRC * 0.91;

	acc *= WALKACC * 0.98;

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

	player.vel += acc * 0.1;

	player.pos += player.vel;

	player.vel.y *= 0.98;
	player.vel.y += GRAVACC;
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

void correctPosition(Player &player, World &world)
{
	for (int x = player.pos.x - 0.3; x < player.pos.x + 0.3; x++)
	{
		for (int z = player.pos.z - 0.3; z < player.pos.z + 0.3; z++)
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

	for (int x = player.pos.x - 0.3; x < player.pos.x + 0.3; x++)
	{
		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			for (int z = player.pos.z - 0.3; z < player.pos.z + 0.3; z++)
			{
				if (world.getAtPos({x, y, z}))
				{
					agl::Vec<float, 2> center = {x + .5, y + .5, z + .5};
					if (abs(center.x - player.pos.x) < abs(center.z - player.pos.z))
					{
						if (center.z > player.pos.z)
						{
							player.vel.z = 0;
							player.pos.z = center.z - 0.3 - .5;
						}
						else if (center.z < player.pos.z)
						{
							player.vel.z = 0;
							player.pos.z = center.z + 0.3 + .5;
						}
					}
					else
					{
						if (center.x > player.pos.x)
						{
							player.vel.x = 0;
							player.pos.x = center.x - 0.3 - .5;
						}
						else if (center.x < player.pos.x)
						{
							player.vel.x = 0;
							player.pos.x = center.x + 0.3 + .5;
						}
					}
				}
			}
		}
	}
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

class CommandBox : public agl::Drawable
{
	public:
		agl::Shape	 &rect;
		agl::Text	 &text;
		agl::Texture &blank;

		std::string cmd = "";

		std::vector<Block> &blocks;

		agl::Vec<int, 2> &winSize;

		bool commit = false;

		bool &focused;

		int pallete = 1;

		CommandBox(agl::Shape &rect, agl::Text &text, agl::Texture &blank, std::vector<Block> &blocks,
				   agl::Vec<int, 2> &winSize, bool &focused)
			: rect(rect), text(text), blank(blank), blocks(blocks), winSize(winSize), focused(focused)
		{
		}

		void update(std::string buffer)
		{
			cmd += buffer;

			auto safeSub = [](std::string &str, int pos) -> std::string {
				if (pos > str.length())
				{
					return "";
				}
				else
				{
					return str.substr(pos);
				}
			};
			auto removeChar = [&](std::string &str, int i) { str = str.substr(0, i) + safeSub(str, i + 1); };

			for (int i = 0; i < cmd.size(); i++)
			{
				if (cmd[i] == 8)
				{
					removeChar(cmd, i);
					i--;
					if (i >= 0)
					{
						removeChar(cmd, i);
						i--;
					}
				}
				else if (cmd[i] == '\r')
				{
					commit = true;
					cmd	   = cmd.substr(0, i);
					break;
				}
				else if (cmd[i] < 32 || cmd[i] == 127)
				{
					removeChar(cmd, i);
					i--;
				}
			}
		}

		void drawFunction(agl::RenderWindow &win) override
		{
			rect.setTexture(&blank);
			rect.setColor(agl::Color::Black);
			rect.setPosition({0, 0, 0});
			rect.setRotation({0, 0, 0});
			rect.setSize({winSize.x, text.getHeight() + 10, 0});

			win.drawShape(rect);

			text.setText(" > " + cmd);
			text.setPosition({0, 0, 0});
			text.setColor(agl::Color::White);

			win.drawText(text);

			int offset = text.getHeight() + 10;

			for (unsigned int i = 0; i < blocks.size(); i++)
			{
				auto &b = blocks[i];

				if (b.name.substr(0, cmd.length()) == cmd)
				{
					rect.setPosition({0, offset, 0});
					text.setPosition({0, offset, 0});
					text.setText(b.name);
					text.setColor(agl::Color::Gray);

					win.drawShape(rect);
					win.drawText(text);

					offset += text.getHeight() + 10;
				}

				if (b.name.substr(0, cmd.size()) == cmd && commit)
				{
					commit	= false;
					pallete = i;
					focused = true;
					cmd		= "";
					break;
				}
			}

			commit = false;
		}
};

class Timer
{
	private:
#ifdef _WIN32
		std::chrono::time_point<std::chrono::steady_clock> begin;
		std::chrono::time_point<std::chrono::steady_clock> end;
#endif
#ifdef __linux__
		std::chrono::time_point<std::chrono::high_resolution_clock> begin;
		std::chrono::time_point<std::chrono::high_resolution_clock> end;
#endif
	public:
		void start()
		{
			begin = std::chrono::high_resolution_clock::now();
		}
		void stop()
		{
			end = std::chrono::high_resolution_clock::now();
		}
		template <typename T = std::chrono::milliseconds> long long get()
		{
			return std::chrono::duration_cast<T>(end - begin).count();
		}
};

struct ChunkMesh
{
		agl::Vec<int, 3> pos;
		agl::GLPrimative mesh;
		bool			 baked	= false;
		bool			 update = false;

		std::vector<float> posList;

		ChunkMesh(World &world, std::vector<Block> &blockDefs, agl::Vec<int, 3> chunkPos);

		~ChunkMesh()
		{
			mesh.deleteData();
		}

		void draw(agl::RenderWindow &w)
		{
			if (!baked)
			{
				mesh.genBuffers(1);
				mesh.setMode(GL_LINES_ADJACENCY);
				mesh.setVertexAmount(posList.size() / 4);
				mesh.setBufferData(0, &posList[0], 4);

				posList.clear();

				baked = true;
			}

			w.drawPrimative(mesh);
		}
};

class WorldMesh
{
	public:
		agl::GLPrimative glp;

		std::list<ChunkMesh> mesh;

		std::mutex		 mutPos;
		agl::Vec<int, 3> playerChunkPos = {0, 0, 0};

		// true - queue full, build thread halt, draw thread add
		// false - queue empty, build thread makes, draw thread draws
		std::atomic<bool>							hasDiffs = false;
		std::vector<std::list<ChunkMesh>::iterator> toDestroy;
		std::list<ChunkMesh>						toAdd;

		std::vector<Block> &blockDefs;
		World			   &world;

		WorldMesh(World &world, std::vector<Block> &blockDefs) : blockDefs(blockDefs), world(world)
		{
		}

		void draw(agl::RenderWindow &rw, Player &p)
		{
			mutPos.lock();
			playerChunkPos	 = p.pos / 16;
			playerChunkPos.y = 0;
			mutPos.unlock();

			Timer t;
			t.start();
			for (auto it = mesh.begin(); it != mesh.end(); it++)
			{
				it->draw(rw);
			}
			t.stop();
			// std::cout << "took " << t.get<std::chrono::milliseconds>() << '\n';

			if (hasDiffs)
			{
				for (auto &e : toDestroy)
				{
					mesh.erase(e);
				}

				toDestroy.clear();

				mesh.splice(mesh.end(), toAdd);

				hasDiffs = false;
			}
		}
};

#define RENDERDIST	8
#define DESTROYDIST 10

void buildThread(WorldMesh &wm, bool &closeThread)
{
	agl::Vec<int, 3> playerChunkPos;
	bool			 changesMade;

	auto buildChunk = [&](int x, int y) {
		agl::Vec<int, 3> cursor = {x, 0, y};

		if (cursor.length() > RENDERDIST)
		{
			return 0;
		}

		cursor += playerChunkPos;

		if (wm.world.loadedChunks.count(cursor) == 0)
		{
			wm.world.createChunk(cursor);
		}

		for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
		{
			if (it->pos == cursor && !it->update)
			{
				goto skip;
			}
		}

		wm.toAdd.emplace_back(wm.world, wm.blockDefs, cursor);
		changesMade = true;

		return 1;

	skip:;

		return 0;
	};

	auto spiral = [](int X, int Y, auto func) {
		int x, y, dx, dy;
		x = y = dx = 0;
		dy		   = -1;
		int t	   = std::max(X, Y);
		int maxI   = t * t;
		for (int i = 0; i < maxI; i++)
		{
			if ((-X / 2 <= x) && (x <= X / 2) && (-Y / 2 <= y) && (y <= Y / 2))
			{
				if (func(x, y))
				{
					return;
				}
			}
			if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
			{
				t  = dx;
				dx = -dy;
				dy = t;
			}
			x += dx;
			y += dy;
		}
	};

	while (!closeThread)
	{
		if (!wm.hasDiffs)
		{
			wm.mutPos.lock();
			playerChunkPos = wm.playerChunkPos;
			wm.mutPos.unlock();

			changesMade = false;

			for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
			{
				if ((it->pos - playerChunkPos).length() > DESTROYDIST)
				{
					wm.toDestroy.push_back(it);
					changesMade = true;
				}

				if (it->update)
				{
					wm.toDestroy.push_back(it);
					changesMade = true;
					buildChunk(it->pos.x - playerChunkPos.x, it->pos.z - playerChunkPos.z);

					goto createSkip;
				}
			}

			spiral(RENDERDIST * 2, RENDERDIST * 2, buildChunk);

		createSkip:;

			if (changesMade)
			{
				wm.hasDiffs = true;
			}
		}
	}
	std::cout << "build thread end" << '\n';
}

template <typename T> inline bool inRange(T t, T min, T max)
{
	return (t <= max) && (t >= min);
}

ChunkMesh::ChunkMesh(World &world, std::vector<Block> &blockDefs, agl::Vec<int, 3> chunkPos) : pos(chunkPos)
{
	auto			 start		 = std::chrono::high_resolution_clock::now();
	agl::Vec<int, 3> chunkPosBig = chunkPos * 16;

	ChunkRaw &chunk = world.loadedChunks[chunkPos];

	posList.reserve(32768);

	BlockMap blockMap;

	if (!world.loadedChunks.count(chunkPos + agl::Vec<int, 3>{1, 0, 0}))
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{1, 0, 0});
	}
	if (!world.loadedChunks.count(chunkPos + agl::Vec<int, 3>{-1, 0, 0}))
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{-1, 0, 0});
	}
	if (!world.loadedChunks.count(chunkPos + agl::Vec<int, 3>{0, 0, 1}))
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{0, 0, 1});
	}
	if (!world.loadedChunks.count(chunkPos + agl::Vec<int, 3>{0, 0, -1}))
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{0, 0, -1});
	}

	for (int x = 0; x < 16; x++)
	{
		for (int y = MINHEIGHT; y < MAXHEIGHT; y++)
		{
			for (int z = 0; z < 16; z++)
			{
				agl::Vec<float, 3> pos = agl::Vec<int, 3>{x, y, z};

				auto &block = chunk.at(pos);

				if (block.type == world.air)
				{
					continue;
				}

#define MACRO(X, Y, Z)                                                                            \
	{                                                                                             \
		agl::Vec<int, 3> offset = agl::Vec<int, 3>{x + X - 1, y + Y - 1, z + Z - 1};              \
		unsigned int	 id;                                                                      \
		if (inRange(offset.x, 0, 15) && inRange(offset.y, 0, 385) && inRange(offset.z, 0, 15))    \
		{                                                                                         \
			id = chunk.blocks[offset.x][offset.y][offset.z].type;                                 \
		}                                                                                         \
		else                                                                                      \
		{                                                                                         \
			id = world.get(chunkPosBig + offset);                                                 \
		}                                                                                         \
		blockMap.data[X][Y][Z] = (id == world.air || id == world.leaves || !blockDefs[id].solid); \
	}

#define MACRO2(L)       \
	{                   \
		MACRO(0, L, 0); \
		MACRO(1, L, 0); \
		MACRO(2, L, 0); \
		MACRO(0, L, 1); \
		MACRO(1, L, 1); \
		MACRO(2, L, 1); \
		MACRO(0, L, 2); \
		MACRO(1, L, 2); \
		MACRO(2, L, 2); \
	}

				MACRO2(0);
				MACRO2(1);
				MACRO2(2);

#undef MACRO2
#undef MACRO
				block.exposed.nonvis = true;

				if (blockMap.data[1][2][1])
				{
					block.exposed.nonvis = false;
					block.exposed.up	 = true;

					block.aoc.up.x0y0 = AmOcCalc(pos + chunkPosBig, {0, 1, 0}, {-1, 0, 0}, {0, 0, -1}, blockMap);
					block.aoc.up.x1y0 = AmOcCalc(pos + chunkPosBig, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, blockMap);
					block.aoc.up.x0y1 = AmOcCalc(pos + chunkPosBig, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, blockMap);
					block.aoc.up.x1y1 = AmOcCalc(pos + chunkPosBig, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, blockMap);
				}
				else
				{
					block.exposed.up = false;
				}

				if (blockMap.data[1][0][1])
				{
					block.exposed.nonvis = false;
					block.exposed.down	 = true;

					block.aoc.down.x0y0 = AmOcCalc(pos + chunkPosBig, {0, -1, 0}, {-1, 0, 0}, {0, 0, 1}, blockMap);
					block.aoc.down.x1y0 = AmOcCalc(pos + chunkPosBig, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, blockMap);
					block.aoc.down.x0y1 = AmOcCalc(pos + chunkPosBig, {0, -1, 0}, {-1, 0, 0}, {0, 0, -1}, blockMap);
					block.aoc.down.x1y1 = AmOcCalc(pos + chunkPosBig, {0, -1, 0}, {1, 0, 0}, {0, 0, -1}, blockMap);
				}
				else
				{
					block.exposed.down = false;
				}

				// z

				if (blockMap.data[1][1][2])
				{
					block.exposed.nonvis = false;
					block.exposed.north	 = true;

					block.aoc.north.x0y0 = AmOcCalc(pos + chunkPosBig, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}, blockMap);
					block.aoc.north.x1y0 = AmOcCalc(pos + chunkPosBig, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, blockMap);
					block.aoc.north.x0y1 = AmOcCalc(pos + chunkPosBig, {0, 0, 1}, {-1, 0, 0}, {0, -1, 0}, blockMap);
					block.aoc.north.x1y1 = AmOcCalc(pos + chunkPosBig, {0, 0, 1}, {1, 0, 0}, {0, -1, 0}, blockMap);
				}
				else
				{
					block.exposed.north = false;
				}

				if (blockMap.data[1][1][0])
				{
					block.exposed.nonvis = false;
					block.exposed.south	 = true;

					block.aoc.south.x0y0 = AmOcCalc(pos + chunkPosBig, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, blockMap);
					block.aoc.south.x1y0 = AmOcCalc(pos + chunkPosBig, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, blockMap);
					block.aoc.south.x0y1 = AmOcCalc(pos + chunkPosBig, {0, 0, -1}, {1, 0, 0}, {0, -1, 0}, blockMap);
					block.aoc.south.x1y1 = AmOcCalc(pos + chunkPosBig, {0, 0, -1}, {-1, 0, 0}, {0, -1, 0}, blockMap);
				}
				else
				{
					block.exposed.south = false;
				}

				// x

				if (blockMap.data[2][1][1])
				{
					block.exposed.nonvis = false;
					block.exposed.east	 = true;

					block.aoc.east.x0y0 = AmOcCalc(pos + chunkPosBig, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}, blockMap);
					block.aoc.east.x1y0 = AmOcCalc(pos + chunkPosBig, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, blockMap);
					block.aoc.east.x0y1 = AmOcCalc(pos + chunkPosBig, {1, 0, 0}, {0, 0, 1}, {0, -1, 0}, blockMap);
					block.aoc.east.x1y1 = AmOcCalc(pos + chunkPosBig, {1, 0, 0}, {0, 0, -1}, {0, -1, 0}, blockMap);
				}
				else
				{
					block.exposed.east = false;
				}

				if (blockMap.data[0][1][1])
				{
					block.exposed.nonvis = false;
					block.exposed.west	 = true;

					block.aoc.west.x0y0 = AmOcCalc(pos + chunkPosBig, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, blockMap);
					block.aoc.west.x1y0 = AmOcCalc(pos + chunkPosBig, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, blockMap);
					block.aoc.west.x0y1 = AmOcCalc(pos + chunkPosBig, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}, blockMap);
					block.aoc.west.x1y1 = AmOcCalc(pos + chunkPosBig, {-1, 0, 0}, {0, 0, 1}, {0, -1, 0}, blockMap);
				}
				else
				{
					block.exposed.west = false;
				}

				if (!block.exposed.nonvis)
				{
					for (auto &e : blockDefs[block.type].elements)
					{
						agl::Mat4f scale;
						scale.scale(e.size);
						agl::Mat4f offset;
						offset.translate(e.offset + pos + (chunkPos * 16));
						agl::Mat4f mat = offset * scale;

						posList.push_back(mat.data[0][0]);
						posList.push_back(mat.data[0][1]);
						posList.push_back(mat.data[0][2]);
						posList.push_back(mat.data[1][0]);

						posList.push_back(mat.data[1][1]);
						posList.push_back(mat.data[1][2]);
						posList.push_back(mat.data[2][0]);
						posList.push_back(mat.data[2][1]);

						posList.push_back(mat.data[2][2]);
						posList.push_back(mat.data[3][0]);
						posList.push_back(mat.data[3][1]);
						posList.push_back(mat.data[3][2]);

#define COOLERSHIT(dir1, dir2)                    \
	{                                             \
		unsigned int buf = 0;                     \
                                                  \
		buf |= block.aoc.dir1.x0y0 << 0;          \
		buf |= block.aoc.dir1.x0y1 << 2;          \
		buf |= block.aoc.dir1.x1y0 << 4;          \
		buf |= block.aoc.dir1.x1y1 << 6;          \
		buf |= (long long)e.dir1.tintImage << 8;  \
                                                  \
		buf |= block.aoc.dir2.x0y0 << 16;         \
		buf |= block.aoc.dir2.x0y1 << 18;         \
		buf |= block.aoc.dir2.x1y0 << 20;         \
		buf |= block.aoc.dir2.x1y1 << 22;         \
		buf |= (long long)e.dir2.tintImage << 24; \
                                                  \
		posList.push_back(*(float *)(&buf));      \
	}

						COOLERSHIT(up, down);	  // 3 x
						COOLERSHIT(south, north); // 3 y
						COOLERSHIT(west, east);	  // 3 z

						posList.push_back(*(float *)&e.id); // 3 w

						// posList.push_back(0); // 4 x
						// posList.push_back(0); // 4 y
						// posList.push_back(0); // 4 z
						// posList.push_back(0); // 4 w
						// posList.push_back(0); // 5 x
						// posList.push_back(0); // 5 y
						// posList.push_back(0); // 5 z
						// posList.push_back(0); // 5 w
					}
				}
			}
		}
	}
	auto end = std::chrono::high_resolution_clock::now();

	// std::cout << "build took " <<
	// std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
	// << '\n';
}

int main()
{
	printf("Starting AGL\n");

#ifdef _WIN32
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#endif

	agl::RenderWindow window;
	window.setup({1920, 1080}, "CaveGame");
	// window.setClearColor({0x78, 0xA7, 0xFF});
	window.setClearColor(agl::Color::Black);

	agl::Vec<int, 2> windowSize;

	window.GLEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);

	glDepthFunc(GL_LEQUAL);

	window.setSwapInterval(1);

	agl::Event event;
	event.setWindow(window);

	std::cout << "Shader Compilation" << '\n';

	ax::Program worldShader(ax::Shader("./shader/worldVert.glsl", GL_VERTEX_SHADER),
							ax::Shader("./shader/worldGeom.glsl", GL_GEOMETRY_SHADER),
							ax::Shader("./shader/worldFrag.glsl", GL_FRAGMENT_SHADER));

	ax::Program uiShader(ax::Shader("./shader/frag.glsl", GL_FRAGMENT_SHADER),
						 ax::Shader("./shader/uivert.glsl", GL_VERTEX_SHADER));

	ax::Program skyShader(ax::Shader("./shader/skyFrag.glsl", GL_FRAGMENT_SHADER),
						  ax::Shader("./shader/skyVert.glsl", GL_VERTEX_SHADER));

	std::cout << "Loading Assets And Textures" << '\n';

	Atlas atlas("./resources/java/assets/minecraft/textures/block/");

	Image tintTextureGrass;
	tintTextureGrass.load("./resources/java/assets/minecraft/textures/colormap/grass.png");

	Image tintTextureFoliage;
	tintTextureFoliage.load("./resources/java/assets/minecraft/textures/colormap/foliage.png");

	std::vector<Block>		   blockDefs;
	std::map<std::string, int> blockNameToDef;

	agl::Texture elementDataTexture;

	agl::Texture blank;
	blank.setBlank();

	agl::Font font;
	font.setup("./font/font.ttf", 24);

	std::cout << "Loading Config" << '\n';

	Config config;

	{
		std::fstream fs("./config", std::ios::in);
		recurse(Input(fs), config, "config");
	}

	window.setFPS(config.fpsCap);

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

			for (auto &e : blockDefs.back().elements)
			{
				for (auto i : e.elementDataArray)
				{
					// 1 face = 1/2 pixel
					// full = 3 pixels
					databuf.push_back(*(float *)&i);
					// databuf.push_back(2);
				}
			}
		}

		databuf.resize(128 * 128 * 4);

		elementDataTexture.genTexture();
		elementDataTexture.bind(elementDataTexture);

		// 128*128
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 128, 128, 0, GL_RGBA, GL_FLOAT, &databuf[0]);

		elementDataTexture.useNearestFiltering();
	}

	std::cout << "Misc Work" << '\n';

	agl::Text text;
	text.setFont(&font);
	text.setScale(1);

	agl::Rectangle blankRect;
	blankRect.setTexture(&blank);

	World world;
	world.setBasics(blockDefs);

	bool focused = true;

	CommandBox cmdBox(blankRect, text, blank, blockDefs, windowSize, focused);

	cmdBox.pallete = world.cobblestone;

	agl::Vec<int, 3> selected;
	agl::Vec<int, 3> front;

	Listener lclis;
	Listener rclis;

	Player player;

	hideCursor(window);

	WorldMesh wm(world, blockDefs);

	bool closeThread = false;

	std::thread thread(buildThread, std::ref(wm), std::ref(closeThread));

	{
		worldShader.use();
		auto id = worldShader.getUniformLocation("elementDataSampler");

		glUniform1i(id, 1);

		id = worldShader.getUniformLocation("textureSampler");

		glUniform1i(id, 0);
	}

	std::cout << "entering" << '\n';

	float currentFrame = 0;

	while (!event.windowClose())
	{
		if (config.showFps)
		{
			static Timer t;
			t.stop();
			std::cout << "FPS: " << 1000. / (t.get<std::chrono::milliseconds>()) << '\n';
			t.start();
		}
		static int milliDiff = 0;
		int		   start	 = getMillisecond();

		event.poll();

		windowSize = window.getState().size;

		window.clear();

		{
			agl::Mat4f proj;
			agl::Mat4f trans;
			proj.ortho(0, windowSize.x, windowSize.y, 0, 0.1, 100);
			trans.lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});

			skyShader.use();
			window.getShaderUniforms(skyShader);
			window.updateMvp(proj * trans);

			int id = skyShader.getUniformLocation("time");
			skyShader.setUniform(id, currentFrame);
			id = skyShader.getUniformLocation("rotx");
			skyShader.setUniform(id, player.rot.x);
			id = skyShader.getUniformLocation("roty");
			skyShader.setUniform(id, player.rot.y);

			blankRect.setSize(windowSize);
			blankRect.setPosition({0, 0, 0});

			glDisable(GL_DEPTH_TEST);

			window.drawShape(blankRect);

			glEnable(GL_DEPTH_TEST);
		}

		worldShader.use();
		window.getShaderUniforms(worldShader);
		{
			agl::Mat<float, 4> tran;
			tran.translate(player.pos * -1 - agl::Vec{0.f, 1.8f, 0.f});

			agl::Mat<float, 4> rot;
			rot.rotate({agl::radianToDegree(player.rot.x), agl::radianToDegree(player.rot.y),
						agl::radianToDegree(player.rot.z)});

			agl::Mat<float, 4> proj;
			proj.perspective(PI / 2, (float)windowSize.x / windowSize.y, 0.1, 10000);

			window.updateMvp(proj * rot * tran);

			int id = worldShader.getUniformLocation("time");
			worldShader.setUniform(id, currentFrame);
			id = worldShader.getUniformLocation("rotx");
			worldShader.setUniform(id, player.rot.x);
			id = worldShader.getUniformLocation("roty");
			worldShader.setUniform(id, player.rot.y);
		}

		glActiveTexture(GL_TEXTURE0 + 1);
		agl::Texture::bind(elementDataTexture);
		glActiveTexture(GL_TEXTURE0 + 0);
		agl::Texture::bind(atlas.texture);
		wm.draw(window, player);

		glDisable(GL_DEPTH_TEST);

		if (!event.isKeyPressed(agl::Key::F1))
		{
			agl::Mat4f proj;
			agl::Mat4f trans;
			proj.ortho(0, windowSize.x, windowSize.y, 0, 0.1, 100);
			trans.lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});

			uiShader.use();
			window.getShaderUniforms(uiShader);
			window.updateMvp(proj * trans);

			blankRect.setTextureScaling({1, 1, 1});
			blankRect.setTextureTranslation({0, 0, 0});
			blankRect.setTexture(&blank);
			blankRect.setColor(agl::Color::Red);
			blankRect.setRotation({0, 0, 0});
			blankRect.setSize({3, 3});
			blankRect.setPosition(windowSize / 2 - blankRect.getSize() / 2);
			window.drawShape(blankRect);

			if (!focused)
			{
				window.draw(cmdBox);
			}
		}

		glEnable(GL_DEPTH_TEST);

		window.display();

		static int frame = 0;
		frame++;

		if (focused)
		{
			static agl::Vec<int, 2> oldMousePos = event.getPointerWindowPosition();

			agl::Vec<int, 2> mousePos = event.getPointerWindowPosition();

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

#ifdef __linux__
			Window win = 0;
			int	   i   = 0;
			XGetInputFocus(window.baseWindow.dpy, &win, &i);
			if (win == window.baseWindow.win)
			{
				if ((mousePos - (windowSize / 2)).length() > std::min(windowSize.x / 2, windowSize.y / 2))
				{
					XWarpPointer(window.baseWindow.dpy, None, window.baseWindow.win, 0, 0, 0, 0, windowSize.x / 2,
								 windowSize.y / 2);

					oldMousePos = windowSize / 2;
				}
			}
#endif

#ifdef _WIN32
			{
				int focused = glfwGetWindowAttrib(window.baseWindow.window, GLFW_FOCUSED);

				if (focused)
				{
					if ((mousePos - (windowSize / 2)).length() > std::min(windowSize.x / 2, windowSize.y / 2))
					{
						glfwSetCursorPos(window.baseWindow.window, windowSize.x / 2, windowSize.y / 2);

						oldMousePos = windowSize / 2;
					}
				}
			}
#endif

			updateSelected(player, selected, front, world);

			agl::Vec<float, 3> acc;
			if (event.isKeyPressed(agl::Key::W))
			{
				acc.x += -sin(player.rot.y);
				acc.z += -cos(player.rot.y);
			}
			if (event.isKeyPressed(agl::Key::A))
			{
				acc.x += -cos(player.rot.y);
				acc.z += sin(player.rot.y);
			}
			if (event.isKeyPressed(agl::Key::S))
			{
				acc.x += sin(player.rot.y);
				acc.z += cos(player.rot.y);
			}
			if (event.isKeyPressed(agl::Key::D))
			{
				acc.x += cos(player.rot.y);
				acc.z += -sin(player.rot.y);
			}

			if (event.isKeyPressed(agl::Key::Space) && player.grounded)
			{
				player.vel.y = 0.48 / 3;
			}

			if (event.isKeyPressed(agl::Key::LeftShift))
			{
				player.sprinting = true;
			}
			else
			{
				player.sprinting = false;
			}

			player.grounded = false;

			movePlayer(player, acc);
			correctPosition(player, world);

			lclis.update(event.isPointerButtonPressed(agl::Button::Left));
			rclis.update(event.isPointerButtonPressed(agl::Button::Right));

			if (rclis.ls == ListenState::First && !(front == player.pos))
			{
				auto block	= world.getBlock(front);
				block->type = cmdBox.pallete;

				for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
				{
					if (it->pos == agl::Vec{front.x / 16, 0, front.z / 16})
					{
						it->update = true;
						break;
					}
				}
			}
			if (lclis.ls == ListenState::First && focused)
			{
				auto block	= world.getBlock(selected);
				block->type = world.air;

				for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
				{
					if (it->pos == agl::Vec{selected.x / 16, 0, selected.z / 16})
					{
						it->update = true;
						break;
					}
				}
			}

			if (event.isKeyPressed(agl::Key::T))
			{
				focused = false;
			}
		}
		else
		{
			if (event.isKeyPressed(agl::Key::Escape))
			{
				focused = true;
			}
			else
			{
				cmdBox.update(event.keybuffer);
			}
		}

		window.setViewport(0, 0, windowSize.x, windowSize.y);

		agl::Vec<int, 3> chunkPos = player.pos / 16;
		chunkPos.y				  = 0;
		// std::cout << chunkPos << '\n';

		currentFrame++;
	}

	font.deleteFont();

	blank.deleteTexture();

	tintTextureGrass.free();
	tintTextureFoliage.free();

	window.close();

	closeThread = true;
	thread.join();

	return 0;
}
