#include <AGL/agl.hpp>

#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <math.h>
#include <string>
#include <thread>

#include "../inc/Atlas.hpp"
#include "../inc/Block.hpp"
#include "../inc/World.hpp"

#define GRAVACC (-.08 / 9)
#define WALKACC (1 / 3.)
#define BLCKFRC 0.6

#define BASESPEED WALKVELPERTICK

#define FOREACH(x) for (int index = 0; index < x.size(); index++)

enum ListenState
{
	Hold,
	First,
	Last,
	Null
};

class Listener
{
	private:
		bool pastState = false;

	public:
		ListenState ls = ListenState::Null;
		void		update(bool state)
		{
			if (state)
			{
				if (pastState)
				{
					ls = ListenState::Hold;
				}
				else
				{
					ls = ListenState::First;

					pastState = true;
				}
			}
			else if (pastState)
			{
				ls		  = ListenState::Last;
				pastState = false;
			}
		}
};

int getMillisecond()
{
	auto timepoint = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(timepoint).count();
}

agl::Vec<float, 2> getCursorScenePosition(agl::Vec<float, 2> cursorWinPos, agl::Vec<float, 2> winSize, float winScale,
										  agl::Vec<float, 2> cameraPos)
{
	return ((cursorWinPos - (winSize * .5)) * winScale) + cameraPos;
}

class Player
{
	public:
		agl::Vec<float, 3> pos = {1, 150, 1};
		agl::Vec<float, 3> rot = {0, 0, 0};
		agl::Vec<float, 3> vel = {0, 0, 0};

		float friction = BLCKFRC;

		bool sneaking  = false;
		bool sprinting = false;
		bool grounded  = false;

		void update()
		{
		}
};

void hideCursor(agl::RenderWindow &window)
{
	Cursor		invisibleCursor;
	Pixmap		bitmapNoData;
	XColor		black;
	static char noData[] = {0, 0, 0, 0, 0, 0, 0, 0};
	black.red = black.green = black.blue = 0;

	bitmapNoData	= XCreateBitmapFromData(window.baseWindow.dpy, window.baseWindow.win, noData, 8, 8);
	invisibleCursor = XCreatePixmapCursor(window.baseWindow.dpy, bitmapNoData, bitmapNoData, &black, &black, 0, 0);
	XDefineCursor(window.baseWindow.dpy, window.baseWindow.win, invisibleCursor);
	XFreeCursor(window.baseWindow.dpy, invisibleCursor);
	XFreePixmap(window.baseWindow.dpy, bitmapNoData);
}

void movePlayer(Player &player, agl::Vec<float, 3> acc)
{
	player.vel.x *= BLCKFRC * 0.91;
	player.vel.z *= BLCKFRC * 0.91;

	acc *= WALKACC * 0.98;

	float mod = 1;
	if (player.sneaking)
	{
		mod *= .3;
	}
	if (player.sprinting)
	{
		mod *= 1.3;
	}

	acc *= mod;
	if (acc.length() > std::max(mod, 1.f) / 3)
	{
		acc = acc.normalized() * mod / 3;
	}

	player.vel += acc * 0.1;

	player.pos += player.vel;

	player.vel.y *= 0.98;
	player.vel.y += GRAVACC;
}

struct Collision
{
		agl::Vec<int, 3> norm;
		float			 overlap;
};

struct Box
{
		agl::Vec<float, 3> pos;
		agl::Vec<float, 3> size;
};

Collision boxCollide(Box b1, Box b2)
{
	// Y test
	agl::Vec<float, 3> overlap;

	{
		overlap.y = (b1.pos.y + b1.size.y) - b2.pos.y;

		if (overlap.y < 0)
		{
			return {};
		}
	}

	return {{0, 1, 0}, overlap.y};
}

