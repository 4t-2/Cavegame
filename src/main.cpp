#include <AXIS/ax.hpp>

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
#include "../inc/Mesh.hpp"
#include "../inc/Serializer.hpp"
#include "../inc/World.hpp"

// b5d1ff

#define GRAVACC (-.08 / 9)
#define WALKACC (1 / 3.)
#define BLCKFRC 0.6

#define BASESPEED WALKVELPERTICK

#define FOREACH(x) for (int index = 0; index < x.size(); index++)

enum GameState
{
	RUNNING = 1,
	CMD		= 0,
	PAUSE	= 69
};

class Config
{
	public:
		bool showFps;
		int	 fpsCap;
};

template <typename T> void recurse(T processor, Config &s, std::string name = "null")
{
	processor.process(name, s);

	RECSER(s.showFps);
	RECSER(s.fpsCap);
}

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

// inline bool &vecToMap(bool arr[3][3][3], agl::Vec<int, 3> vec)
// {
// 	return arr[1 + vec.x][1 + vec.y][1 + vec.z];
// }
//
// inline bool &vecToArr(bool arr[3][3][3], agl::Vec<int, 3> vec)
// {
// 	return arr[vec.x][vec.y][vec.z];
// }
//
// unsigned int AmOcCalc(bool blockMap[3][3][3], agl::Vec<int, 3> norm,
// agl::Vec<int, 3> acc1, agl::Vec<int, 3> acc2)
// {
// 	bool &cornerTouch = vecToMap(blockMap, norm + acc1 + acc2);
// 	bool &lineTouch	  = vecToMap(blockMap, norm + acc1);
// 	bool &oppo		  = vecToMap(blockMap, norm + acc2);

