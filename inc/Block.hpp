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

static void cubeRender(agl::RenderWindow &window, agl::Rectangle &blankRect, Atlas &atlas, agl::Vec<int, 3> pos,
					   Layout layout)
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
			top	  = atlas.blockMap[textures["top"].asString()];
		}

		void render(agl::RenderWindow &window, agl::Rectangle &blankRect, Atlas &atlas, agl::Vec<int, 3> pos)
		{
			cubeRender(window, blankRect, atlas, pos, {top, top, side, front, side, side});
		}
};

struct Face
{
		agl::Vec<float, 2> uv	  = {0, 0};
		agl::Vec<float, 2> size	  = {0, 0};
		bool			   exists = false;
};

struct Element
{
		Face up;
		Face down;
		Face east;
		Face north;
		Face south;
		Face west;

		agl::Vec<float, 3> size;
		agl::Vec<float, 3> offset;

		Element(Json::Value &val, std::map<std::string, agl::Vec<int, 2>> &texHash, agl::Vec<int, 2> atlasSize)
		{
			agl::Vec<float, 3> from;
			agl::Vec<float, 3> to;

			from.x = (float)val["from"][0].asInt() / 16;
			from.y = (float)val["from"][1].asInt() / 16;
			from.z = (float)val["from"][2].asInt() / 16;

			to.x = (float)val["to"][0].asInt() / 16;
			to.y = (float)val["to"][1].asInt() / 16;
			to.z = (float)val["to"][2].asInt() / 16;

			offset = from;
			size   = to - from;

#define COOLSHIT(dir)                                                                                            \
	if (val["faces"].isMember(#dir))                                                                             \
	{                                                                                                            \
		auto &v	   = val["faces"][#dir];                                                                         \
		dir.exists = true;                                                                                       \
		dir.uv	   = texHash[v["texture"].asString()];                                                           \
		dir.uv.x /= atlasSize.x;                                                                                 \
		dir.uv.y /= atlasSize.y;                                                                                 \
		dir.uv.x += (float)v["uv"].get(Json::ArrayIndex(0), 0).asInt() / atlasSize.x;                            \
		dir.uv.y += (float)v["uv"].get(Json::ArrayIndex(1), 0).asInt() / atlasSize.y;                            \
		dir.size.x = v["uv"].get(Json::ArrayIndex(2), 16).asInt() - v["uv"].get(Json::ArrayIndex(0), 0).asInt(); \
		dir.size.y = v["uv"].get(Json::ArrayIndex(3), 16).asInt() - v["uv"].get(Json::ArrayIndex(1), 0).asInt(); \
		dir.size.x /= atlasSize.x;                                                                               \
		dir.size.y /= atlasSize.y;                                                                               \
	}

			COOLSHIT(up)
			COOLSHIT(down)
			COOLSHIT(north)
			COOLSHIT(south)
			COOLSHIT(east)
			COOLSHIT(west)
		}

		void render(agl::RenderWindow &window, agl::Rectangle &blankRect, agl::Vec<int, 3> pos)
		{
			// y+
			if (up.exists)
			{
				blankRect.setTextureTranslation(up.uv);
				blankRect.setTextureScaling(up.size);
				blankRect.setSize({size.x, size.z, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x, pos.y + size.y, pos.z} + offset);
				blankRect.setRotation({90, 0, 0});
				window.drawShape(blankRect);
			}

			// y-
			if (down.exists)
			{
				blankRect.setTextureTranslation(down.uv);
				blankRect.setTextureScaling(down.size);
				blankRect.setSize({size.x, -size.z, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x, pos.y, pos.z + 1} + offset);
				blankRect.setRotation({90, 0, 0});
				window.drawShape(blankRect);
			}

			if (south.exists)
			{
				// z-
				blankRect.setTextureTranslation(south.uv);
				blankRect.setTextureScaling(south.size);
				blankRect.setSize({-size.x, -size.y, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x + size.x, pos.y + size.y, pos.z} + offset);
				blankRect.setRotation({0, 0, 0});
				window.drawShape(blankRect);
			}

			// z+
			if (north.exists)
			{
				blankRect.setTextureTranslation(north.uv);
				blankRect.setTextureScaling(north.size);
				blankRect.setSize({size.x, -size.y, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x, pos.y + size.y, pos.z + size.z} + offset);
				blankRect.setRotation({0, 0, 0});
				window.drawShape(blankRect);
			}

			// x-
			if (west.exists)
			{
				blankRect.setTextureTranslation(west.uv);
				blankRect.setTextureScaling(west.size);
				blankRect.setSize({size.z, -size.y, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x, pos.y + size.y, pos.z} + offset);
				blankRect.setRotation({0, 90, 0});
				window.drawShape(blankRect);
			}

			// x+
			if (east.exists)
			{
				blankRect.setTextureTranslation(east.uv);
				blankRect.setTextureScaling(east.size);
				blankRect.setSize({-size.z, -size.y, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x + size.x, pos.y + size.y, pos.z + size.z} + offset);
				blankRect.setRotation({0, 90, 0});
				window.drawShape(blankRect);
			}
		}
};

class Block
{
	public:
		std::string name;

		std::vector<Element> elements;

		Block(Atlas &atlas, std::string name, std::map<std::string, Json::Value> &jsonPairs)
		{
            std::cout << "Registering " + name << '\n';

			this->name = name;

			std::vector<Json::Value> inherit;
			inherit.emplace_back(jsonPairs[name]);

			Json::Value expanded;

			while (inherit.back().isMember("parent"))
			{
				std::string cleaned = inherit.back()["parent"].asString();

				if (cleaned[0] == 'm')
				{
					cleaned = cleaned.substr(16);
				}
				else
				{
					cleaned = cleaned.substr(6);
				}

				inherit.emplace_back(jsonPairs[cleaned]);
			}

			for (int i = inherit.size() - 1; i >= 0; i--)
			{
				for (auto &key : inherit[i].getMemberNames())
				{
					if (key != "textures")
					{
						expanded[key] = inherit[i][key];
					}
					else
					{
						for (auto &key : inherit[i]["textures"].getMemberNames())
						{
							expanded["textures"][key] = inherit[i]["textures"][key];
						}
					}
				}
			}

			for (auto it = expanded["textures"].begin(); it != expanded["textures"].end(); it++)
			{
				if (it->asString()[0] == '#')
				{
					*it = expanded["textures"][it->asString().substr(1)].asString();
				}
			}

			expanded.removeMember("parent");

			std::map<std::string, agl::Vec<int, 2>> texHash;

			for (auto &key : expanded["textures"].getMemberNames())
			{
				std::string cleaned = (expanded["textures"])[key].asString();
				if (cleaned[0] == 'm')
				{
					cleaned = cleaned.substr(10);
				}
				texHash["#" + key] = atlas.blockMap[cleaned];
			}

			elements.reserve(expanded["elements"].size());

			for (auto &val : expanded["elements"])
			{
				elements.emplace_back(val, texHash, atlas.size);
			}
		}

		void render(agl::RenderWindow &window, agl::Rectangle &blankRect, Atlas &atlas, agl::Vec<int, 3> pos)
		{
			for (auto &e : elements)
			{
				e.render(window, blankRect, pos);
			}
		}

		~Block()
		{
			// dynamicDo([](auto pd) { delete pd; });
		}
};