void correctPosition(Player &player, World &world)
{
	for (int x = player.pos.x - 0.3; x < player.pos.x + 0.3; x++)
	{
		for (int z = player.pos.z - 0.3; z < player.pos.z + 0.3; z++)
		{
			if (!world.getAtPos({x, player.pos.y + 1, z}) && !world.getAtPos({x, player.pos.y + 2, z}))
			{
				if (world.getAtPos({x, player.pos.y, z}) && player.vel.y < 0)
				{
					if (player.pos.y - (int)player.pos.y < .5)
					{
						continue;
					}
					player.vel.y = 0;
					player.pos.y = (int)player.pos.y + 1;

					player.grounded = true;
				}

				if (world.getAtPos({x, player.pos.y + 1.8, z}) && player.vel.y > 0)
				{
					player.vel.y = 0;
					player.pos.y = (int)(player.pos.y + 1.8) - 1.8;
				}
			}
		}
	}

	for (int x = player.pos.x - 0.3; x < player.pos.x + 0.3; x++)
	{
		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			for (int z = player.pos.z - 0.3; z < player.pos.z + 0.3; z++)
			{
				if (world.getAtPos({x, y, z}))
				{
					agl::Vec<float, 2> center = {x + .5, y + .5, z + .5};
					if (abs(center.x - player.pos.x) < abs(center.z - player.pos.z))
					{
						if (center.z > player.pos.z)
						{
							player.vel.z = 0;
							player.pos.z = center.z - 0.3 - .5;
						}
						else if (center.z < player.pos.z)
						{
							player.vel.z = 0;
							player.pos.z = center.z + 0.3 + .5;
						}
					}
					else
					{
						if (center.x > player.pos.x)
						{
							player.vel.x = 0;
							player.pos.x = center.x - 0.3 - .5;
						}
						else if (center.x < player.pos.x)
						{
							player.vel.x = 0;
							player.pos.x = center.x + 0.3 + .5;
						}
					}
				}
			}
		}
	}
}

void updateSelected(Player &player, agl::Vec<int, 3> &selected, agl::Vec<int, 3> &front, World &world)
{
	agl::Vec<float, 3> dir = {-sin(player.rot.y) * cos(player.rot.x), -sin(player.rot.x),
							  -cos(player.rot.y) * cos(player.rot.x)};

	agl::Vec<float, 3> blockPos = player.pos + agl::Vec<float, 3>{0, 1.8, 0};

	selected = blockPos;
	front	 = blockPos;

	while (true)
	{
		if (blockPos.x >= world.size.x || blockPos.x < 0 || blockPos.y >= world.size.y || blockPos.y < 0 ||
			blockPos.z >= world.size.z || blockPos.z < 0)
		{
			selected = front;
			break;
		}

		if (world.getAtPos(blockPos))
		{
			selected = blockPos;
			break;
		}

		blockPos += dir / 100;

		if (selected == agl::Vec<int, 3>(blockPos))
		{
		}
		else
		{
			front	 = selected;
			selected = blockPos;
		}
	}
}

class CommandBox : public agl::Drawable
{
	public:
		agl::Shape	 &rect;
		agl::Text	 &text;
		agl::Texture &blank;

		std::string cmd = "";

		std::vector<Block> &blocks;

		agl::Vec<int, 2> &winSize;

		bool commit = false;

		bool &focused;

		int pallete = 1;

		CommandBox(agl::Shape &rect, agl::Text &text, agl::Texture &blank, std::vector<Block> &blocks,
				   agl::Vec<int, 2> &winSize, bool &focused)
			: rect(rect), text(text), blank(blank), blocks(blocks), winSize(winSize), focused(focused)
		{
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

		void drawFunction(agl::RenderWindow &win) override
		{
			rect.setTexture(&blank);
			rect.setColor(agl::Color::Black);
			rect.setPosition({0, 0, 0});
			rect.setRotation({0, 0, 0});
			rect.setSize({winSize.x, text.getHeight() + 10, 0});

			win.drawShape(rect);

			text.setText(" > " + cmd);
			text.setPosition({0, 0, 0});
			text.setColor(agl::Color::White);

			win.drawText(text);

			int offset = text.getHeight() + 10;

			for (unsigned int i = 0; i < blocks.size(); i++)
			{
				auto &b = blocks[i];

				if (b.name.substr(0, cmd.length()) == cmd)
				{
					rect.setPosition({0, offset, 0});
					text.setPosition({0, offset, 0});
					text.setText(b.name);
					text.setColor(agl::Color::Gray);

					win.drawShape(rect);
					win.drawText(text);

					offset += text.getHeight() + 10;
				}

				if (b.name.substr(0, cmd.size()) == cmd && commit)
				{
					commit	= false;
					pallete = i;
					focused = true;
					cmd		= "";
					break;
				}
			}

			commit = false;
		}
};

int main()
{
	printf("Starting AGL\n");

	agl::RenderWindow window;
	window.setup({1920, 1080}, "CaveGame");
	window.setClearColor({0x78, 0xA7, 0xFF});
	window.setFPS(0);

	agl::Vec<int, 2> windowSize;

	window.GLEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);

	glDepthFunc(GL_LEQUAL);

	window.setSwapInterval(1);

	agl::Event event;
	event.setWindow(window);

	agl::Shader worldShader;
	worldShader.loadFromFile("./shader/vert.glsl", "./shader/frag.glsl");

	agl::Shader uiShader;
	uiShader.loadFromFile("./shader/uivert.glsl", "./shader/frag.glsl");

	Atlas atlas("./resources/java/assets/minecraft/textures/block/");

	std::vector<Block> blockDefs;
	std::map<std::string, int> blockNameToDef;

