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

#define WALKACC (1 / 3.)
#define BLCKFRC 0.6

#define BASESPEED WALKVELPERTICK

class Listener
{
	private:
		std::function<void()> first;
		std::function<void()> hold;
		std::function<void()> last;
		bool				  pastState = false;

	public:
		Listener(std::function<void()> first, std::function<void()> hold, std::function<void()> last);
		void update(bool state);
};

Listener::Listener(std::function<void()> first, std::function<void()> hold, std::function<void()> last)
{
	this->first = first;
	this->hold	= hold;
	this->last	= last;
}

void Listener::update(bool state)
{
	if (state)
	{
		if (pastState)
		{
			hold();
		}
		else
		{
			first();

			pastState = true;
		}
	}
	else if (pastState)
	{
		last();
		pastState = false;
	}
}

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
		agl::Vec<float, 3> pos = {1.1, 2.5, 10.1};
		agl::Vec<float, 3> rot = {0.1, 0.1, 0};
		agl::Vec<float, 3> vel = {0, 0, 0};

		float friction = BLCKFRC;

		bool sneaking  = true;
		bool sprinting = false;

		void update()
		{
		}
};

class World
{
	public:
		std::vector<std::vector<std::vector<bool>>> blocks;
		agl::Vec<int, 3>							size;

		World(agl::Vec<int, 3> size) : size(size)
		{
			blocks.resize(size.x);

			for (auto &b : blocks)
			{
				b.resize(size.y);
				for (auto &b : b)
				{
					b.resize(size.z);
				}
			}
		}

