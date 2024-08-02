#pragma once

#include <AXIS/ax.hpp>
#include <mutex>
#include <atomic>
#include "World.hpp"

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

struct BlockMap
{
		bool data[3][3][3];

		inline bool get(agl::Vec<int, 3> pos)
		{
			return data[pos.x + 1][pos.y + 1][pos.z + 1];
		}
};

unsigned int AmOcCalc(agl::Vec<int, 3> pos, agl::Vec<int, 3> norm, agl::Vec<int, 3> acc1, agl::Vec<int, 3> acc2,
					  BlockMap &map);

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

#define RENDERDIST	8
#define DESTROYDIST 10

void buildThread(WorldMesh &wm, bool &closeThread);

template <typename T> inline bool inRange(T t, T min, T max)
{
	return (t <= max) && (t >= min);
}
