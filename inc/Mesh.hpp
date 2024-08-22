#pragma once

#include "World.hpp"
#include <AXIS/ax.hpp>
#include <atomic>
#include <mutex>

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

		std::vector<float> posBuffer;
		std::vector<float> UVBuffer;
		std::vector<float> lightBuffer;

		ChunkMesh(World &world, std::vector<Block> &blockDefs, agl::Vec<int, 3> chunkPos);

		~ChunkMesh()
		{
			mesh.deleteData();
		}

		void draw(agl::RenderWindow &w)
		{
			if (!baked)
			{
				mesh.genBuffers(3);
				mesh.setMode(GL_TRIANGLES);
				mesh.setVertexAmount(posBuffer.size() / 3);
				mesh.setBufferData(0, &posBuffer[0], 3);
				mesh.setBufferData(1, &UVBuffer[0], 2);
				mesh.setBufferData(2, &lightBuffer[0], 3);

				posBuffer.clear();
				UVBuffer.clear();
				lightBuffer.clear();

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

		void draw(agl::RenderWindow &rw)
		{
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

#define RENDERDIST	3
#define DESTROYDIST 4

void buildThread(WorldMesh &wm, bool &closeThread);

struct BlockMap
{
		bool data[3][3][3];

		inline bool get(agl::Vec<int, 3> pos)
		{
			return data[pos.x + 1][pos.y + 1][pos.z + 1];
		}
};

template <typename T> inline bool inRange(T t, T min, T max)
{
	return (t <= max) && (t >= min);
}

unsigned int AmOcCalc(agl::Vec<int, 3> pos, agl::Vec<int, 3> norm, agl::Vec<int, 3> acc1, agl::Vec<int, 3> acc2,
					  BlockMap &map);

inline void calcAOCandExposed(BlockData &bd, agl::Vec<int, 3> pos, agl::Vec<int, 3> chunkPosBig, World &world,
							  std::vector<Block> &blockDefs, ChunkRaw &chunk)
{
	BlockMap blockMap;

#define MACRO(X, Y, Z)                                                                            \
	{                                                                                             \
		agl::Vec<int, 3> offset = agl::Vec<int, 3>{pos.x + X - 1, pos.y + Y - 1, pos.z + Z - 1};  \
		unsigned int	 id;                                                                      \
		if (inRange(offset.x, 0, 15) && inRange(offset.y, 0, 384) && inRange(offset.z, 0, 15))    \
		{                                                                                         \
			id = chunk.get(offset).type;                                                          \
		}                                                                                         \
		else                                                                                      \
		{                                                                                         \
			id = world.getBlock(offset + chunkPosBig).type;                                       \
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

	bd.exposed.nonvis = true;

	if (blockMap.data[1][2][1])
	{
		bd.exposed.nonvis = false;
		bd.exposed.up	  = true;

		bd.aoc.up.x0y0 = AmOcCalc(pos, {0, 1, 0}, {-1, 0, 0}, {0, 0, -1}, blockMap);
		bd.aoc.up.x1y0 = AmOcCalc(pos, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, blockMap);
		bd.aoc.up.x0y1 = AmOcCalc(pos, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, blockMap);
		bd.aoc.up.x1y1 = AmOcCalc(pos, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, blockMap);
	}
	else
	{
		bd.exposed.up = false;
	}

	if (blockMap.data[1][0][1])
	{
		bd.exposed.nonvis = false;
		bd.exposed.down	  = true;

		bd.aoc.down.x0y0 = AmOcCalc(pos, {0, -1, 0}, {-1, 0, 0}, {0, 0, 1}, blockMap);
		bd.aoc.down.x1y0 = AmOcCalc(pos, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, blockMap);
		bd.aoc.down.x0y1 = AmOcCalc(pos, {0, -1, 0}, {-1, 0, 0}, {0, 0, -1}, blockMap);
		bd.aoc.down.x1y1 = AmOcCalc(pos, {0, -1, 0}, {1, 0, 0}, {0, 0, -1}, blockMap);
	}
	else
	{
		bd.exposed.down = false;
	}

	// z

	if (blockMap.data[1][1][2])
	{
		bd.exposed.nonvis = false;
		bd.exposed.north  = true;

		bd.aoc.north.x0y0 = AmOcCalc(pos, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}, blockMap);
		bd.aoc.north.x1y0 = AmOcCalc(pos, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, blockMap);
		bd.aoc.north.x0y1 = AmOcCalc(pos, {0, 0, 1}, {-1, 0, 0}, {0, -1, 0}, blockMap);
		bd.aoc.north.x1y1 = AmOcCalc(pos, {0, 0, 1}, {1, 0, 0}, {0, -1, 0}, blockMap);
	}
	else
	{
		bd.exposed.north = false;
	}

	if (blockMap.data[1][1][0])
	{
		bd.exposed.nonvis = false;
		bd.exposed.south  = true;

		bd.aoc.south.x0y0 = AmOcCalc(pos, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, blockMap);
		bd.aoc.south.x1y0 = AmOcCalc(pos, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, blockMap);
		bd.aoc.south.x0y1 = AmOcCalc(pos, {0, 0, -1}, {1, 0, 0}, {0, -1, 0}, blockMap);
		bd.aoc.south.x1y1 = AmOcCalc(pos, {0, 0, -1}, {-1, 0, 0}, {0, -1, 0}, blockMap);
	}
	else
	{
		bd.exposed.south = false;
	}

	// x

	if (blockMap.data[2][1][1])
	{
		bd.exposed.nonvis = false;
		bd.exposed.east	  = true;

		bd.aoc.east.x0y0 = AmOcCalc(pos, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}, blockMap);
		bd.aoc.east.x1y0 = AmOcCalc(pos, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, blockMap);
		bd.aoc.east.x0y1 = AmOcCalc(pos, {1, 0, 0}, {0, 0, 1}, {0, -1, 0}, blockMap);
		bd.aoc.east.x1y1 = AmOcCalc(pos, {1, 0, 0}, {0, 0, -1}, {0, -1, 0}, blockMap);
	}
	else
	{
		bd.exposed.east = false;
	}

	if (blockMap.data[0][1][1])
	{
		bd.exposed.nonvis = false;
		bd.exposed.west	  = true;

		bd.aoc.west.x0y0 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, blockMap);
		bd.aoc.west.x1y0 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, blockMap);
		bd.aoc.west.x0y1 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}, blockMap);
		bd.aoc.west.x1y1 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, 1}, {0, -1, 0}, blockMap);
	}
	else
	{
		bd.exposed.west = false;
	}
}