		bool getAtPos(agl::Vec<int, 3> pos)
		{
			return blocks.at(pos.x).at(pos.y).at(pos.z);
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

void movePlayer(Player &player, agl::Vec<float, 3> acc, float vel)
{
	player.vel *= BLCKFRC * 0.91;

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
}

int main()
{
	printf("Starting AGL\n");

	agl::RenderWindow window;
	window.setup({1920, 1080}, "CaveGame");
	window.setClearColor(agl::Color::Gray);
	window.setFPS(0);

	agl::Vec<float, 2> windowSize;

	window.GLEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);

	window.setSwapInterval(1);

	agl::Event event;
	event.setWindow(window);

	agl::Shader simpleShader;
	simpleShader.loadFromFile("./shader/vert.glsl", "./shader/frag.glsl");

	agl::Shader gridShader;
	gridShader.loadFromFile("./shader/gridvert.glsl", "./shader/grid.glsl");

	agl::Shader viteShader;
	viteShader.loadFromFile("./shader/viteVert.glsl", "./shader/vite.glsl");

	agl::Shader menuShader;
	menuShader.loadFromFile("./shader/menuVert.glsl", "./shader/menu.glsl");

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
	font.setup("./font/font.ttf", 16);

	agl::Font smallFont;
	smallFont.setup("./font/font.ttf", 12);

	agl::Rectangle blankRect;
	blankRect.setTexture(&blank);

	blankRect.setPosition({0, 0, 0});
	blankRect.setSize({10, 20, 0});
	blankRect.setRotation({0, 0, 0});
	blankRect.setColor(agl::Color::Red);

	World world({20, 5, 20});

	world.blocks[3 + 5][3][3] = true;
	world.blocks[3 + 5][2][3] = true;
	world.blocks[3 + 5][4][3] = true;
	world.blocks[2 + 5][2][3] = true;
	world.blocks[4 + 5][2][3] = true;
	/*
	 *   # #
	 *   # #
	 * #     #
	 *  #####
	 */
	world.blocks[2][4][0] = true;
	world.blocks[2][3][0] = true;
	world.blocks[4][4][0] = true;
	world.blocks[4][3][0] = true;

	world.blocks[0][1][0] = true;
	world.blocks[1][0][0] = true;
	world.blocks[2][0][0] = true;
	world.blocks[3][0][0] = true;
	world.blocks[4][0][0] = true;
	world.blocks[5][0][0] = true;
	world.blocks[6][1][0] = true;

	agl::Cuboid cube;
	cube.setSize({1, 1, 1});
	cube.setPosition({0, 0, 0});
	cube.setRotation({0, 0, 0});
	cube.setTexture(&blank);
	cube.setColor(agl::Color::White);

	window.getShaderUniforms(simpleShader);
	simpleShader.use();

	agl::Vec<int, 3> selected;
	agl::Vec<int, 3> front;

	Player player;

	hideCursor(window);

	while (!event.windowClose())
	{
		static int milliDiff = 0;
		int		   start	 = getMillisecond();

		event.poll();

		windowSize = window.getState().size;

		window.clear();

		for (int x = 0; x < world.blocks.size(); x++)
		{

			for (int y = 0; y < world.blocks[x].size(); y++)
			{
				for (int z = 0; z < world.blocks[x][y].size(); z++)
				{
					if (world.blocks[x][y][z])
					{
						cube.setPosition({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)});
						cube.setColor({(unsigned char)(255 * ((float)x / world.size.x)),
									   (unsigned char)(255 * ((float)y / world.size.y)),
									   (unsigned char)(255 * ((float)z / world.size.z))});

						if (selected == agl::Vec{x, y, z})
						{
							cube.setColor(agl::Color::Green);
						}
						window.drawShape(cube);
					}
				}
			}
		}

		glDisable(GL_DEPTH_TEST);

		{
			agl::Mat4f proj;
			agl::Mat4f trans;
			proj.ortho(0, windowSize.x, windowSize.y, 0, 0.1, 100);
			trans.lookAt({0, 0, 10}, {0, 0, 0}, {0, 1, 0});

			window.updateMvp(proj * trans);

			blankRect.setPosition(windowSize / 2 - agl::Vec{3, 3});
			blankRect.setSize({3, 3});
			window.drawShape(blankRect);
		}

		glEnable(GL_DEPTH_TEST);

		window.display();

		static int frame = 0;
		frame++;

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

		{
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
		}

		{
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

			movePlayer(player, acc, WALKACC);
		}

		{
			agl::Mat<float, 4> tran;
			tran.translate(player.pos * -1);

			agl::Mat<float, 4> rot;
			rot.rotate({agl::radianToDegree(player.rot.x), agl::radianToDegree(player.rot.y),
						agl::radianToDegree(player.rot.z)});

			agl::Mat<float, 4> proj;
			proj.perspective(PI / 2, windowSize.x / windowSize.y, 0.1, 100);

			window.updateMvp(proj * rot * tran);
		}

		{
			agl::Vec<float, 3> dir = {-sin(player.rot.y) * cos(player.rot.x), -sin(player.rot.x),
									  -cos(player.rot.y) * cos(player.rot.x)};

			agl::Vec<float, 3> blockPos = player.pos;

			selected = blockPos;
			front = blockPos;

			while (true)
			{
				if (blockPos.x >= world.size.x || blockPos.x < 0 || blockPos.y >= world.size.y || blockPos.y < 0 ||
					blockPos.z >= world.size.z || blockPos.z < 0)
				{
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

		if(event.keybuffer.find("e") != std::string::npos)
		{
			world.blocks[front.x][front.y][front.z] = true;
		}
		if(event.keybuffer.find("q") != std::string::npos)
		{
			world.blocks[selected.x][selected.y][selected.z] = false;
		}

		window.setViewport(0, 0, windowSize.x, windowSize.y);
	}

	font.deleteFont();

	foodTexture.deleteTexture();
	creatureBodyTexture.deleteTexture();
	creatureExtraTexture.deleteTexture();
	eggTexture.deleteTexture();
	blank.deleteTexture();

	simpleShader.deleteProgram();
	gridShader.deleteProgram();

	window.close();

	return 0;
}