	blockDefs.reserve(atlas.blockMap.size() + 1);
	{
		std::map<std::string, Json::Value> jsonPairs;

		for (auto &entry :
			 std::filesystem::recursive_directory_iterator("./resources/java/assets/minecraft/models/block/"))
		{
			std::fstream fs(entry.path(), std::ios::in);

			Json::Value	 root;
			Json::Reader reader;
			reader.parse(fs, root, false);

			fs.close();

			auto s = std::filesystem::path(entry).filename().string();

			jsonPairs[s.substr(0, s.length() - 5)] = root;
		}

		for (auto &e : jsonPairs)
		{
			blockNameToDef["minecraft:" + e.first] = blockDefs.size();
			blockDefs.emplace_back(atlas, e.first, jsonPairs);
		}
	}

	agl::Texture foodTexture;
	foodTexture.loadFromFile("./img/food.png");

	agl::Texture creatureBodyTexture;
	creatureBodyTexture.loadFromFile("./img/creatureBody.png");

	agl::Texture creatureExtraTexture;
	creatureExtraTexture.loadFromFile("./img/creatureExtra.png");

	agl::Texture eggTexture;
	eggTexture.loadFromFile("./img/egg.png");

	agl::Texture meatTexture;
	meatTexture.loadFromFile("./img/meat.png");

	agl::Texture blank;
	blank.setBlank();

	agl::Font font;
	font.setup("./font/font.ttf", 24);

	agl::Text text;
	text.setFont(&font);
	text.setScale(1);

	agl::Rectangle blankRect;
	blankRect.setTexture(&blank);

	World world;
	world.setBasics(blockDefs);
	world.loadFromFile("world/region/r.0.0.mca", blockNameToDef);

	// {
	// 	unsigned int cobblestone = 0;
	//
	// 	for (int i = 0; i < blockDefs.size(); i++)
	// 	{
	// 		if (blockDefs[i].name == "cobblestone")
	// 		{
	// 			cobblestone = i;
	// 		}
	// 	}
	//
	// 	for (int x = 0; x < 20; x++)
	// 	{
	// 		for (int z = 0; z < 20; z++)
	// 		{
	// 			world.blocks[x][0][z] = cobblestone;
	// 			world.blocks[x][1][z] = cobblestone;
	// 			world.blocks[x][2][z] = cobblestone;
	// 		}
	// 	}
	//
	// 	for (int x = 0; x < 20; x++)
	// 	{
	// 		for (int y = 3; y < 20; y++)
	// 		{
	// 			for (int z = 0; z < 20; z++)
	// 			{
	// 				world.blocks[x][y][z] = world.air;
	// 			}
	// 		}
	// 	}
	// }

	// return 0;

	window.getShaderUniforms(worldShader);
	worldShader.use();
	auto		normUniform = worldShader.getUniformLocation("norm");
	AOUnfiforms aou;
	aou.x0y0 = worldShader.getUniformLocation("x0y0");
	aou.x1y0 = worldShader.getUniformLocation("x1y0");
	aou.x0y1 = worldShader.getUniformLocation("x0y1");
	aou.x1y1 = worldShader.getUniformLocation("x1y1");

	bool focused = true;

	CommandBox cmdBox(blankRect, text, blank, blockDefs, windowSize, focused);

	cmdBox.pallete = world.cobblestone;

	agl::Vec<int, 3> selected;
	agl::Vec<int, 3> front;

	Listener lclis;
	Listener rclis;

	Player player;

	hideCursor(window);

	while (!event.windowClose())
	{
		static int milliDiff = 0;
		int		   start	 = getMillisecond();

		event.poll();

		windowSize = window.getState().size;

		window.clear();

		worldShader.use();
		window.getShaderUniforms(worldShader);
		{
			agl::Mat<float, 4> tran;
			tran.translate(player.pos * -1 - agl::Vec{0.f, 1.8f, 0.f});

			agl::Mat<float, 4> rot;
			rot.rotate({agl::radianToDegree(player.rot.x), agl::radianToDegree(player.rot.y),
						agl::radianToDegree(player.rot.z)});

			agl::Mat<float, 4> proj;
			proj.perspective(PI / 2, (float)windowSize.x / windowSize.y, 0.1, 100);

			window.updateMvp(proj * rot * tran);
		}

		for (int x = 0; x < world.blocks.size(); x++)
		{

			for (int y = 0; y < world.blocks[x].size(); y++)
			{
				for (int z = 0; z < world.blocks[x][y].size(); z++)
				{
					if (world.getAtPos({x, y, z}))
					{
						blankRect.setColor(agl::Color::White);
						blankRect.setTexture(&atlas.texture);
						blankRect.setTextureScaling({16 / (float)atlas.size.x, 16 / (float)atlas.size.y});

						Block &type = blockDefs[world.blocks[x][y][z]];

						type.render(window, blankRect, {x, y, z}, normUniform, aou, Grid3{world.blocks, world.air});

						// if (selected == agl::Vec{x, y, z})
						// {
						// 	cube.setColor(agl::Color::Green);
						// }
					}
				}
			}
		}

		glDisable(GL_DEPTH_TEST);

		if (!event.isKeyPressed(agl::Key::F1))
		{
			agl::Mat4f proj;
			agl::Mat4f trans;
			proj.ortho(0, windowSize.x, windowSize.y, 0, 0.1, 100);
			trans.lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});

