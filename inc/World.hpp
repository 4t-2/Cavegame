#pragma once

#include "../inc/Block.hpp"
#include <MI/enkimi.h>
#include <MI/miniz.h>
#include <filesystem>
#include <list>

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

namespace std {
    template <>
    struct hash<agl::Vec<int, 3>> {
        
        size_t operator() (const agl::Vec<int, 3>& v) const { return v.x * v.y * v.z; }
    };
}

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
		agl::Vec<int, 3>					 size;
		unsigned int						 air;
		unsigned int						 cobblestone;
		unsigned int						 leaves;

		World() : loadedChunks()
		{
		}

		void setBasics(std::vector<Block> blocks)
		{
			for (int i = 0; i < blocks.size(); i++)
			{
				if (blocks[i].name == "air")
				{
					air = i;
					break;
				}
			}
			for (int i = 0; i < blocks.size(); i++)
			{
				if (blocks[i].name == "cobblestone")
				{
					cobblestone = i;
					break;
				}
			}
			for (int i = 0; i < blocks.size(); i++)
			{
				if (blocks[i].name == "oak_leaves")
				{
					leaves = i;
					break;
				}
			}
		}

		bool getAtPos(agl::Vec<int, 3> pos)
		{
			agl::Vec<int, 3> chunkPos;
			chunkPos.x = pos.x >> 4;
			chunkPos.y = 0;
			chunkPos.z = pos.z >> 4;

			if (loadedChunks.count(chunkPos) == 0)
			{
				return false;
			}

			return loadedChunks[chunkPos].at(pos - (chunkPos * 16)).type != air;
		}

		void loadFromFile(std::string dir, std::map<std::string, int> &strToId);

		unsigned int &get(agl::Vec<int, 3> pos)
		{
			agl::Vec<int, 3> chunkPos;
			chunkPos.x = pos.x >> 4;
			chunkPos.y = 0;
			chunkPos.z = pos.z >> 4;

			return loadedChunks[chunkPos].at(pos - (chunkPos * 16)).type;
		}
};
