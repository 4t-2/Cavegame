#pragma once

#include "../inc/Block.hpp"
#include <filesystem>
#include <list>
#include <unordered_map>

#define MAXHEIGHT 384
#define MINHEIGHT 0

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
				return (v.x*100) + (v.y*10000) + v.z;
			}
	};
} // namespace std

struct ChunkRaw
{
		BlockData blocks[16][384][16];

		ChunkRaw()
		{
		}

		BlockData get(agl::Vec<int, 3> v)
		{
			return blocks[v.x][v.y][v.z];
		}

		void set(agl::Vec<int, 3> v, BlockData block)
		{
			blocks[v.x][v.y][v.z] = block;
		}
};

class World
{
	public:
		std::unordered_map<agl::Vec<int, 3>, ChunkRaw> loadedChunks;
		agl::Vec<int, 3>							   size;
		std::vector<Block>							  *blockDefs;
		unsigned int								   air;
		unsigned int errorBlock;
		std::map<std::string, unsigned int> *blockNameToDef;

		World(std::map<std::string, unsigned int> *blockNameToDef, std::vector<Block> *blockDefs)
		{
			this->blockNameToDef = blockNameToDef;
			this->blockDefs = blockDefs;

			air = (*blockNameToDef)["minecraft:air"];
			errorBlock= (*blockNameToDef)["minecraft:stone"];
		}

		bool getAtPos(agl::Vec<int, 3> pos)
		{
			auto b = getBlock(pos);

			if(pos.x < 0)
			{
			Log::addLog(std::format("{} {} {}", pos.x, pos.y, pos.z));
			Log::addLog(std::format("{} {}", b.type, (*blockDefs)[b.type].name));
			}

			return (*blockDefs)[b.type].solid;
		}

		void generateRandom(std::map<std::string, int> &strToId);

		void createChunk(agl::Vec<int, 3> pos);

		BlockData getBlock(agl::Vec<int, 3> pos)
		{
			agl::Vec<int, 3> chunkPos;
			chunkPos.x = pos.x >> 4;
			chunkPos.y = pos.y / 384;
			chunkPos.z = pos.z >> 4;

			if (loadedChunks.count(chunkPos) == 0 || pos.y < 0)
			{
				return BlockData{air};
			}

			pos.x = pos.x < 0 ? 16 + pos.x : pos.x;
			pos.y = pos.y < 0 ? 16 + pos.y : pos.y;

			return loadedChunks[chunkPos].get({pos.x % 16, pos.y, pos.z % 16});
		}

		void setBlock(agl::Vec<int, 3> pos, BlockData data)
		{
			agl::Vec<int, 3> chunkPos;
			chunkPos.x = pos.x >> 4;
			chunkPos.y = pos.y / 384;
			chunkPos.z = pos.z >> 4;

			if (loadedChunks.count(chunkPos) == 0 || pos.y < 0)
			{
				return;
			}

			loadedChunks[chunkPos].set(pos - agl::Vec<int, 3>{chunkPos.x * 16, chunkPos.y * 384, chunkPos.z * 16},
									   data);
		}
};
