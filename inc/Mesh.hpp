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

		void clear()
		{
			mesh.clear();
			toDestroy.clear();
			toAdd.clear();
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

#define RENDERDIST	6
#define DESTROYDIST 7

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
							  std::vector<Block> &blockDefs, ChunkRaw *chunkMap[3][3]);