class Player
{
	public:
		agl::Vec<float, 3> pos = {16 * 16, 150, 16 * 16};
		agl::Vec<float, 3> rot = {0, PI / 2, 0};
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
#ifdef __linux__
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
#endif

#ifdef _WIN32
	glfwSetInputMode(window.baseWindow.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
#endif
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

void correctPositionXZ(Player &player, World &world)
{
	for (int x = player.pos.x - 0.3; x < player.pos.x + 0.3; x++)
	{
		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, (int)player.pos.z}))
			{
				agl::Vec<float, 2> center = {x + .5, y + .5, (int)player.pos.z + .5};
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
	for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
	{
		for (int z = player.pos.z - 0.3; z < player.pos.z + 0.3; z++)
		{
			if (world.getAtPos({(int)player.pos.x, y, z}))
			{
				agl::Vec<float, 2> center = {(int)player.pos.x + .5, y + .5, z + .5};
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
		}
	}
}

void correctPositionY(Player &player, World &world)
{
	for (int x = player.pos.x - 0.29; x < player.pos.x + 0.29; x++)
	{
		for (int z = player.pos.z - 0.29; z < player.pos.z + 0.29; z++)
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

void movePlayer(Player &player, agl::Vec<float, 3> acc, World &world)
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
		mod *= 2;
	}

	acc *= mod;
	// if (acc.length() > std::max(mod, 1.f) / 3)
	// {
	// 	acc = acc.normalized() * mod / 3;
	// }

	player.vel += acc * 0.1;

	player.pos.x += player.vel.x;
	player.pos.z += player.vel.z;

	correctPositionXZ(player, world);

	player.pos.y += player.vel.y;

	correctPositionY(player, world);

	player.vel.y *= 0.98;
	player.vel.y += GRAVACC;
}

void updateSelected(Player &player, agl::Vec<int, 3> &selected, agl::Vec<int, 3> &front, World &world)
{
	agl::Vec<float, 3> dir = {-sin(player.rot.y) * cos(player.rot.x), -sin(player.rot.x),
							  -cos(player.rot.y) * cos(player.rot.x)};

	agl::Vec<float, 3> blockPos = player.pos + agl::Vec<float, 3>{0, 1.8, 0};

	selected = blockPos;
	front	 = blockPos;

	float dist = 0;

	while (true)
	{
		if (world.getAtPos(blockPos))
		{
			selected = blockPos;
			break;
		}

		blockPos += dir / 100;

		dist += dir.length() / 100;

		if (selected == agl::Vec<int, 3>(blockPos))
		{
		}
		else
		{
			front	 = selected;
			selected = blockPos;
		}

		if (dist > 10)
		{
			selected = player.pos;
			front	 = player.pos;
			break;
		}
	}
}

class MCText
{
	public:
		// 8x8 char, ASCII
		agl::Texture	atlas;
		agl::Rectangle &rect;

		int kerning[256];

		static float scale;

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

		void draw(agl::RenderWindow &window, std::string text, agl::Vec<float, 2> pos, agl::Color color)
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

float MCText::scale = 1;

class CommandBox : public agl::Drawable
{
	public:
		agl::Rectangle &rect;
		agl::Texture   &blank;

		std::string cmd = "";

		std::vector<Block> &blocks;

		agl::Vec<int, 2> &winSize;

		bool commit = false;

		GameState &focused;

		MCText &text;

		int pallete = 1;

		CommandBox(agl::Rectangle &rect, MCText &text, agl::Texture &blank, std::vector<Block> &blocks,
				   agl::Vec<int, 2> &winSize, GameState &focused)
			: rect(rect), blank(blank), blocks(blocks), winSize(winSize), focused(focused), text(text)
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

			text.draw(win, " > " + cmd, {0, 0}, agl::Color::White);

			int offset = text.getHeight() + 10;

			for (unsigned int i = 0; i < blocks.size(); i++)
			{
				auto &b = blocks[i];

				if (b.name.substr(0, cmd.length()) == cmd)
				{
					rect.setTexture(&blank);
					rect.setTextureTranslation({0, 0});
					rect.setTextureScaling({0, 0});
					rect.setColor(agl::Color::Black);

					rect.setPosition({0, offset, 0});

					win.drawShape(rect);
					text.draw(win, b.name, {0, offset, 0}, agl::Color::Gray);

					offset += text.getHeight() + 10;
				}

				if (b.name.substr(0, cmd.size()) == cmd && commit)
				{
					commit	= false;
					pallete = i;
					focused = GameState::RUNNING;
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

#ifdef _WIN32
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#endif

	agl::RenderWindow window;
	window.setup({1920, 1080}, "CaveGame");
	// window.setClearColor({0x78, 0xA7, 0xFF});
	window.setClearColor(agl::Color::Black);

	agl::Vec<int, 2> windowSize;

	window.GLEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);

	glDepthFunc(GL_LEQUAL);

	window.setSwapInterval(1);

	agl::Event event;
	event.setWindow(window);

	std::cout << "Shader Compilation" << '\n';

	ax::Program worldShader(ax::Shader("./shader/worldVert.glsl", GL_VERTEX_SHADER),
							ax::Shader("./shader/worldGeom.glsl", GL_GEOMETRY_SHADER),
							ax::Shader("./shader/worldFrag.glsl", GL_FRAGMENT_SHADER));

	ax::Program uiShader(ax::Shader("./shader/frag.glsl", GL_FRAGMENT_SHADER),
						 ax::Shader("./shader/uivert.glsl", GL_VERTEX_SHADER));

	ax::Program skyShader(ax::Shader("./shader/skyFrag.glsl", GL_FRAGMENT_SHADER),
						  ax::Shader("./shader/skyVert.glsl", GL_VERTEX_SHADER));

	std::cout << "Loading Assets And Textures" << '\n';

	Atlas atlas("./resources/java/assets/minecraft/textures/block/");

	Image tintTextureGrass;
	tintTextureGrass.load("./resources/java/assets/minecraft/textures/colormap/grass.png");

	Image tintTextureFoliage;
	tintTextureFoliage.load("./resources/java/assets/minecraft/textures/colormap/foliage.png");

	std::vector<Block>		   blockDefs;
	std::map<std::string, int> blockNameToDef;

	agl::Texture elementDataTexture;

	agl::Texture blank;
	blank.setBlank();

	agl::Font font;
	font.setup("./font/font.ttf", 24);

	std::cout << "Loading Config" << '\n';

	Config config;

	{
		std::fstream fs("./config", std::ios::in);
		recurse(Input(fs), config, "config");
	}

	window.setFPS(config.fpsCap);

	recurse(Output(std::cout), config, "config");

	std::cout << "Loading Block Data" << '\n';

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

		std::vector<float> databuf;
		databuf.reserve(128 * 128 * 4);

		for (auto &e : jsonPairs)
		{
			blockNameToDef["minecraft:" + e.first] = blockDefs.size();
			blockDefs.emplace_back(atlas, e.first, jsonPairs, tintTextureGrass, tintTextureFoliage);

			for (auto &e : blockDefs.back().elements)
			{
				for (auto i : e.elementDataArray)
				{
					// 1 face = 1/2 pixel
					// full = 3 pixels
					databuf.push_back(*(float *)&i);
					// databuf.push_back(2);
				}
			}
		}

		databuf.resize(128 * 128 * 4);

		elementDataTexture.genTexture();
		elementDataTexture.bind(elementDataTexture);

		// 128*128
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 128, 128, 0, GL_RGBA, GL_FLOAT, &databuf[0]);

		elementDataTexture.useNearestFiltering();
	}

	std::cout << "Misc Work" << '\n';

	agl::Rectangle blankRect;
	blankRect.setTexture(&blank);

	MCText text(blankRect);
	text.scale = 2;

	World world;
	world.setBasics(blockDefs);

	GameState gamestate = GameState::RUNNING;

	CommandBox cmdBox(blankRect, text, blank, blockDefs, windowSize, gamestate);

	cmdBox.pallete = world.cobblestone;

	agl::Vec<int, 3> selected;
	agl::Vec<int, 3> front;

	Listener lclis;
	Listener rclis;

	Player player;

	hideCursor(window);

	WorldMesh wm(world, blockDefs);

	bool closeThread = false;

	std::thread thread(buildThread, std::ref(wm), std::ref(closeThread));

	{
		worldShader.use();
		auto id = worldShader.getUniformLocation("elementDataSampler");

		glUniform1i(id, 1);

		id = worldShader.getUniformLocation("textureSampler");

		glUniform1i(id, 0);
	}

	std::cout << "entering" << '\n';

	float currentFrame = 0;

	while (!event.windowClose())
	{
		if (config.showFps)
		{
			static Timer t;
			t.stop();
			std::cout << "FPS: " << 1000. / (t.get<std::chrono::milliseconds>()) << '\n';
			t.start();
		}
		static int milliDiff = 0;
		int		   start	 = getMillisecond();

		event.poll();

		windowSize = window.getState().size;

		window.clear();

		if (gamestate == GameState::RUNNING || gamestate == GameState::PAUSE)
		{
			{
				agl::Mat4f proj;
				agl::Mat4f trans;
				proj.ortho(0, windowSize.x, windowSize.y, 0, 0.1, 100);
				trans.lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});

				skyShader.use();
				window.getShaderUniforms(skyShader);
				window.updateMvp(proj * trans);

				int id = skyShader.getUniformLocation("time");
				skyShader.setUniform(id, currentFrame);
				id = skyShader.getUniformLocation("rotx");
				skyShader.setUniform(id, player.rot.x);
				id = skyShader.getUniformLocation("roty");
				skyShader.setUniform(id, player.rot.y);

				blankRect.setSize(windowSize);
				blankRect.setPosition({0, 0, 0});

				glDisable(GL_DEPTH_TEST);

				window.drawShape(blankRect);

				glEnable(GL_DEPTH_TEST);
			}

			worldShader.use();
			window.getShaderUniforms(worldShader);
			{
				agl::Mat<float, 4> tran;
				tran.translate(player.pos * -1 - agl::Vec{0.f, 1.62f, 0.f});

				agl::Mat<float, 4> rot;
				rot.rotate({agl::radianToDegree(player.rot.x), agl::radianToDegree(player.rot.y),
							agl::radianToDegree(player.rot.z)});

				agl::Mat<float, 4> proj;
				proj.perspective(PI / 2, (float)windowSize.x / windowSize.y, 0.1, 10000);

				window.updateMvp(proj * rot * tran);

				int id = worldShader.getUniformLocation("time");
				worldShader.setUniform(id, currentFrame);
				id = worldShader.getUniformLocation("rotx");
				worldShader.setUniform(id, player.rot.x);
				id = worldShader.getUniformLocation("roty");
				worldShader.setUniform(id, player.rot.y);
			}

			glActiveTexture(GL_TEXTURE0 + 1);
			agl::Texture::bind(elementDataTexture);
			glActiveTexture(GL_TEXTURE0 + 0);
			agl::Texture::bind(atlas.texture);

			wm.mutPos.lock();
			wm.playerChunkPos	= player.pos / 16;
			wm.playerChunkPos.y = 0;
			wm.mutPos.unlock();

			wm.draw(window);

			glDisable(GL_DEPTH_TEST);

			agl::Mat4f proj;
			agl::Mat4f trans;
			proj.ortho(0, windowSize.x, windowSize.y, 0, 0.1, 100);
			trans.lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});

