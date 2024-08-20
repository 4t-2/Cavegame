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

template <typename T> class OcTree
{
	public:
		int size;

		T value;

		OcTree<T> ***node;

		OcTree()
		{
		}

		OcTree(T &startVal, int size)
		{
			setup(startVal, size);
		}

		~OcTree()
		{
			delete[] node;
		}

		void setup(T &startVal, int size)
		{
			value	   = startVal;
			this->size = size;
		}

		void setValue(agl::Vec<int, 3> pos, T &newValue)
		{
			if (size == 1)
			{
				value = newValue;
			}
			else
			{
				if (node == nullptr)
				{
					node = new OcTree[2][2][2];

					node[0][0][0].setup(value, size / 2);
					node[0][0][1].setup(value, size / 2);
					node[0][1][0].setup(value, size / 2);
					node[0][1][1].setup(value, size / 2);
					node[1][0][0].setup(value, size / 2);
					node[1][0][1].setup(value, size / 2);
					node[1][1][0].setup(value, size / 2);
					node[1][1][1].setup(value, size / 2);
				}

				int x = (pos.x + 1) <= (size / 2);
				int y = (pos.y + 1) <= (size / 2);
				int z = (pos.z + 1) <= (size / 2);

				T value = node[x][y][z]->setValue(
					{pos.x - ((size / 2) * -x), pos.y - ((size / 2) * -y), pos.z - ((size / 2) * -z)}, newValue);

				if (node[0][0][0] == node[0][0][1] && node[0][0][1] == node[0][1][0] &&
					node[0][1][0] == node[0][1][1] && node[0][1][1] == node[1][0][0] &&
					node[1][0][0] == node[1][0][1] && node[1][0][1] == node[1][1][0] && node[1][1][0] == node[1][1][1])
				{
					value = node;
					delete[] node;
				}
			}
		}

		T getValue(agl::Vec<int, 3> pos)
		{
			if (node != nullptr)
			{
				return value;
			}
			else
			{
				int x = (pos.x + 1) <= (size / 2);
				int y = (pos.y + 1) <= (size / 2);
				int z = (pos.z + 1) <= (size / 2);

				return node[x][y][z]->getValue(
					{pos.x - ((size / 2) * -x), pos.y - ((size / 2) * -y), pos.z - ((size / 2) * -z)});
			}
		}
};

struct ChunkRaw
{
		BlockData blocks[16][384][16];

		BlockData &get(agl::Vec<int, 3> v)
		{
			return blocks[v.x][v.y][v.z];
		}

		void set(agl::Vec<int, 3> v, BlockData &block)
		{
			blocks[v.x][v.y][v.z] = block;
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
			chunkPos.y = pos.y / 384;
			chunkPos.z = pos.z >> 4;

			if (loadedChunks.count(chunkPos) == 0 || pos.y < 0)
			{
				return false;
			}

			return loadedChunks[chunkPos]
					   .get(pos - agl::Vec<int, 3>{chunkPos.x * 16, chunkPos.y * 384, chunkPos.z * 16})
					   .type != air;
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

			return loadedChunks[chunkPos].get(pos -
											  agl::Vec<int, 3>{chunkPos.x * 16, chunkPos.y * 384, chunkPos.z * 16});
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

			loadedChunks[chunkPos].set(pos - agl::Vec<int, 3>{chunkPos.x * 16, chunkPos.y * 384, chunkPos.z * 16}, data);
		}
};
