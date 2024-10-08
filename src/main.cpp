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
#include "../inc/CommandBox.hpp"
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
		int currentPallete = 0;
		int pallete[9];

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
	// correct side pos

	// +X
	{
		int x = player.pos.x + 0.3;
		int z = player.pos.z;

		if (x != int(player.pos.x))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				if (world.getAtPos({x, y, z}))
				{
					player.vel.x = 0;
					player.pos.x = x - .3;
				}
			}
		}
	}
	// -X
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z;

		if (x != int(player.pos.x))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				if (world.getAtPos({x, y, z}))
				{
					player.vel.x = 0;
					player.pos.x = x + 1.3;
				}
			}
		}
	}
	// +Z
	{
		int x = player.pos.x;
		int z = player.pos.z + 0.3;

		if (z != int(player.pos.z))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				if (world.getAtPos({x, y, z}))
				{
					player.vel.z = 0;
					player.pos.z = z - .3;
				}
			}
		}
	}
	// -Z
	{
		int x = player.pos.x;
		int z = player.pos.z - 0.3;

		if (z != int(player.pos.z))
		{
			for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
			{
				if (world.getAtPos({x, y, z}))
				{
					player.vel.z = 0;
					player.pos.z = z + 1.3;
				}
			}
		}
	}

	// diagonal

	if (!(agl::Vec<int, 3>{(player.pos.x - .3), player.pos.y, (player.pos.z - .3)} == player.pos))
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z - 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.vel.x) > fabs(player.vel.z))
				{
					player.vel.x = 0;
					player.pos.x = x + 1.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z + 1.3;
				}
			}
		}
	}
	if (!(agl::Vec<int, 3>{(player.pos.x + 0.3), player.pos.y, (player.pos.z - .3)} == player.pos))
	{
		int x = player.pos.x + 0.3;
		int z = player.pos.z - 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.vel.x) > fabs(player.vel.z))
				{
					player.vel.x = 0;
					player.pos.x = x - 0.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z + 1.3;
				}
			}
		}
	}
	if (!(agl::Vec<int, 3>{(player.pos.x - .3), player.pos.y, (player.pos.z + .3)} == player.pos))
	{
		int x = player.pos.x - 0.3;
		int z = player.pos.z + 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.vel.x) > fabs(player.vel.z))
				{
					player.vel.x = 0;
					player.pos.x = x + 1.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z - 0.3;
				}
			}
		}
	}
	if (!(agl::Vec<int, 3>{(player.pos.x + .3), player.pos.y, (player.pos.z + .3)} == player.pos))
	{
		int x = player.pos.x + 0.3;
		int z = player.pos.z + 0.3;

		for (int y = player.pos.y; y < player.pos.y + 1.8; y++)
		{
			if (world.getAtPos({x, y, z}))
			{
				if (fabs(player.vel.x) > fabs(player.vel.z))
				{
					player.vel.x = 0;
					player.pos.x = x - 0.3;
				}
				else
				{
					player.vel.z = 0;
					player.pos.z = z - 0.3;
				}
			}
		}
	}

	/*// diagonal*/
	/**/
	/*int xoff = 0;*/
	/*if (player.pos.x - (int)player.pos.x > .7)*/
	/*{*/
	/*	xoff = 1;*/
	/*}*/
	/*else if (player.pos.x - (int)player.pos.x < .3)*/
	/*{*/
	/*	xoff = -1;*/
	/*}*/
	/*int zoff = 0;*/
	/*if (player.pos.z - (int)player.pos.z > .7)*/
	/*{*/
	/*	zoff = 1;*/
	/*}*/
	/*else if (player.pos.z - (int)player.pos.z < .3)*/
	/*{*/
	/*	zoff = -1;*/
	/*}*/
	/**/
	/*if (xoff != 0 && zoff != 0)*/
	/*{*/
	/*	int x = player.pos.x + xoff;*/
	/*	int z = player.pos.z + zoff;*/
	/**/
	/*	for (int y = player.pos.y; y < player.pos.y + 1.8; y++)*/
	/*	{*/
	/*		if (world.getAtPos({x, y, z}))*/
	/*		{*/
	/*			if (fabs(player.vel.x) > fabs(player.vel.z))*/
	/*			{*/
	/*				if (player.vel.x > 0 && xoff > 0)*/
	/*				{*/
	/*					player.vel.x = 0;*/
	/*					player.pos.x = x - .3;*/
	/*				}*/
	/*				else if (player.vel.x < 0 && xoff < 0)*/
	/*				{*/
	/*					player.vel.x = 0;*/
	/*					player.pos.x = x + 1.3;*/
	/*				}*/
	/*			}*/
	/*			else*/
	/*			{*/
	/*				if (player.vel.z > 0 && zoff > 0)*/
	/*				{*/
	/*					player.vel.z = 0;*/
	/*					player.pos.z = z - .3;*/
	/*				}*/
	/*				else if (player.vel.z < 0 && zoff < 0)*/
	/*				{*/
	/*					player.vel.z = 0;*/
	/*					player.pos.z = z + 1.3;*/
	/*				}*/
	/*			}*/
	/*		}*/
	/*	}*/
	/*}*/
}

