#include "../inc/CommandBox.hpp"

void CommandBox::drawFunction(agl::RenderWindow &win)
{
	rect.setTexture(&blank);
	rect.setColor(agl::Color{0, 0, 0, 127});
	rect.setPosition({4, winSize.y - 4 - 24, 0});
	rect.setRotation({0, 0, 0});
	rect.setSize({winSize.x - 8, 24, 0});

	win.drawShape(rect);

	std::vector<std::string> array = splitString(cmd, ' ');

	{
		agl::Vec<float, 2> offset = {8, winSize.y - 24};

		int funcid = -1;

		if (array.size() != 0)
		{
			for (int i = 0; i < functions.size(); i++)
			{
				if (array[0] == functions[i].name)
				{
					funcid = i;
					break;
				}
			}
		}

		for (int i = 0; i < array.size(); i++)
		{
			auto &e = array[i];

			text.draw(win, e, {offset.x + 2, offset.y + 2}, agl::Color{0x34, 0x34, 0x34, 0xFF});
			auto last = text.draw(win, e, offset, agl::Color{0xDE, 0xDE, 0xDE, 0xFF});

			if (cmd.back() != ' ')
			{
				if (array.size() == 1)
				{
					drawList(e, offset, funcNames, 10, text, rect, blank, winSize, win);
				}
				else if (funcid != -1 && i == (array.size() - 1))
				{
					if ((array.size() - 1) <= functions[funcid].params.size())
					{
						drawList(e, offset, *functions[funcid].params[array.size() - 2], 10, text, rect, blank, winSize,
								 win);
					}
				}
			}

			offset.x = 8 + last.x;
		}
		if (array.size() == 0)
		{
			drawList("", offset, funcNames, 10, text, rect, blank, winSize, win);
		}
		else if (cmd.back() == ' ' && funcid != -1)
		{
			if ((array.size() - 1) < functions[funcid].params.size())
			{
				drawList("", offset, *functions[funcid].params[array.size() - 1], 10, text, rect, blank, winSize, win);
			}
		}
	}
}
