#pragma once

#include "../inc/Block.hpp"
#include <filesystem>
#include <list>

#define MAXHEIGHT 180
#define MINHEIGHT 100

// class ChunkGrid
// {
// 	public:
// 		std::unordered_map<std::pair<int, int>,
// std::vector<std::vector<std::vector<unsigned int>>>> blocks;
//
// 		unsigned int getAtPos(agl::Vec<int, 3> pos)
// 		{
// 			agl::Vec<int, 2> chunkPos = pos / 16;
//
// 			chunkPos *= 16;
//
// 			return blocks[std::pair(chunkPos.x, chunkPos.y)][pos.x %
// 16][pos.y % 16][pos.z % 16];
// 		}
// };

namespace std
{
	template <> struct hash<agl::Vec<int, 3>>
	{

			size_t operator()(const agl::Vec<int, 3> &v) const
			{
				return v.x * v.y * v.z;
			}
	};
} // namespace std

struct ChunkRaw
{
		BlockData blocks[16][385][16];

		BlockData &at(agl::Vec<int, 3> v)
		{
			return blocks[v.x][v.y][v.z];
		}
};

class World
{
	public:
		std::unordered_map<agl::Vec<int, 3>, ChunkRaw> loadedChunks;
		agl::Vec<int, 3>							   size;
		unsigned int								   air;
		unsigned int								   cobblestone;
		unsigned int								   leaves;
		unsigned int								   blue_wool;
		unsigned int								   grass;
		unsigned int								   snow;
		unsigned int								   dirt;
		unsigned int								   stone;
		unsigned int								   sand;

		World() : loadedChunks()
		{
		}

		void setBasics(std::vector<Block> blocks)
		{
			for (int i = 0; i < blocks.size(); i++)
			{
				if (blocks[i].name == "cobblestone")
				{
					cobblestone = i;
				}
				if (blocks[i].name == "air")
				{
					air = i;
				}
				if (blocks[i].name == "oak_leaves")
				{
					leaves = i;
				}
				if (blocks[i].name == "blue_wool")
				{
					blue_wool = i;
				}
				if (blocks[i].name == "dirt")
				{
					dirt = i;
				}
				if (blocks[i].name == "grass_block")
				{
					grass = i;
				}
				if (blocks[i].name == "stone")
				{
					stone = i;
				}
				if (blocks[i].name == "snow_block")
				{
					snow = i;
				}
				if (blocks[i].name == "sand")
				{
					sand = i;
				}
			}
		}

		bool getAtPos(agl::Vec<int, 3> pos)
		{
			agl::Vec<int, 3> chunkPos;
			chunkPos.x = pos.x >> 4;
			chunkPos.y = pos.y / 385;
			chunkPos.z = pos.z >> 4;

			if (loadedChunks.count(chunkPos) == 0 || pos.y < 0)
			{
				return false;
			}

			return loadedChunks[chunkPos]
					   .at(pos - agl::Vec<int, 3>{chunkPos.x * 16, chunkPos.y * 385, chunkPos.z * 16})
					   .type != air;
		}

		void generateRandom(std::map<std::string, int> &strToId);

		void createChunk(agl::Vec<int, 3> pos);

		unsigned int &get(agl::Vec<int, 3> pos)
		{
			agl::Vec<int, 3> chunkPos;
			chunkPos.x = pos.x >> 4;
			chunkPos.y = pos.y / 385;
			chunkPos.z = pos.z >> 4;

			if (loadedChunks.count(chunkPos) == 0 || pos.y < 0)
			{
				return air;
			}

			return loadedChunks[chunkPos]
				.at(pos - agl::Vec<int, 3>{chunkPos.x * 16, chunkPos.y * 385, chunkPos.z * 16})
				.type;
		}

		BlockData *getBlock(agl::Vec<int, 3> pos)
		{
			agl::Vec<int, 3> chunkPos;
			chunkPos.x = pos.x >> 4;
			chunkPos.y = pos.y / 385;
			chunkPos.z = pos.z >> 4;

			if (loadedChunks.count(chunkPos) == 0 || pos.y < 0)
			{
				return nullptr;
			}

			return &loadedChunks[chunkPos].at(pos -
											  agl::Vec<int, 3>{chunkPos.x * 16, chunkPos.y * 385, chunkPos.z * 16});
		}
};
