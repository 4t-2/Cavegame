#pragma once

#include <Viz/viz.hpp>
#include <Vec.hpp>
#include <filesystem>
#include <json/json.h>
#include <stb_image.h>

#include <map>

static int dimtorange(agl::Vec<int, 2> dim, agl::Vec<int, 2> size, int period)
{
	return dim.x * period + (dim.y * size.x * period);
}

namespace cg
{
class Image
{
	public:
		agl::Vec<int, 2> size;
		agl::Color		*data;

		void load(std::string path)
		{
			data = (agl::Color *)stbi_load(path.c_str(), &size.x, &size.y, 0, 4);
		}

		agl::Color at(agl::Vec<int, 2> pos)
		{
			return data[dimtorange(pos, size, 1)];
		}

		void free()
		{
			stbi_image_free((char *)data);
		}
};
}

template<typename T>
std::string FUCKWINDOWS(const T* input) {
    static_assert(std::is_same<T, char>::value || std::is_same<T, wchar_t>::value,
                  "This function only accepts char* or wchar_t*");

    if constexpr (std::is_same<T, char>::value) {
        return std::string(input);
    } else if constexpr (std::is_same<T, wchar_t>::value) {
        std::wstring wstr(input);
        return std::string(wstr.begin(), wstr.end());
    }
}

class Atlas
{
	public:
		Image texture;

		agl::Vec<int, 2> size;

		std::map<std::string, agl::Vec<int, 2>> blockMap;

		Atlas(std::string path, Instance *instance)
		{
			std::vector<std::pair<unsigned char *, std::string>> blocks;

			for (auto &entry : std::filesystem::recursive_directory_iterator(path))
			{
				if (entry.is_directory())
				{
					continue;
				}

				agl::Vec<int, 2> size;
				unsigned char	*data = stbi_load(entry.path().string().c_str(), &size.x, &size.y, 0, 4);

				if (size.x != 16 || size.y != 16)
				{
					stbi_image_free(data);
					continue;
				}

				std::string filename = entry.path().filename().string();

				blocks.push_back({data, filename.substr(0, filename.length() - 4)});
			}

			float exact = std::sqrt(blocks.size() * 16 * 16);

			int sq = 1;

			for (; sq < exact; sq *= 2);

			std::vector<unsigned char> finalTex;
			finalTex.resize(sq * sq * 4);

			agl::Vec<int, 2> offset = {0, 0};

			for (int i = 0; i < blocks.size(); i++)
			{
				for (int x = 0; x < 16; x++)
				{
					for (int y = 0; y < 16; y++)
					{
						int texI					   = dimtorange(agl::Vec<int, 2>{x, y} + offset, {sq, sq}, 4);
						int blcI					   = dimtorange({x, y}, {16, 16}, 4);
						*(agl::Color *)&finalTex[texI] = *(agl::Color *)&blocks[i].first[blcI];
					}
				}

				blockMap["block/" + blocks[i].second] = offset;

				offset.x += 16;
				if ((offset.x + 16) > sq)
				{
					offset.x = 0;
					offset.y += 16;
				}

				stbi_image_free(blocks[i].first);
			}

			texture = instance->createImage(sq, sq, &finalTex[0]);

			this->size = {sq, sq};
		}
};
