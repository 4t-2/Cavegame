#pragma once

#include "Atlas.hpp"
#include <bitset>

struct Exposed
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
		unsigned int type;
		AmOcCache	 aoc;
		Exposed		 exposed;
		bool		 update = true;
		/**/
		/*bool operator==(const BlockData bd) const*/
		/*{*/
		/*	return type == bd.type;*/
		/*}*/
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
		Image			*tintImage = nullptr;
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

		Element(Json::Value &val, std::map<std::string, agl::Vec<int, 2>> &texHash, agl::Vec<int, 2> atlasSize,
				Image &tintGrass, Image &tintFoliage, std::string &name, bool &solid)
		{
			agl::Vec<float, 3> from;
			agl::Vec<float, 3> to;

			from.x = (float)val["from"][0].asInt() / 16;
			from.y = (float)val["from"][1].asInt() / 16;
			from.z = (float)val["from"][2].asInt() / 16;

			to.x = (float)val["to"][0].asInt() / 16;
			to.y = (float)val["to"][1].asInt() / 16;
			to.z = (float)val["to"][2].asInt() / 16;

			if (from == agl::Vec<float, 3>{0, 0, 0} && to == agl::Vec<float, 3>{1, 1, 1})
			{
				solid = true;
			}

			offset = from;
			size   = to - from;

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
				dir.tintImage = &tintGrass;                                                                      \
			}                                                                                                    \
			else                                                                                                 \
			{                                                                                                    \
				dir.tintImage = &tintFoliage;                                                                    \
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
	}

			// 10 bytes each
			COOLSHIT(up)
			COOLSHIT(down)
			COOLSHIT(north)
			COOLSHIT(south)
			COOLSHIT(east)
			COOLSHIT(west)
		}

		void draw(Exposed &exposed, AmOcCache &aoc, std::vector<float> &posBuffer, std::vector<float> &UVBuffer,
				  std::vector<float> &lightBuffer, agl::Vec<float, 3> pos)
		{
			pos += offset;

			// y+
			if (up.exists && !(exposed.up == false && up.cull == true))
			{
				agl::Vec<float, 4> col = {84. / 85., 84. / 85., 84. / 85., 1};
				if (up.tintImage != nullptr)
				{
					col = up.tintImage
							  ->at({(up.tintImage->size.x - 1) - ((up.tintImage->size.x - 1) * 0.8),
									(up.tintImage->size.y - 1) * 0.4})
							  .normalized();
				}

				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z + size.z);

				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z + size.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z + size.z);

				UVBuffer.push_back((float)up.uv.x / 512);
				UVBuffer.push_back((float)up.uv.y / 512);
				UVBuffer.push_back((float)up.uv.x / 512 + (up.size.x / 512.));
				UVBuffer.push_back((float)up.uv.y / 512);
				UVBuffer.push_back((float)up.uv.x / 512);
				UVBuffer.push_back((float)up.uv.y / 512 + (up.size.y / 512.));

				UVBuffer.push_back((float)up.uv.x / 512 + (up.size.x / 512.));
				UVBuffer.push_back((float)up.uv.y / 512);
				UVBuffer.push_back((float)up.uv.x / 512);
				UVBuffer.push_back((float)up.uv.y / 512 + (up.size.y / 512.));
				UVBuffer.push_back((float)up.uv.x / 512 + (up.size.x / 512.));
				UVBuffer.push_back((float)up.uv.y / 512 + (up.size.y / 512.));

				lightBuffer.push_back((1 - (aoc.up.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.up.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.up.x0y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.up.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.up.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.up.x1y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.up.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.up.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.up.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.up.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.up.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.up.x1y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.up.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.up.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.up.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.up.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.up.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.up.x1y1 * .2)) * col.z);
			}

			// y-
			if (down.exists && !(exposed.down == false && down.cull == true))
			{
				agl::Vec<float, 4> col = {25. / 51., 25. / 51., 25. / 51., 1};
				if (down.tintImage != nullptr)
				{
					col = down.tintImage
							  ->at({(down.tintImage->size.x - 1) - ((down.tintImage->size.x - 1) * 0.8),
									(down.tintImage->size.y - 1) * 0.4})
							  .normalized();
				}

				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);

				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);

				UVBuffer.push_back((float)down.uv.x / 512);
				UVBuffer.push_back((float)down.uv.y / 512 + (down.size.y / 512.));
				UVBuffer.push_back((float)down.uv.x / 512 + (down.size.x / 512.));
				UVBuffer.push_back((float)down.uv.y / 512 + (down.size.y / 512.));
				UVBuffer.push_back((float)down.uv.x / 512);
				UVBuffer.push_back((float)down.uv.y / 512);

				UVBuffer.push_back((float)down.uv.x / 512 + (down.size.x / 512.));
				UVBuffer.push_back((float)down.uv.y / 512 + (down.size.y / 512.));
				UVBuffer.push_back((float)down.uv.x / 512);
				UVBuffer.push_back((float)down.uv.y / 512);
				UVBuffer.push_back((float)down.uv.x / 512 + (down.size.x / 512.));
				UVBuffer.push_back((float)down.uv.y / 512);

				lightBuffer.push_back((1 - (aoc.down.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.down.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.down.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.down.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.down.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.down.x1y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.down.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.down.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.down.x0y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.down.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.down.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.down.x1y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.down.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.down.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.down.x0y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.down.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.down.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.down.x1y0 * .2)) * col.z);
			}

			// z-
			if (south.exists && !(exposed.south == false && south.cull == true))
			{
				agl::Vec<float, 4> col = {202. / 255., 202. / 255., 202. / 255., 1};
				if (south.tintImage != nullptr)
				{
					col = south.tintImage
							  ->at({(south.tintImage->size.x - 1) - ((south.tintImage->size.x - 1) * 0.8),
									(south.tintImage->size.y - 1) * 0.4})
							  .normalized();
				}

				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);

				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);

				UVBuffer.push_back((float)south.uv.x / 512 + (south.size.x / 512.));
				UVBuffer.push_back((float)south.uv.y / 512 + (south.size.y / 512.));
				UVBuffer.push_back((float)south.uv.x / 512);
				UVBuffer.push_back((float)south.uv.y / 512 + (south.size.y / 512.));
				UVBuffer.push_back((float)south.uv.x / 512 + (south.size.x / 512.));
				UVBuffer.push_back((float)south.uv.y / 512);

				UVBuffer.push_back((float)south.uv.x / 512);
				UVBuffer.push_back((float)south.uv.y / 512 + (south.size.y / 512.));
				UVBuffer.push_back((float)south.uv.x / 512 + (south.size.x / 512.));
				UVBuffer.push_back((float)south.uv.y / 512);
				UVBuffer.push_back((float)south.uv.x / 512);
				UVBuffer.push_back((float)south.uv.y / 512);

				lightBuffer.push_back((1 - (aoc.south.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.south.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.south.x1y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.south.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.south.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.south.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.south.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.south.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.south.x1y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.south.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.south.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.south.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.south.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.south.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.south.x1y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.south.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.south.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.south.x0y0 * .2)) * col.z);
			}

			// z+
			if (north.exists && !(exposed.north == false && north.cull == true))
			{
				agl::Vec<float, 4> col = {202. / 255., 202. / 255., 202. / 255., 1};
				if (north.tintImage != nullptr)
				{
					col = north.tintImage
							  ->at({(north.tintImage->size.x - 1) - ((north.tintImage->size.x - 1) * 0.8),
									(north.tintImage->size.y - 1) * 0.4})
							  .normalized();
				}

				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z + size.z);

				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z + size.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z + size.z);

				UVBuffer.push_back((float)north.uv.x / 512);
				UVBuffer.push_back((float)north.uv.y / 512 + (north.size.y / 512.));
				UVBuffer.push_back((float)north.uv.x / 512 + (north.size.x / 512.));
				UVBuffer.push_back((float)north.uv.y / 512 + (north.size.y / 512.));
				UVBuffer.push_back((float)north.uv.x / 512);
				UVBuffer.push_back((float)north.uv.y / 512);

				UVBuffer.push_back((float)north.uv.x / 512 + (north.size.x / 512.));
				UVBuffer.push_back((float)north.uv.y / 512 + (north.size.y / 512.));
				UVBuffer.push_back((float)north.uv.x / 512);
				UVBuffer.push_back((float)north.uv.y / 512);
				UVBuffer.push_back((float)north.uv.x / 512 + (north.size.x / 512.));
				UVBuffer.push_back((float)north.uv.y / 512);

				lightBuffer.push_back((1 - (aoc.north.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.north.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.north.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.north.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.north.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.north.x1y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.north.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.north.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.north.x0y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.north.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.north.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.north.x1y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.north.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.north.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.north.x0y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.north.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.north.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.north.x1y0 * .2)) * col.z);
			}

			// x-
			if (west.exists && !(exposed.west == false && west.cull == true))
			{
				agl::Vec<float, 4> col = {151. / 255., 151. / 255., 151. / 255., 1};
				if (west.tintImage != nullptr)
				{
					col = west.tintImage
							  ->at({(west.tintImage->size.x - 1) - ((west.tintImage->size.x - 1) * 0.8),
									(west.tintImage->size.y - 1) * 0.4})
							  .normalized();
				}

				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);

				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);
				posBuffer.push_back(pos.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z + size.z);

				UVBuffer.push_back((float)west.uv.x / 512);
				UVBuffer.push_back((float)west.uv.y / 512 + (west.size.y / 512.));
				UVBuffer.push_back((float)west.uv.x / 512);
				UVBuffer.push_back((float)west.uv.y / 512);
				UVBuffer.push_back((float)west.uv.x / 512 + (west.size.x / 512.));
				UVBuffer.push_back((float)west.uv.y / 512 + (west.size.y / 512.));

				UVBuffer.push_back((float)west.uv.x / 512);
				UVBuffer.push_back((float)west.uv.y / 512);
				UVBuffer.push_back((float)west.uv.x / 512 + (west.size.x / 512.));
				UVBuffer.push_back((float)west.uv.y / 512 + (west.size.y / 512.));
				UVBuffer.push_back((float)west.uv.x / 512 + (west.size.x / 512.));
				UVBuffer.push_back((float)west.uv.y / 512);

				lightBuffer.push_back((1 - (aoc.west.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.west.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.west.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.west.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.west.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.west.x0y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.west.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.west.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.west.x1y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.west.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.west.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.west.x0y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.west.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.west.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.west.x1y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.west.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.west.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.west.x1y0 * .2)) * col.z);
			}

			// NOTE - The UV offset (16. / 512.) follows the aoc offset

			// x+
			if (east.exists && !(exposed.east == false && east.cull == true))
			{
				agl::Vec<float, 4> col = {151. / 255., 151. / 255., 151. / 255., 1};
				if (east.tintImage != nullptr)
				{
					col = east.tintImage
							  ->at({(east.tintImage->size.x - 1) - ((east.tintImage->size.x - 1) * 0.8),
									(east.tintImage->size.y - 1) * 0.4})
							  .normalized();
				}

				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);

				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y);
				posBuffer.push_back(pos.z + size.z);
				posBuffer.push_back(pos.x + size.x);
				posBuffer.push_back(pos.y + size.y);
				posBuffer.push_back(pos.z + size.z);

				UVBuffer.push_back((float)east.uv.x / 512 + (east.size.x / 512.));
				UVBuffer.push_back((float)east.uv.y / 512 + (east.size.y / 512.));
				UVBuffer.push_back((float)east.uv.x / 512 + (east.size.x / 512.));
				UVBuffer.push_back((float)east.uv.y / 512);
				UVBuffer.push_back((float)east.uv.x / 512);
				UVBuffer.push_back((float)east.uv.y / 512 + (east.size.y / 512.));

				UVBuffer.push_back((float)east.uv.x / 512 + (east.size.x / 512.));
				UVBuffer.push_back((float)east.uv.y / 512);
				UVBuffer.push_back((float)east.uv.x / 512);
				UVBuffer.push_back((float)east.uv.y / 512 + (east.size.y / 512.));
				UVBuffer.push_back((float)east.uv.x / 512);
				UVBuffer.push_back((float)east.uv.y / 512);

				lightBuffer.push_back((1 - (aoc.east.x1y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.east.x1y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.east.x1y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.east.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.east.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.east.x1y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.east.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.east.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.east.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.east.x1y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.east.x1y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.east.x1y0 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.east.x0y1 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.east.x0y1 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.east.x0y1 * .2)) * col.z);

				lightBuffer.push_back((1 - (aoc.east.x0y0 * .2)) * col.x);
				lightBuffer.push_back((1 - (aoc.east.x0y0 * .2)) * col.y);
				lightBuffer.push_back((1 - (aoc.east.x0y0 * .2)) * col.z);
			}
		}
};

class Block
{
	public:
		std::string name;

		bool				 solid = false;
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
				elements.emplace_back(val, texHash, atlas.size, tintGrass, tintFoliage, name, solid);
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
