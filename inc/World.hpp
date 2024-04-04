#pragma once

#include "../inc/Block.hpp"
#include <AGL/agl.hpp>
#include <MI/enkimi.h>
#include <MI/miniz.h>
#include <filesystem>

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

class World
{
	public:
		std::vector<std::vector<std::vector<unsigned int>>> blocks;
		agl::Vec<int, 3>									size;
		unsigned int										air;
		unsigned int										cobblestone;

		World(agl::Vec<int, 3> size = {16*8, 385, 16*8}) : size(size)
		{
			blocks.resize(size.x);

			for (auto &b : blocks)
			{
				b.resize(size.y);
				for (auto &b : b)
				{
					b.resize(size.z);
				}
			}
		}

		void setBasics(std::vector<Block> blocks)
		{
			for (int i = 0; i < blocks.size(); i++)
			{
				if (blocks[i].name == "air")
				{
					air = i;
				}
			}
			for (int i = 0; i < blocks.size(); i++)
			{
				if (blocks[i].name == "cobblestone")
				{
					cobblestone = i;
				}
			}

			for (auto &x : this->blocks)
			{
				for (auto &x : x)
				{
					for (auto &x : x)
					{
						x = air;
					}
				}
			}
		}

		bool getAtPos(agl::Vec<int, 3> pos)
		{
			if (pos.x >= size.x || pos.x < 0 || pos.y >= size.y || pos.y < 0 || pos.z >= size.z || pos.z < 0)
			{
				return false;
			}
			return blocks.at(pos.x).at(pos.y).at(pos.z) != air;
		}

		void loadFromFile(std::string dir, std::map<std::string, int> &strToId);

		unsigned int &get(agl::Vec<int, 3> pos)
		{
			return blocks[pos.x][pos.y][pos.z];
		}
};
