#include "Atlas.hpp"
#include <AGL/agl.hpp>
#include <json/json.h>

class Block;

enum ParentType
{
	cube_all,
	cube,
	orientable,
	null
};

struct Layout
{
		agl::Vec<int, 2> up;
		agl::Vec<int, 2> down;
		agl::Vec<int, 2> east;
		agl::Vec<int, 2> north;
		agl::Vec<int, 2> south;
		agl::Vec<int, 2> west;
};

static void cubeRender(agl::RenderWindow &window, agl::Rectangle &blankRect, Atlas &atlas, agl::Vec<int, 3> pos, Layout layout)
{
	// y+
	blankRect.setTextureTranslation({layout.up.x / (float)atlas.size.x, layout.up.y / (float)atlas.size.y});
	blankRect.setSize({1, 1, 1});
	blankRect.setPosition({pos.x, pos.y + 1, pos.z});
	blankRect.setRotation({90, 0, 0});
	window.drawShape(blankRect);

	// y-
	blankRect.setTextureTranslation({layout.down.x / (float)atlas.size.x, layout.down.y / (float)atlas.size.y});
	blankRect.setSize({1, -1, 1});
	blankRect.setPosition({pos.x, pos.y, pos.z + 1});
	blankRect.setRotation({90, 0, 0});
	window.drawShape(blankRect);

	// z-
	blankRect.setTextureTranslation({layout.south.x / (float)atlas.size.x, layout.south.y / (float)atlas.size.y});
	blankRect.setSize({-1, -1, 1});
	blankRect.setPosition({pos.x + 1, pos.y + 1, pos.z});
	blankRect.setRotation({0, 0, 0});
	window.drawShape(blankRect);

	// z+
	blankRect.setTextureTranslation({layout.north.x / (float)atlas.size.x, layout.north.y / (float)atlas.size.y});
	blankRect.setSize({1, -1, 1});
	blankRect.setPosition({pos.x, pos.y + 1, pos.z + 1});
	blankRect.setRotation({0, 0, 0});
	window.drawShape(blankRect);

	// x-
	blankRect.setTextureTranslation({layout.west.x / (float)atlas.size.x, layout.west.y / (float)atlas.size.y});
	blankRect.setSize({-1, -1, 1});
	blankRect.setPosition({pos.x + 1, pos.y + 1, pos.z + 1});
	blankRect.setRotation({0, 90, 0});
	window.drawShape(blankRect);

	// x+
	blankRect.setTextureTranslation({layout.east.x / (float)atlas.size.x, layout.east.y / (float)atlas.size.y});
	blankRect.setSize({1, -1, 1});
	blankRect.setPosition({pos.x, pos.y + 1, pos.z});
	blankRect.setRotation({0, 90, 0});
	window.drawShape(blankRect);
}

struct Cube_all
{
		agl::Vec<int, 2> all;

		Cube_all(Json::Value &textures, Atlas &atlas)
		{
			all = atlas.blockMap[textures["all"].asString()];
		}

		void render(agl::RenderWindow &window, agl::Rectangle &blankRect, Atlas &atlas, agl::Vec<int, 3> pos)
		{
			cubeRender(window, blankRect, atlas, pos, {all, all, all, all, all, all});
		}
};

struct Cube
{
		agl::Vec<int, 2> up;
		agl::Vec<int, 2> down;
		agl::Vec<int, 2> east;
		agl::Vec<int, 2> north;
		agl::Vec<int, 2> south;
		agl::Vec<int, 2> west;

		Cube(Json::Value &textures, Atlas &atlas)
		{
			up	  = atlas.blockMap[textures["up"].asString()];
			down  = atlas.blockMap[textures["down"].asString()];
			east  = atlas.blockMap[textures["east"].asString()];
			north = atlas.blockMap[textures["north"].asString()];
			south = atlas.blockMap[textures["south"].asString()];
			west  = atlas.blockMap[textures["west"].asString()];
		}

		void render(agl::RenderWindow &window, agl::Rectangle &blankRect, Atlas &atlas, agl::Vec<int, 3> pos)
		{
			cubeRender(window, blankRect, atlas, pos, {up, down, east, north, south, west});
		}
};

struct Orientable
{
		agl::Vec<int, 2> front;
		agl::Vec<int, 2> side;
		agl::Vec<int, 2> top;

		Orientable(Json::Value &textures, Atlas &atlas)
		{
			front = atlas.blockMap[textures["front"].asString()];
			side  = atlas.blockMap[textures["side"].asString()];
			top	  = atlas.blockMap[textures["up"].asString()];
		}

		void render(agl::RenderWindow &window, agl::Rectangle &blankRect, Atlas &atlas, agl::Vec<int, 3> pos)
		{
			cubeRender(window, blankRect, atlas, pos, {top, top, side, front, side, side});
		}
};

class Block
{
	public:
		std::string name;

		ParentType parentType = ParentType::null;
		void	  *parentData = nullptr;

		Block(Json::Value &json, Atlas &atlas, std::string name)
		{
			auto &textures = json["textures"];
			auto &parent   = json["parent"];

			if (parent == "minecraft:block/cube_all")
			{
				parentType = cube_all;
				parentData = new Cube_all(textures, atlas);
			}
			else if (parent == "minecraft:block/cube")
			{
				parentType = cube;
				parentData = new Cube(textures, atlas);
			}
			else if (parent == "minecraft:block/orientable")
			{
				parentType = orientable;
				parentData = new Orientable(textures, atlas);
			}

			this->name = name;
		}

		Block(std::string name)
		{
			this->name = name;
		}

		template <typename Func> void dynamicDo(Func func)
		{
			switch (parentType)
			{
				case cube_all:
					func((Cube_all *)parentData);
					break;
				case cube:
					func((Cube *)parentData);
					break;
				case orientable:
					func((Orientable *)parentData);
					break;
				case null:
					break;
			}
		}

		void render(agl::RenderWindow &window, agl::Rectangle &blankRect, Atlas &atlas, agl::Vec<int, 3> pos)
		{
			dynamicDo([&](auto pd) { pd->render(window, blankRect, atlas, pos); });
		}

		~Block()
		{
			// dynamicDo([](auto pd) { delete pd; });
		}
};