void correctPositionY(Player &player, World &world)
{
	player.grounded = false;

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
	player.vel.y *= 0.98;
	player.vel.y += GRAVACC;

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

	player.pos.y += player.vel.y;

	correctPositionY(player, world);

	player.pos.x += player.vel.x;
	player.pos.z += player.vel.z;

	correctPositionXZ(player, world);
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

void toggleFullscreen(Display *display, Window window)
{
	Atom wmState	= XInternAtom(display, "_NET_WM_STATE", False);
	Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);

	XEvent xev				 = {0};
	xev.type				 = ClientMessage;
	xev.xclient.window		 = window;
	xev.xclient.message_type = wmState;
	xev.xclient.format		 = 32;
	xev.xclient.data.l[0]	 = 2; // _NET_WM_STATE_ADD
	xev.xclient.data.l[1]	 = fullscreen;
	xev.xclient.data.l[2]	 = 0; // no second property to toggle

	XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask, &xev);
}

void save(std::string path, World &world)
{
	std::fstream fs(path, std::ios::out);

	for (auto e : world.loadedChunks)
	{
		fs.write(reinterpret_cast<char *>((int *)&e.first.x), sizeof(int));
		fs.write(reinterpret_cast<char *>((int *)&e.first.y), sizeof(int));
		fs.write(reinterpret_cast<char *>((int *)&e.first.z), sizeof(int));

		for (int x = 0; x < 16; x++)
		{
			for (int y = 0; y < 384; y++)
			{
				for (int z = 0; z < 16; z++)
				{
					unsigned int id = e.second.blocks[x][y][z].type;
					fs.write(reinterpret_cast<char *>(&id), sizeof(id));
				}
			}
		}
	}

	fs.close();
}

