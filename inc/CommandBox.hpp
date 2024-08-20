#pragma once

#include "Atlas.hpp"
#include "Block.hpp"
#include <AXIS/ax.hpp>
#include <algorithm>

class MCText
{
	public:
		// 8x8 char, ASCII
		agl::Texture	atlas;
		agl::Rectangle &rect;

		int kerning[256];

		float scale = 1;

		MCText(agl::Rectangle &rect) : rect(rect)
		{
			Image im;
			im.load("resources/java/assets/minecraft/textures/font/ascii.png");

			Json::Value	 root;
			Json::Reader reader;

			std::fstream fs("resources/java/assets/minecraft/font/include/default.json");

			reader.parse(fs, root, false);

			root = root["providers"][2]["chars"];

			for (int c = 0; c < 255; c++)
			{
				int offsetX = c % 16;
				int offsetY = c / 16;
				offsetX *= 8;
				offsetY *= 8;

				int	 last;
				bool none = true;

				for (int x = 0; x < 8; x++)
				{
					for (int y = 0; y < 8; y++)
					{
						if (im.at({offsetX + x, offsetY + y}).a != 0)
						{
							last = x + 2;
							none = false;
							break;
						}
					}
				}

				if (none)
				{
					kerning[c] = 4;
				}
				else
				{
					kerning[c] = last;
				}
			}

			im.free();

			atlas.loadFromFile("resources/java/assets/minecraft/textures/font/ascii.png");
			atlas.useNearestFiltering();
		}

		agl::Vec<float, 2> draw(agl::RenderWindow &window, std::string text, agl::Vec<float, 2> pos, agl::Color color)
		{
			agl::Vec<float, 2> offset = pos;

			rect.setSize(agl::Vec<float, 2>{8, 8} * scale);
			rect.setOffset({0, 0});
			rect.setTexture(&atlas);
			rect.setRotation({0, 0, 0});
			rect.setTextureScaling({1 / 16., 1 / 16.});
			rect.setColor(color);

			for (char c : text)
			{
				rect.setPosition(offset);
				rect.setTextureTranslation({int(c % 16) / 16., int(c / 16) / 16.});
				window.drawShape(rect);

				if (c == '\n')
				{
					offset.x = pos.x;
					offset.y += 8 * scale;
				}
				else
				{
					offset.x += kerning[c] * scale;
				}
			}

			return offset;
		}

		agl::Vec<float, 2> fakeDraw(std::string text, agl::Vec<float, 2> pos)
		{
			agl::Vec<float, 2> offset = pos;

			for (char c : text)
			{
				if (c == '\n')
				{
					offset.x = pos.x;
					offset.y += 8 * scale;
				}
				else
				{
					offset.x += kerning[c] * scale;
				}
			}

			return offset;
		}

		float getHeight()
		{
			return 8 * scale;
		}

		~MCText()
		{
			atlas.deleteTexture();
		}
};

inline std::vector<std::string> splitString(const std::string &str, char delimiter)
{
	std::vector<std::string> tokens;
	std::stringstream		 ss(str);
	std::string				 token;
	while (std::getline(ss, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

class CommandFunction
{
	public:
		std::string								name;
		std::vector<std::vector<std::string> *> params;

		std::function<void(std::vector<std::string>)> func;
};

inline void drawList(std::string input, agl::Vec<float, 2> offset, std::vector<std::string> &blockList, int length,
					 MCText &text, agl::Rectangle &rect, agl::Texture &blank, agl::Vec<int, 2> winSize,
					 agl::RenderWindow &win)
{
	int width  = 0;
	int height = 0;

	for (auto &element : blockList)
	{
		if (element.substr(0, input.size()) == input)
		{
			auto vec = text.fakeDraw(element, {0, 0});

			if (width < vec.x)
			{
				width = vec.x;
			}

			height++;
			if (height >= 10)
			{
				break;
			}
		}
	}

	height *= 24;

	rect.setTexture(&blank);
	rect.setColor(agl::Color{0, 0, 0, 192});
	rect.setPosition({offset.x - 2, winSize.y - 30 - height});
	rect.setSize({width, height});

	win.drawShape(rect);

	int count = 0;
	for (auto &element : blockList)
	{
		if (element.substr(0, input.size()) == input)
		{
			if (count == 0)
			{
				text.draw(win, element, {offset.x + 2, winSize.y - 24 - height + 2 + (count * 24)},
						  agl::Color{0x3c, 0x3c, 0x00, 0xFF});
				text.draw(win, element, {offset.x, winSize.y - 24 - height + (count * 24)},
						  agl::Color{0xfd, 0xfe, 0x00, 0xFF});
			}
			else
			{
				text.draw(win, element, {offset.x + 2, winSize.y - 24 - height + 2 + (count * 24)},
						  agl::Color{0x34, 0x34, 0x34, 0xFF});
				text.draw(win, element, {offset.x, winSize.y - 24 - height + (count * 24)},
						  agl::Color{0xDE, 0xDE, 0xDE, 0xFF});
			}

			count++;
			if (count >= length)
			{
				break;
			}
		}
	}
}

class CommandBox : public agl::Drawable
{
	public:
		agl::Rectangle &rect;
		agl::Texture   &blank;

		std::string cmd = "";

		std::vector<std::string> funcNames;

		agl::Vec<int, 2> &winSize;

		bool commit = false;

		MCText &text;

		std::vector<CommandFunction> functions;

		CommandBox(agl::Rectangle &rect, MCText &text, agl::Texture &blank, agl::Vec<int, 2> &winSize)
			: rect(rect), blank(blank), winSize(winSize), text(text)
		{
		}

		void setCommands()
		{
			for (auto &e : functions)
			{
				funcNames.emplace_back(e.name);
			}

			std::sort(funcNames.begin(), funcNames.end());
		}

		void update(std::string buffer)
		{
			cmd += buffer;

			auto safeSub = [](std::string &str, int pos) -> std::string {
				if (pos > str.length())
				{
					return "";
				}
				else
				{
					return str.substr(pos);
				}
			};
			auto removeChar = [&](std::string &str, int i) { str = str.substr(0, i) + safeSub(str, i + 1); };

			for (int i = 0; i < cmd.size(); i++)
			{
				if (cmd[i] == 8)
				{
					removeChar(cmd, i);
					i--;
					if (i >= 0)
					{
						removeChar(cmd, i);
						i--;
					}
				}
				else if (cmd[i] == '\r')
				{
					commit = true;
					cmd	   = cmd.substr(0, i);
					break;
				}
				else if (cmd[i] < 32 || cmd[i] == 127)
				{
					removeChar(cmd, i);
					i--;
				}
			}
		}

		void drawFunction(agl::RenderWindow &win) override;
};
