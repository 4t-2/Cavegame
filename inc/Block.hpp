#pragma once

#include "Atlas.hpp"
#include <AGL/agl.hpp>
#include <json/json.h>

class Block;

struct Grid3
{
		std::vector<std::vector<std::vector<unsigned int>>> &data;
		unsigned int										 air;

		int exists(agl::Vec<int, 3> pos)
		{
			if (pos.x >= data.size() || pos.x < 0 || pos.y >= data[0].size() || pos.y < 0 ||
				pos.z >= data[0][0].size() || pos.z < 0)
			{
				return false;
			}
			return (data[pos.x][pos.y][pos.z] != air);
		}
};

struct Face
{
		agl::Vec<float, 2> uv		 = {0, 0};
		agl::Vec<float, 2> size		 = {0, 0};
		bool			   exists	 = false;
		Image			  *tintImage = nullptr;
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
		Face up;
		Face down;
		Face east;
		Face north;
		Face south;
		Face west;

		agl::Vec<float, 3> size;
		agl::Vec<float, 3> offset;

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
	}

			COOLSHIT(up)
			COOLSHIT(down)
			COOLSHIT(north)
			COOLSHIT(south)
			COOLSHIT(east)
			COOLSHIT(west)
		}

		static float AmOcCalc(agl::Vec<int, 3> pos, agl::Vec<int, 3> norm, agl::Vec<int, 3> acc1, agl::Vec<int, 3> acc2,
							  Grid3 &surround)
		{
			bool cornerTouch = surround.exists(pos + norm + acc1 + acc2);
			bool lineTouch	 = surround.exists(pos + norm + acc1);
			bool oppo		 = surround.exists(pos + norm + acc2);

			if (lineTouch && oppo)
			{
				return 0.6;
			}
			else if ((lineTouch && cornerTouch) || (oppo && cornerTouch))
			{
				return 0.4;
			}
			else if (lineTouch || oppo || cornerTouch)
			{
				return 0.2;
			}
			else
			{
				return 0;
			}
		}

		void render(agl::RenderWindow &window, agl::Shape &blankRect, agl::Vec<int, 3> pos, int id, AOUnfiforms aou,
					Grid3 &surround)
		{
			// 67/84 1 0.7976190476
			// 25/42 2 0.5952380952
			// 25/63 corner 0.3968253968
			// 67/84 outie 0.7976190476
			// all in .2s

			// y+
			if (up.exists)
			{
				if (up.tintImage != nullptr)
				{
					blankRect.setColor(
						up.tintImage->at({(up.tintImage->size.x - 1) - ((up.tintImage->size.x - 1) * 0.8),
										  (up.tintImage->size.y - 1) * 0.4}));
				}
				else
				{
					blankRect.setColor(agl::Color::White);
				}

				blankRect.setTextureTranslation(up.uv);
				blankRect.setTextureScaling(up.size);
				blankRect.setSize({size.x, size.z, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x, pos.y + size.y, pos.z} + offset);
				blankRect.setRotation({90, 0, 0});

				agl::Shader::setUniform(id, agl::Vec<float, 3>{0, 1, 0});

				float x0y0 = AmOcCalc(pos, {0, 1, 0}, {-1, 0, 0}, {0, 0, -1}, surround);
				float x1y0 = AmOcCalc(pos, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, surround);
				float x0y1 = AmOcCalc(pos, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, surround);
				float x1y1 = AmOcCalc(pos, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, surround);

				glUniform1f(aou.x0y0, x0y0);
				glUniform1f(aou.x1y0, x1y0);
				glUniform1f(aou.x0y1, x0y1);
				glUniform1f(aou.x1y1, x1y1);

				window.drawShape(blankRect);
			}

			// y-
			if (down.exists)
			{
				if (down.tintImage != nullptr)
				{
					blankRect.setColor(
						down.tintImage->at({(down.tintImage->size.x - 1) - ((down.tintImage->size.x - 1) * 0.8),
											(down.tintImage->size.y - 1) * 0.4}));
				}
				else
				{
					blankRect.setColor(agl::Color::White);
				}

				blankRect.setTextureTranslation(down.uv);
				blankRect.setTextureScaling(down.size);
				blankRect.setSize({size.x, -size.z, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x, pos.y, pos.z + size.z} + offset);
				blankRect.setRotation({90, 0, 0});

				agl::Shader::setUniform(id, agl::Vec<float, 3>{0, -1, 0});

				float x0y0 = AmOcCalc(pos, {0, -1, 0}, {-1, 0, 0}, {0, 0, 1}, surround);
				float x1y0 = AmOcCalc(pos, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, surround);
				float x0y1 = AmOcCalc(pos, {0, -1, 0}, {-1, 0, 0}, {0, 0, -1}, surround);
				float x1y1 = AmOcCalc(pos, {0, -1, 0}, {1, 0, 0}, {0, 0, -1}, surround);

				glUniform1f(aou.x0y0, x0y0);
				glUniform1f(aou.x1y0, x1y0);
				glUniform1f(aou.x0y1, x0y1);
				glUniform1f(aou.x1y1, x1y1);

				window.drawShape(blankRect);
			}

			if (south.exists)
			{
				if (south.tintImage != nullptr)
				{
					blankRect.setColor(
						south.tintImage->at({(south.tintImage->size.x - 1) - ((south.tintImage->size.x - 1) * 0.8),
											 (south.tintImage->size.y - 1) * 0.4}));
				}
				else
				{
					blankRect.setColor(agl::Color::White);
				}
				// z-
				blankRect.setTextureTranslation(south.uv);
				blankRect.setTextureScaling(south.size);
				blankRect.setSize({-size.x, -size.y, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x + size.x, pos.y + size.y, pos.z} + offset);
				blankRect.setRotation({0, 0, 0});

				agl::Shader::setUniform(id, agl::Vec<float, 3>{0, 0, -1});

				float x0y0 = AmOcCalc(pos, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, surround);
				float x1y0 = AmOcCalc(pos, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, surround);
				float x0y1 = AmOcCalc(pos, {0, 0, -1}, {1, 0, 0}, {0, -1, 0}, surround);
				float x1y1 = AmOcCalc(pos, {0, 0, -1}, {-1, 0, 0}, {0, -1, 0}, surround);

				glUniform1f(aou.x0y0, x0y0);
				glUniform1f(aou.x1y0, x1y0);
				glUniform1f(aou.x0y1, x0y1);
				glUniform1f(aou.x1y1, x1y1);

				window.drawShape(blankRect);
			}

			// z+
			if (north.exists)
			{
				if (north.tintImage != nullptr)
				{
					blankRect.setColor(
						north.tintImage->at({(north.tintImage->size.x - 1) - ((north.tintImage->size.x - 1) * 0.8),
											 (north.tintImage->size.y - 1) * 0.4}));
				}
				else
				{
					blankRect.setColor(agl::Color::White);
				}
				blankRect.setTextureTranslation(north.uv);
				blankRect.setTextureScaling(north.size);
				blankRect.setSize({size.x, -size.y, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x, pos.y + size.y, pos.z + size.z} + offset);
				blankRect.setRotation({0, 0, 0});

				agl::Shader::setUniform(id, agl::Vec<float, 3>{0, 0, 1});

				float x0y0 = AmOcCalc(pos, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}, surround);
				float x1y0 = AmOcCalc(pos, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, surround);
				float x0y1 = AmOcCalc(pos, {0, 0, 1}, {-1, 0, 0}, {0, -1, 0}, surround);
				float x1y1 = AmOcCalc(pos, {0, 0, 1}, {1, 0, 0}, {0, -1, 0}, surround);

				glUniform1f(aou.x0y0, x0y0);
				glUniform1f(aou.x1y0, x1y0);
				glUniform1f(aou.x0y1, x0y1);
				glUniform1f(aou.x1y1, x1y1);

				window.drawShape(blankRect);
			}

			// x-
			if (west.exists)
			{
				if (west.tintImage != nullptr)
				{
					blankRect.setColor(
						west.tintImage->at({(west.tintImage->size.x - 1) - ((west.tintImage->size.x - 1) * 0.8),
											(west.tintImage->size.y - 1) * 0.4}));
				}
				else
				{
					blankRect.setColor(agl::Color::White);
				}
				blankRect.setTextureTranslation(west.uv);
				blankRect.setTextureScaling(west.size);
				blankRect.setSize({size.z, -size.y, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x, pos.y + size.y, pos.z} + offset);
				blankRect.setRotation({0, 90, 0});

				agl::Shader::setUniform(id, agl::Vec<float, 3>{-1, 0, 0});

				float x0y0 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, surround);
				float x1y0 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, surround);
				float x0y1 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}, surround);
				float x1y1 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, 1}, {0, -1, 0}, surround);

				glUniform1f(aou.x0y0, x0y0);
				glUniform1f(aou.x1y0, x1y0);
				glUniform1f(aou.x0y1, x0y1);
				glUniform1f(aou.x1y1, x1y1);

				window.drawShape(blankRect);
			}

			// x+
			if (east.exists)
			{
				if (east.tintImage != nullptr)
				{
					blankRect.setColor(
						east.tintImage->at({(east.tintImage->size.x - 1) - ((east.tintImage->size.x - 1) * 0.8),
											(east.tintImage->size.y - 1) * 0.4}));
				}
				else
				{
					blankRect.setColor(agl::Color::White);
				}

				blankRect.setTextureTranslation(east.uv);
				blankRect.setTextureScaling(east.size);
				blankRect.setSize({-size.z, -size.y, 0});
				blankRect.setPosition(agl::Vec<float, 3>{pos.x + size.x, pos.y + size.y, pos.z + size.z} + offset);
				blankRect.setRotation({0, 90, 0});

				agl::Shader::setUniform(id, agl::Vec<float, 3>{1, 0, 0});

				float x0y0 = AmOcCalc(pos, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}, surround);
				float x1y0 = AmOcCalc(pos, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, surround);
				float x0y1 = AmOcCalc(pos, {1, 0, 0}, {0, 0, 1}, {0, -1, 0}, surround);
				float x1y1 = AmOcCalc(pos, {1, 0, 0}, {0, 0, -1}, {0, -1, 0}, surround);

				glUniform1f(aou.x0y0, x0y0);
				glUniform1f(aou.x1y0, x1y0);
				glUniform1f(aou.x0y1, x0y1);
				glUniform1f(aou.x1y1, x1y1);

				window.drawShape(blankRect);
			}
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
					Grid3 surround)
		{
			for (auto &e : elements)
			{
				e.render(window, blankRect, pos, id, aou, surround);
			}
		}

		~Block()
		{
			// dynamicDo([](auto pd) { delete pd; });
		}
};
