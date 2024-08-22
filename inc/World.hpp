#pragma once

#include "../inc/Block.hpp"
#include <filesystem>
#include <list>

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
				return v.x * v.y * v.z;
			}
	};
} // namespace std

#define ACCESSNODE(x, y, z) node[(x * 4) + (y * 2) + z]

template <typename T> class OcTree
{
	public:
		int size = 0;

		T value;

		OcTree<T> *node = nullptr;

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
			node = nullptr;
		}

		void setup(T &startVal, int size)
		{
			node	   = nullptr;
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
					node = new OcTree<T>[8];

					node[0].setup(value, size / 2);
					node[1].setup(value, size / 2);
					node[2].setup(value, size / 2);
					node[3].setup(value, size / 2);
					node[4].setup(value, size / 2);
					node[5].setup(value, size / 2);
					node[6].setup(value, size / 2);
					node[7].setup(value, size / 2);
				}

				int x = (pos.x + 1) > (size / 2);
				int y = (pos.y + 1) > (size / 2);
				int z = (pos.z + 1) > (size / 2);

				ACCESSNODE(x, y, z).setValue(
					{pos.x - ((size / 2) * -x), pos.y - ((size / 2) * -y), pos.z - ((size / 2) * -z)}, newValue);

				if (node[0].node == nullptr && node[1].node == nullptr && node[2].node == nullptr &&
					node[3].node == nullptr && node[4].node == nullptr && node[5].node == nullptr &&
					node[6].node == nullptr && node[7].node == nullptr)
				{
					if (node[0].value == node[1].value && node[1].value == node[2].value &&
						node[2].value == node[3].value && node[3].value == node[4].value &&
						node[4].value == node[5].value && node[5].value == node[6].value &&
						node[6].value == node[7].value)
					{
						value = ACCESSNODE(0, 0, 0).value;
						delete[] node;
						node = nullptr;
					}
				}
			}
		}

		T getValue(agl::Vec<int, 3> pos)
		{
			std::cout << size << " " << node << '\n';
			if (node == nullptr)
			{
				std::cout << "null" << '\n';
				return value;
			}
			else
			{
				int x = (pos.x + 1) > (size / 2);
				int y = (pos.y + 1) > (size / 2);
				int z = (pos.z + 1) > (size / 2);

				return ACCESSNODE(x, y, z).getValue(
					{pos.x - ((size / 2) * x), pos.y - ((size / 2) * y), pos.z - ((size / 2) * z)});
			}
		}
};

#undef ACCESSNODE

class SegStack
{
	public:
		/*bool
		 * same[16];*/
		std::array<std::array<std::array<BlockData, 16>, 16>, 16> buffer;

		void setup(BlockData &bd, int size)
		{
		}
		void setValue(agl::Vec<int, 3> pos, BlockData &bd)
		{
			buffer[pos.x][pos.y][pos.z] = bd;

			/*	bool good = true;*/
			/*	;*/
			/**/
			/*	for (int x = 0; x < 16; x++)*/
			/*	{*/
			/*		for (int z = 0; z < 16; z++)*/
			/*		{*/
			/*			if (!(buffer[x][pos.y][z] == bd))*/
			/*			{*/
			/*				good = false;*/
			/*				goto nope;*/
			/*			}*/
			/*		}*/
			/*	}*/
			/*nope:;*/
			/*	same[pos.y] = good;*/
		}
		BlockData getValue(agl::Vec<int, 3> pos)
		{
			return buffer[pos.x][pos.y][pos.z];
		}
};

struct ChunkRaw
{
		bool	  meshedBefore = false;
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

			loadedChunks[chunkPos].set(pos - agl::Vec<int, 3>{chunkPos.x * 16, chunkPos.y * 384, chunkPos.z * 16},
									   data);
		}
};
