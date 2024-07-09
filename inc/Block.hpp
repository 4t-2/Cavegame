#pragma once

#include "Atlas.hpp"
#include <bitset>
#include <json/json.h>

struct Covered
{
		bool up		= true;
		bool down	= true;
		bool north	= true;
		bool east	= true;
		bool south	= true;
		bool west	= true;
		bool nonvis = true;
};

struct AmOcCacheCoords
{
		unsigned char x0y0 = 0;
		unsigned char x0y1 = 0;
		unsigned char x1y0 = 0;
		unsigned char x1y1 = 0;
};

struct AmOcCache
{
		AmOcCacheCoords up;
		AmOcCacheCoords down;
		AmOcCacheCoords north;
		AmOcCacheCoords east;
		AmOcCacheCoords south;
		AmOcCacheCoords west;
};

struct BlockData
{
		bool		 needUpdate = true;
		unsigned int type;

		Covered	  exposed;
		AmOcCache aoc;
};

struct Grid3
{
		std::vector<std::vector<std::vector<BlockData>>> &data;
		unsigned int									  air;

		int exists(agl::Vec<int, 3> pos)
		{
			if (pos.x >= data.size() || pos.x < 0 || pos.y >= data[0].size() || pos.y < 0 ||
				pos.z >= data[0][0].size() || pos.z < 0)
			{
				return false;
			}
			return (data[pos.x][pos.y][pos.z].type != air);
		}
};

struct Face
{
		agl::Vec<int, 2> uv		   = {0, 0};
		agl::Vec<int, 2> size	   = {0, 0};
		bool			 exists	   = false;
		int tintImage = 0;
		bool			 cull	   = false;
};

struct AOUnfiforms
{
		int x0y0;
		int x1y0;
		int x0y1;
		int x1y1;
};

struct Element
{
		int extract(int buf, int size, int start)
		{
			return (((1 << size) - 1) & (buf >> start));
		}

		agl::Vec<int, 2> idToSample(int id)
		{
			int x = extract(id, 7, 0);
			int y = extract(id, 7, 7);

			return {x, y};
		}

		Face up;
		Face down;
		Face east;
		Face north;
		Face south;
		Face west;

		agl::Vec<float, 3> size;
		agl::Vec<float, 3> offset;

		int						  id = 0;
		std::vector<unsigned int> elementDataArray;

		Element(Json::Value &val, std::map<std::string, agl::Vec<int, 2>> &texHash, agl::Vec<int, 2> atlasSize,
				Image &tintGrass, Image &tintFoliage, std::string &name)
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

			static int maxElementId = 0;
			id						= maxElementId;
			maxElementId++;

#define COOLSHIT(dir)                                                                                            \
	if (val["faces"].isMember(#dir))                                                                             \
	{                                                                                                            \
		auto &v	   = val["faces"][#dir];                                                                         \
		dir.exists = true;                                                                                       \
		dir.uv	   = texHash[v["texture"].asString()];                                                           \
		dir.uv.x += (float)v["uv"].get(Json::ArrayIndex(0), 0).asInt();                                          \
		dir.uv.y += (float)v["uv"].get(Json::ArrayIndex(1), 0).asInt();                                          \
		dir.size.x = v["uv"].get(Json::ArrayIndex(2), 16).asInt() - v["uv"].get(Json::ArrayIndex(0), 0).asInt(); \
		dir.size.y = v["uv"].get(Json::ArrayIndex(3), 16).asInt() - v["uv"].get(Json::ArrayIndex(1), 0).asInt(); \
		if (v.isMember("tintindex"))                                                                             \
		{                                                                                                        \
			if (name.find("grass") != std::string::npos)                                                         \
			{                                                                                                    \
				dir.tintImage = 1;                                                                      \
			}                                                                                                    \
			else                                                                                                 \
			{                                                                                                    \
				dir.tintImage = 2;                                                                      \
			}                                                                                                    \
		}                                                                                                        \
		if (v.isMember("cullface"))                                                                              \
		{                                                                                                        \
			dir.cull = true;                                                                                     \
		}                                                                                                        \
		else                                                                                                     \
		{                                                                                                        \
			dir.cull = false;                                                                                    \
		}                                                                                                        \
		{                                                                                                        \
			unsigned int buf1 = 0;                                                                               \
			unsigned int buf2 = 0;                                                                               \
                                                                                                                 \
			buf1 |= 1 << 31;                                                                                     \
			buf1 |= dir.uv.x << 0;                                                                               \
			buf1 |= dir.uv.y << 16;                                                                              \
                                                                                                                 \
			buf2 |= (dir.size.x - 1) << 0;                                                                       \
			buf2 |= (dir.size.y - 1) << 16;                                                                      \
                                                                                                                 \
			elementDataArray.push_back(buf1);                                                                    \
			elementDataArray.push_back(buf2);                                                                    \
		}                                                                                                        \
	}                                                                                                            \
	else                                                                                                         \
	{                                                                                                            \
		elementDataArray.push_back(0);                                                                           \
		elementDataArray.push_back(0);                                                                           \
	}

			// 10 bytes each
			COOLSHIT(up)
			COOLSHIT(down)
			COOLSHIT(north)
			COOLSHIT(south)
			COOLSHIT(east)
			COOLSHIT(west)
		}
};

class Block
{
	public:
		std::string name;

		std::vector<Element> elements;

		Block(Atlas &atlas, std::string name, std::map<std::string, Json::Value> &jsonPairs, Image &tintGrass,
			  Image &tintFoliage)
		{
			this->name = name;

			std::vector<Json::Value> inherit;
			inherit.emplace_back(jsonPairs[name]);

			Json::Value expanded;

			// std::cout << name << '\n';

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
				elements.emplace_back(val, texHash, atlas.size, tintGrass, tintFoliage, name);
			}
		}

		void render(agl::RenderWindow &window, agl::Shape &blankRect, agl::Vec<int, 3> pos, int id, AOUnfiforms aou,
					BlockData &bd)
		{
		}

		~Block()
		{
			// dynamicDo([](auto pd) { delete pd; });
		}
};