			uiShader.use();
			window.getShaderUniforms(uiShader);
			window.updateMvp(proj * trans);

			if (!event.isKeyPressed(agl::Key::F1))
			{
				blankRect.setTextureScaling({1, 1, 1});
				blankRect.setTextureTranslation({0, 0, 0});
				blankRect.setTexture(&blank);
				blankRect.setColor(agl::Color::Red);
				blankRect.setRotation({0, 0, 0});
				blankRect.setSize({3, 3});
				blankRect.setPosition(windowSize / 2 - blankRect.getSize() / 2);
				window.drawShape(blankRect);
			}

			if (gamestate == GameState::PAUSE)
			{
				blankRect.setTexture(&blank);
				blankRect.setColor({0, 0, 0, 127});
				blankRect.setSize(windowSize);
				blankRect.setPosition({0, 0, 0});
				window.drawShape(blankRect);
			}
		}
		else if (gamestate == GameState::CMD)
		{
			agl::Mat4f proj;
			agl::Mat4f trans;
			proj.ortho(0, windowSize.x, windowSize.y, 0, 0.1, 100);
			trans.lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});

			uiShader.use();
			window.getShaderUniforms(uiShader);
			window.updateMvp(proj * trans);

			if (gamestate == GameState::CMD)
			{
				window.draw(cmdBox);
			}
		}

		glEnable(GL_DEPTH_TEST);

		window.display();

		static int frame = 0;
		frame++;

		if (gamestate == GameState::RUNNING)
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