			uiShader.use();
			window.getShaderUniforms(uiShader);
			window.updateMvp(proj * trans);

			blankRect.setTextureScaling({1, 1, 1});
			blankRect.setTextureTranslation({0, 0, 0});
			blankRect.setTexture(&blank);
			blankRect.setColor(agl::Color::White);
			blankRect.setRotation({0, 0, 0});
			blankRect.setSize({3, 3});
			blankRect.setPosition(windowSize / 2 - blankRect.getSize() / 2);
			window.drawShape(blankRect);

			if (!focused)
			{
				window.draw(cmdBox);
			}
		}

		glEnable(GL_DEPTH_TEST);

		window.display();

		static int frame = 0;
		frame++;

		if (focused)
		{
			static agl::Vec<int, 2> oldMousePos = event.getPointerWindowPosition();

			agl::Vec<int, 2> mousePos = event.getPointerWindowPosition();

			agl::Vec<int, 2> deltaPos = mousePos - oldMousePos;

			constexpr float sensitivity = .5;

			agl::Vec<float, 2> rotDelta =
				(agl::Vec<float, 3>(deltaPos.y, -deltaPos.x, 0) * 1.2 * std::pow(sensitivity * 0.6 + 0.2, 3));

			player.rot += rotDelta * PI / 180;

			if (player.rot.x > PI / 2)
			{
				player.rot.x = PI / 2;
			}
			else if (player.rot.x < -PI / 2)
			{

				player.rot.x = -PI / 2;
			}

			oldMousePos = mousePos;
			Window win	= 0;
			int	   i	= 0;
			XGetInputFocus(window.baseWindow.dpy, &win, &i);
			if (win == window.baseWindow.win)
			{
				if ((mousePos - (windowSize / 2)).length() > std::min(windowSize.x / 2, windowSize.y / 2))
				{
					XWarpPointer(window.baseWindow.dpy, None, window.baseWindow.win, 0, 0, 0, 0, windowSize.x / 2,
								 windowSize.y / 2);

					oldMousePos = windowSize / 2;
				}
			}

			updateSelected(player, selected, front, world);

			agl::Vec<float, 3> acc;
			if (event.isKeyPressed(agl::Key::W))
			{
				acc.x += -sin(player.rot.y);
				acc.z += -cos(player.rot.y);
			}
			if (event.isKeyPressed(agl::Key::A))
			{
				acc.x += -cos(player.rot.y);
				acc.z += sin(player.rot.y);
			}
			if (event.isKeyPressed(agl::Key::S))
			{
				acc.x += sin(player.rot.y);
				acc.z += cos(player.rot.y);
			}
			if (event.isKeyPressed(agl::Key::D))
			{
				acc.x += cos(player.rot.y);
				acc.z += -sin(player.rot.y);
			}

			if (event.isKeyPressed(agl::Key::Space) && player.grounded)
			{
				player.vel.y = 0.48 / 3;
			}

			player.grounded = false;

			movePlayer(player, acc);
			correctPosition(player, world);

			lclis.update(event.isPointerButtonPressed(agl::Button::Left));
			rclis.update(event.isPointerButtonPressed(agl::Button::Right));

			if (rclis.ls == ListenState::First && !(front == player.pos))
			{
				world.blocks[front.x][front.y][front.z] = cmdBox.pallete;
			}
			if (lclis.ls == ListenState::First && focused)
			{
				world.blocks[selected.x][selected.y][selected.z] = world.air;
			}

			if (event.isKeyPressed(agl::Key::T))
			{
				focused = false;
			}
		}
		else
		{
			if (event.isKeyPressed(agl::Key::Escape))
			{
				focused = true;
			}
			else
			{
				cmdBox.update(event.keybuffer);
			}
		}

		window.setViewport(0, 0, windowSize.x, windowSize.y);
	}

	font.deleteFont();

	foodTexture.deleteTexture();
	creatureBodyTexture.deleteTexture();
	creatureExtraTexture.deleteTexture();
	eggTexture.deleteTexture();
	blank.deleteTexture();

	worldShader.deleteProgram();

	window.close();

	return 0;
}