void load(std::string path, World &world)
{
	std::fstream fs(path, std::ios::in);

	while (!fs.eof())
	{
		int x = 0;
		int y = 0;
		int z = 0;
		fs.read(reinterpret_cast<char *>(&x), sizeof(int));
		fs.read(reinterpret_cast<char *>(&y), sizeof(int));
		fs.read(reinterpret_cast<char *>(&z), sizeof(int));

		auto &chunk = world.loadedChunks[{x, y, z}];

		for (int x = 0; x < 16; x++)
		{
			for (int y = 0; y < 384; y++)
			{
				for (int z = 0; z < 16; z++)
				{
					unsigned int id = 0;
					fs.read(reinterpret_cast<char *>(&id), sizeof(id));

					chunk.blocks[x][y][z].type = id;
				}
			}
		}
	}

	fs.close();
}

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

	ax::Program blockShader(ax::Shader("./shader/blockVert.glsl", GL_VERTEX_SHADER),
							ax::Shader("./shader/blockFrag.glsl", GL_FRAGMENT_SHADER));

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
	std::vector<std::string>   blockList;

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
		}
	}

	for (auto &e : blockNameToDef)
	{
		blockList.emplace_back(e.first.substr(10));
	}

	std::cout << "Misc Work" << '\n';

	agl::Rectangle blankRect;
	blankRect.setTexture(&blank);

	MCText text(blankRect);
	text.scale = 2;

	World world;
	world.setBasics(blockDefs);

	world.blockDefs = &blockDefs;

	Player player;

	GameState gamestate = GameState::RUNNING;

	CommandBox cmdBox(blankRect, text, blank, windowSize);

	agl::Vec<int, 3> selected;
	agl::Vec<int, 3> front;

	Listener lclis;
	Listener rclis;

	for (auto &e : player.pallete)
	{
		e = world.cobblestone;
	}

	hideCursor(window);

	WorldMesh wm(world, blockDefs);

	bool closeThread = false;

	std::thread *thread = new std::thread(buildThread, std::ref(wm), std::ref(closeThread));

	cmdBox.functions = {
		CommandFunction{"set",
						{&blockList},
						[&](std::vector<std::string> v) {
							std::string &name = v[1];

							if (blockNameToDef.count("minecraft:" + name) != 0)
							{
								player.pallete[player.currentPallete] = blockNameToDef["minecraft:" + name];
							}

							return;
						}},
		CommandFunction{"resetpos",
						{},
						[&](std::vector<std::string> v) {
							player.pos = {16 * 16, 150, 16 * 16};

							return;
						}},
		CommandFunction{"togglefullscreen",
						{},
						[&](std::vector<std::string> v) {
							toggleFullscreen(window.baseWindow.dpy, window.baseWindow.win);

							return;
						}},
		CommandFunction{"save",
						{},
						[&](std::vector<std::string> v) {
							save(v[1], world);

							return;
						}},
		CommandFunction{"load",
						{},
						[&](std::vector<std::string> v) {
							closeThread = true;
							thread->join();
							closeThread = false;
							wm.clear();
							world.loadedChunks.clear();
							player.pos = {16 * 16, 150, 16 * 16};
							load(v[1], world);

							thread = new std::thread(buildThread, std::ref(wm), std::ref(closeThread));

							return;
						}},
	};

	cmdBox.setCommands();

	{
		int id = blockShader.getUniformLocation("textureSampler");

		glUniform1i(id, 0);
	}

	std::cout << "entering" << '\n';

	float currentFrame = 0;

	bool windowFocus = true;

	while (!event.windowClose())
	{
		{
			int	   revert = 0;
			Window win;
			XGetInputFocus(window.baseWindow.dpy, &win, &revert);

			windowFocus = win == window.baseWindow.win;
		}

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

			blockShader.use();
			window.getShaderUniforms(blockShader);
			{
				agl::Mat<float, 4> tran;
				tran.translate(player.pos * -1 - agl::Vec{0.f, 1.62f, 0.f});

				agl::Mat<float, 4> rot;
				rot.rotate({agl::radianToDegree(player.rot.x), agl::radianToDegree(player.rot.y),
							agl::radianToDegree(player.rot.z)});

				agl::Mat<float, 4> proj;
				proj.perspective(PI / 2, (float)windowSize.x / windowSize.y, 0.1, 10000);

				window.updateMvp(proj * rot * tran);

				/*int id = worldShader.getUniformLocation("time");*/
				/*worldShader.setUniform(id, currentFrame);*/
				/*id = worldShader.getUniformLocation("rotx");*/
				/*worldShader.setUniform(id, player.rot.x);*/
				/*id = worldShader.getUniformLocation("roty");*/
				/*worldShader.setUniform(id, player.rot.y);*/
			}

			glActiveTexture(GL_TEXTURE0);
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
				blankRect.setColor(agl::Color::White);
				blankRect.setRotation({0, 0, 0});
				blankRect.setSize({3, 3});
				blankRect.setPosition(windowSize / 2 - blankRect.getSize() / 2);
				window.drawShape(blankRect);

				blankRect.setColor(agl::Color{0, 0, 0, 127});
				blankRect.setPosition({4, 4});
				blankRect.setSize({200, 216});

				window.drawShape(blankRect);

				int i = 0;

				for (auto &e : player.pallete)
				{
					std::string name = blockDefs[e].name;

					if (i == player.currentPallete)
					{
						text.draw(window, std::to_string(i + 1) + ") " + name, {10, 10 + 0 + (i * 24)},
								  agl::Color{0x3c, 0x3c, 0x00, 0xFF});
						text.draw(window, std::to_string(i + 1) + ") " + name, {8, 8 + 0 + (i * 24)},
								  agl::Color{0xfd, 0xfe, 0x00, 0xFF});
					}
					else
					{
						text.draw(window, std::to_string(i + 1) + ") " + name, {10, 10 + 0 + (i * 24)},
								  agl::Color{0x34, 0x34, 0x34, 0xFF});
						text.draw(window, std::to_string(i + 1) + ") " + name, {8, 8 + 0 + (i * 24)},
								  agl::Color{0xDE, 0xDE, 0xDE, 0xFF});
					}

					i++;
				}
			}

			if (gamestate == GameState::PAUSE)
			{
				blankRect.setTexture(&blank);
				blankRect.setColor({0, 0, 0, 127});
				blankRect.setSize(windowSize);
				blankRect.setPosition({0, 0, 0});
				window.drawShape(blankRect);
			}
			if (gamestate == GameState::CMD)
			{
				if (cmdBox.commit)
				{
					std::vector<std::string> array	= splitString(cmdBox.cmd, ' ');
					int						 funcid = -1;

					if (array.size() != 0)
					{
						for (int i = 0; i < cmdBox.functions.size(); i++)
						{
							if (array[0] == cmdBox.functions[i].name)
							{
								funcid = i;
								break;
							}
						}
					}

					if (funcid != -1)
					{
						cmdBox.functions[funcid].func(array);
					}

					cmdBox.cmd	  = "";
					cmdBox.commit = false;
					gamestate	  = GameState::RUNNING;
				}
				else
				{
					window.draw(cmdBox);
				}
			}
		}

		glEnable(GL_DEPTH_TEST);

		window.display();

		static int frame = 0;
		frame++;

		if (gamestate == GameState::RUNNING)
		{
			if (windowFocus)
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
			}

			updateSelected(player, selected, front, world);

			agl::Vec<float, 3> acc;

			if (windowFocus)
			{
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

				if (event.isKeyPressed(agl::Key::Num1))
				{
					player.currentPallete = 0;
				}
				if (event.isKeyPressed(agl::Key::Num2))
				{
					player.currentPallete = 1;
				}
				if (event.isKeyPressed(agl::Key::Num3))
				{
					player.currentPallete = 2;
				}
				if (event.isKeyPressed(agl::Key::Num4))
				{
					player.currentPallete = 3;
				}
				if (event.isKeyPressed(agl::Key::Num5))
				{
					player.currentPallete = 4;
				}
				if (event.isKeyPressed(agl::Key::Num6))
				{
					player.currentPallete = 5;
				}
				if (event.isKeyPressed(agl::Key::Num7))
				{
					player.currentPallete = 6;
				}
				if (event.isKeyPressed(agl::Key::Num8))
				{
					player.currentPallete = 7;
				}
				if (event.isKeyPressed(agl::Key::Num8))
				{
					player.currentPallete = 8;
				}
				if (event.isKeyPressed(agl::Key::Num9))
				{
					player.currentPallete = 9;
				}
			}

			player.grounded = false;

			movePlayer(player, acc, world);

			if (windowFocus)
			{
				lclis.update(event.isPointerButtonPressed(agl::Button::Left));
				rclis.update(event.isPointerButtonPressed(agl::Button::Right));

				if (rclis.ls == ListenState::First && !(front == player.pos))
				{
					world.setBlock(front, BlockData{(unsigned int)player.pallete[player.currentPallete]});

					for (int x = -1; x < 2; x++)
					{
						for (int y = -1; y < 2; y++)
						{
							for (int z = -1; z < 2; z++)
							{
								auto global = front + agl::Vec<int, 3>{x, y, z};
								auto cpos	= global / 16;
								cpos.y		= 0;

								auto it = world.loadedChunks.find(cpos);

								if (it == world.loadedChunks.end())
								{
									continue;
								}

								auto local = global - (cpos * 16);

								it->second.blocks[local.x][local.y][local.z].update = true;
							}
						}
					}

					std::vector<agl::Vec<int, 3>> polluted;
					polluted.push_back({front.x / 16, 0, front.z / 16});

					if (front.x % 16 == 15)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z});
					}
					if (front.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x, polluted[0].y, polluted[0].z + 1});
					}
					if (front.x % 16 == 0)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z});
					}
					if (front.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x, polluted[0].y, polluted[0].z - 1});
					}
					if (front.x % 16 == 15 && front.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z + 1});
					}
					if (front.x % 16 == 0 && front.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z + 1});
					}
					if (front.x % 16 == 15 && front.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z - 1});
					}
					if (front.x % 16 == 0 && front.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z - 1});
					}

					for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
					{
						for (auto i = polluted.begin(); i != polluted.end(); i++)
						{
							if (it->pos == *i)
							{
								it->update = true;
								polluted.erase(i);
								break;
							}
						}
					}
				}
				if (lclis.ls == ListenState::First && gamestate)
				{
					world.setBlock(selected, BlockData{(unsigned int)world.air});

					for (int x = -1; x < 2; x++)
					{
						for (int y = -1; y < 2; y++)
						{
							for (int z = -1; z < 2; z++)
							{
								auto global = selected + agl::Vec<int, 3>{x, y, z};
								auto cpos	= global / 16;
								cpos.y		= 0;

								auto it = world.loadedChunks.find(cpos);

								if (it == world.loadedChunks.end())
								{
									continue;
								}

								auto local = global - (cpos * 16);

								it->second.blocks[local.x][local.y][local.z].update = true;
							}
						}
					}

					std::vector<agl::Vec<int, 3>> polluted;
					polluted.push_back({selected.x / 16, 0, selected.z / 16});

					if (selected.x % 16 == 15)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z});
					}
					if (selected.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x, polluted[0].y, polluted[0].z + 1});
					}
					if (selected.x % 16 == 0)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z});
					}
					if (selected.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x, polluted[0].y, polluted[0].z + 1});
					}
					if (selected.x % 16 == 15 && selected.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z + 1});
					}
					if (selected.x % 16 == 0 && selected.z % 16 == 15)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z + 1});
					}
					if (selected.x % 16 == 15 && selected.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x + 1, polluted[0].y, polluted[0].z - 1});
					}
					if (selected.x % 16 == 0 && selected.z % 16 == 0)
					{
						polluted.push_back({polluted[0].x - 1, polluted[0].y, polluted[0].z - 1});
					}

					for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
					{
						for (auto i = polluted.begin(); i != polluted.end(); i++)
						{
							if (it->pos == *i)
							{
								it->update = true;
								polluted.erase(i);
								break;
							}
						}
					}
				}

				if (event.isKeyPressed(agl::Key::T))
				{
					gamestate = GameState::CMD;
				}
			}
		}
		else if (gamestate == GameState::CMD)
		{
			if (windowFocus)
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
		}
		else if (gamestate == GameState::PAUSE)
		{
		}

		window.setViewport(0, 0, windowSize.x, windowSize.y);

		agl::Vec<int, 3> chunkPos = player.pos / 16;
		chunkPos.y				  = 0;
		/*std::cout << player.pos << '\n';*/

		currentFrame++;
	}

	closeThread = true;
	thread->join();
	delete thread;

	wm.clear();

	font.deleteFont();

	blank.deleteTexture();

	tintTextureGrass.free();
	tintTextureFoliage.free();

	window.close();

	std::cout << "end" << '\n';

	return 0;
}