#ifdef __linux__
			Window win = 0;
			int	   i   = 0;
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
#endif

#ifdef _WIN32
			{
				int focused = glfwGetWindowAttrib(window.baseWindow.window, GLFW_FOCUSED);

				if (focused)
				{
					if ((mousePos - (windowSize / 2)).length() > std::min(windowSize.x / 2, windowSize.y / 2))
					{
						glfwSetCursorPos(window.baseWindow.window, windowSize.x / 2, windowSize.y / 2);

						oldMousePos = windowSize / 2;
					}
				}
			}
#endif

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

			if (event.isKeyPressed(agl::Key::LeftShift))
			{
				player.sprinting = true;
			}
			else
			{
				player.sprinting = false;
			}

			player.grounded = false;

			movePlayer(player, acc, world);

			lclis.update(event.isPointerButtonPressed(agl::Button::Left));
			rclis.update(event.isPointerButtonPressed(agl::Button::Right));

			if (rclis.ls == ListenState::First && !(front == player.pos))
			{
				auto block	= world.getBlock(front);
				block->type = cmdBox.pallete;

				for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
				{
					if (it->pos == agl::Vec{front.x / 16, 0, front.z / 16})
					{
						it->update = true;
						break;
					}
				}
			}
			if (lclis.ls == ListenState::First && gamestate)
			{
				auto block	= world.getBlock(selected);
				block->type = world.air;

				for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
				{
					if (it->pos == agl::Vec{selected.x / 16, 0, selected.z / 16})
					{
						it->update = true;
						break;
					}
				}
			}

			if (event.isKeyPressed(agl::Key::T))
			{
				gamestate = GameState::CMD;
			}
		}
		else if (gamestate == GameState::CMD)
		{
			if (event.isKeyPressed(agl::Key::Escape))
			{
				gamestate = GameState::RUNNING;
			}
			else
			{
				cmdBox.update(event.keybuffer);
			}
		}
		else if (gamestate == GameState::PAUSE)
		{
		}

		window.setViewport(0, 0, windowSize.x, windowSize.y);

		agl::Vec<int, 3> chunkPos = player.pos / 16;
		chunkPos.y				  = 0;
		// std::cout << chunkPos << '\n';

		currentFrame++;
	}

	font.deleteFont();

	blank.deleteTexture();

	tintTextureGrass.free();
	tintTextureFoliage.free();

	window.close();

	closeThread = true;
	thread.join();

	return 0;
}
