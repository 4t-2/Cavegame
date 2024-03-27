#include "Atlas.hpp"
#include <AGL/agl.hpp>
#include <json/json.h>

class Block
{
	public:
		std::string name;

		agl::Vec<int, 2> up;
		agl::Vec<int, 2> down;
		agl::Vec<int, 2> east;
		agl::Vec<int, 2> north;
		agl::Vec<int, 2> south;
		agl::Vec<int, 2> west;

		Block(Json::Value &json, Atlas &atlas, std::string name)
		{
			auto &textures = json["textures"];
			auto &parent   = json["parent"];
			if (parent == "minecraft:block/cube_all")
			{
				up	  = atlas.blockMap[textures["all"].asString()];
				down  = atlas.blockMap[textures["all"].asString()];
				east  = atlas.blockMap[textures["all"].asString()];
				north = atlas.blockMap[textures["all"].asString()];
				south = atlas.blockMap[textures["all"].asString()];
				west  = atlas.blockMap[textures["all"].asString()];
			}
			else if (parent == "minecraft:block/cube")
			{
				up	  = atlas.blockMap[textures["up"].asString()];
				down  = atlas.blockMap[textures["down"].asString()];
				east  = atlas.blockMap[textures["east"].asString()];
				north = atlas.blockMap[textures["north"].asString()];
				south = atlas.blockMap[textures["south"].asString()];
				west  = atlas.blockMap[textures["west"].asString()];
			}
			else if (parent == "minecraft:block/orientable")
			{
				up	  = atlas.blockMap[textures["top"].asString()];
				down  = atlas.blockMap[textures["top"].asString()];
				east  = atlas.blockMap[textures["side"].asString()];
				north = atlas.blockMap[textures["front"].asString()];
				south = atlas.blockMap[textures["side"].asString()];
				west  = atlas.blockMap[textures["side"].asString()];
			}

			this->name = name;
		}

		Block(std::string name)
		{

		}
};
