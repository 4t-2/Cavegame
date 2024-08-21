#include "../inc/Mesh.hpp"

void buildThread(WorldMesh &wm, bool &closeThread)
{
	agl::Vec<int, 3> playerChunkPos;
	bool			 changesMade;

	auto buildChunk = [&](int x, int y) {
		agl::Vec<int, 3> cursor = {x, 0, y};

		if (cursor.length() > RENDERDIST)
		{
			return 0;
		}

		cursor += playerChunkPos;

		if (wm.world.loadedChunks.count(cursor) == 0)
		{
			wm.world.createChunk(cursor);
		}

		for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
		{
			if (it->pos == cursor && !it->update)
			{
				goto skip;
			}
		}

		wm.toAdd.emplace_back(wm.world, wm.blockDefs, cursor);
		changesMade = true;

		return 1;

	skip:;

		return 0;
	};

	auto spiral = [](int X, int Y, auto func) {
		int x, y, dx, dy;
		x = y = dx = 0;
		dy		   = -1;
		int t	   = std::max(X, Y);
		int maxI   = t * t;
		for (int i = 0; i < maxI; i++)
		{
			if ((-X / 2 <= x) && (x <= X / 2) && (-Y / 2 <= y) && (y <= Y / 2))
			{
				if (func(x, y))
				{
					return;
				}
			}
			if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
			{
				t  = dx;
				dx = -dy;
				dy = t;
			}
			x += dx;
			y += dy;
		}
	};

	while (!closeThread)
	{
		if (!wm.hasDiffs)
		{
			wm.mutPos.lock();
			playerChunkPos = wm.playerChunkPos;
			wm.mutPos.unlock();

			changesMade = false;

			for (auto it = wm.mesh.begin(); it != wm.mesh.end(); it++)
			{
				if ((it->pos - playerChunkPos).length() > DESTROYDIST)
				{
					wm.toDestroy.push_back(it);
					changesMade = true;
				}

				if (it->update)
				{
					wm.toDestroy.push_back(it);
					changesMade = true;
					buildChunk(it->pos.x - playerChunkPos.x, it->pos.z - playerChunkPos.z);

					goto createSkip;
				}
			}

			spiral(RENDERDIST * 2, RENDERDIST * 2, buildChunk);

		createSkip:;

			if (changesMade)
			{
				wm.hasDiffs = true;
			}
		}
	}
	std::cout << "build thread end" << '\n';
}

ChunkMesh::ChunkMesh(World &world, std::vector<Block> &blockDefs, agl::Vec<int, 3> chunkPos) : pos(chunkPos)
{
	Timer t;
	t.start();
	agl::Vec<int, 3> chunkPosBig = chunkPos * 16;

	ChunkRaw &chunk = world.loadedChunks[chunkPos];

	posList.reserve(32768);

	BlockMap blockMap;

	if (!world.loadedChunks.count(chunkPos + agl::Vec<int, 3>{1, 0, 0}))
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{1, 0, 0});
	}
	if (!world.loadedChunks.count(chunkPos + agl::Vec<int, 3>{-1, 0, 0}))
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{-1, 0, 0});
	}
	if (!world.loadedChunks.count(chunkPos + agl::Vec<int, 3>{0, 0, 1}))
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{0, 0, 1});
	}
	if (!world.loadedChunks.count(chunkPos + agl::Vec<int, 3>{0, 0, -1}))
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{0, 0, -1});
	}

	for (int y = MINHEIGHT; y < MAXHEIGHT; y++)
	{
		for (int x = 0; x < 16; x++)
		{
			for (int z = 0; z < 16; z++)
			{
				agl::Vec<float, 3> pos = agl::Vec<int, 3>{x, y, z};

				auto block = chunk.get(pos);

				if (block.type == world.air)
				{
					continue;
				}

#define MACRO(X, Y, Z)                                                                            \
	{                                                                                             \
		agl::Vec<int, 3> offset = agl::Vec<int, 3>{x + X - 1, y + Y - 1, z + Z - 1};              \
		unsigned int	 id;                                                                      \
		if (inRange(offset.x, 0, 15) && inRange(offset.y, 0, 384) && inRange(offset.z, 0, 15))    \
		{                                                                                         \
			id = chunk.get(offset).type;                                                          \
		}                                                                                         \
		else                                                                                      \
		{                                                                                         \
			id = world.getBlock(chunkPosBig + offset).type;                                       \
		}                                                                                         \
		blockMap.data[X][Y][Z] = (id == world.air || id == world.leaves || !blockDefs[id].solid); \
	}

#define MACRO2(L)       \
	{                   \
		MACRO(0, L, 0); \
		MACRO(1, L, 0); \
		MACRO(2, L, 0); \
		MACRO(0, L, 1); \
		MACRO(1, L, 1); \
		MACRO(2, L, 1); \
		MACRO(0, L, 2); \
		MACRO(1, L, 2); \
		MACRO(2, L, 2); \
	}

				MACRO2(0);
				MACRO2(1);
				MACRO2(2);

#undef MACRO2
#undef MACRO

				bool nonvis = true;

				AmOcCache aoc;

				if (blockMap.data[1][2][1])
				{
					nonvis = false;
					/*block.up	 = true;*/

					aoc.up.x0y0 = AmOcCalc(pos + chunkPosBig, {0, 1, 0}, {-1, 0, 0}, {0, 0, -1}, blockMap);
					aoc.up.x1y0 = AmOcCalc(pos + chunkPosBig, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, blockMap);
					aoc.up.x0y1 = AmOcCalc(pos + chunkPosBig, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, blockMap);
					aoc.up.x1y1 = AmOcCalc(pos + chunkPosBig, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, blockMap);
				}
				else
				{
					/*block.up = false;*/
				}

				if (blockMap.data[1][0][1])
				{
					nonvis = false;
					/*block.down	 = true;*/

					aoc.down.x0y0 = AmOcCalc(pos + chunkPosBig, {0, -1, 0}, {-1, 0, 0}, {0, 0, 1}, blockMap);
					aoc.down.x1y0 = AmOcCalc(pos + chunkPosBig, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, blockMap);
					aoc.down.x0y1 = AmOcCalc(pos + chunkPosBig, {0, -1, 0}, {-1, 0, 0}, {0, 0, -1}, blockMap);
					aoc.down.x1y1 = AmOcCalc(pos + chunkPosBig, {0, -1, 0}, {1, 0, 0}, {0, 0, -1}, blockMap);
				}
				else
				{
					/*block.down = false;*/
				}

				// z

				if (blockMap.data[1][1][2])
				{
					nonvis = false;
					/*block.north	 = true;*/

					aoc.north.x0y0 = AmOcCalc(pos + chunkPosBig, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}, blockMap);
					aoc.north.x1y0 = AmOcCalc(pos + chunkPosBig, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, blockMap);
					aoc.north.x0y1 = AmOcCalc(pos + chunkPosBig, {0, 0, 1}, {-1, 0, 0}, {0, -1, 0}, blockMap);
					aoc.north.x1y1 = AmOcCalc(pos + chunkPosBig, {0, 0, 1}, {1, 0, 0}, {0, -1, 0}, blockMap);
				}
				else
				{
					/*block.north = false;*/
				}

				if (blockMap.data[1][1][0])
				{
					nonvis = false;
					/*block.south	 = true;*/

					aoc.south.x0y0 = AmOcCalc(pos + chunkPosBig, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, blockMap);
					aoc.south.x1y0 = AmOcCalc(pos + chunkPosBig, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, blockMap);
					aoc.south.x0y1 = AmOcCalc(pos + chunkPosBig, {0, 0, -1}, {1, 0, 0}, {0, -1, 0}, blockMap);
					aoc.south.x1y1 = AmOcCalc(pos + chunkPosBig, {0, 0, -1}, {-1, 0, 0}, {0, -1, 0}, blockMap);
				}
				else
				{
					/*block.south = false;*/
				}

				// x

				if (blockMap.data[2][1][1])
				{
					nonvis = false;
					/*block.east	 = true;*/

					aoc.east.x0y0 = AmOcCalc(pos + chunkPosBig, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}, blockMap);
					aoc.east.x1y0 = AmOcCalc(pos + chunkPosBig, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, blockMap);
					aoc.east.x0y1 = AmOcCalc(pos + chunkPosBig, {1, 0, 0}, {0, 0, 1}, {0, -1, 0}, blockMap);
					aoc.east.x1y1 = AmOcCalc(pos + chunkPosBig, {1, 0, 0}, {0, 0, -1}, {0, -1, 0}, blockMap);
				}
				else
				{
					/*block.east = false;*/
				}

				if (blockMap.data[0][1][1])
				{
					nonvis = false;
					/*block.west	 = true;*/

					aoc.west.x0y0 = AmOcCalc(pos + chunkPosBig, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, blockMap);
					aoc.west.x1y0 = AmOcCalc(pos + chunkPosBig, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, blockMap);
					aoc.west.x0y1 = AmOcCalc(pos + chunkPosBig, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}, blockMap);
					aoc.west.x1y1 = AmOcCalc(pos + chunkPosBig, {-1, 0, 0}, {0, 0, 1}, {0, -1, 0}, blockMap);
				}
				else
				{
					/*block.exposed.west = false;*/
				}

				if (!nonvis)
				{
					for (auto &e : blockDefs[block.type].elements)
					{
						agl::Mat4f scale;
						scale.scale(e.size);
						agl::Mat4f offset;
						offset.translate(e.offset + pos + (chunkPos * 16));
						agl::Mat4f mat = offset * scale;

						posList.push_back(mat.data[0][0]);
						posList.push_back(mat.data[0][1]);
						posList.push_back(mat.data[0][2]);
						posList.push_back(mat.data[1][0]);

						posList.push_back(mat.data[1][1]);
						posList.push_back(mat.data[1][2]);
						posList.push_back(mat.data[2][0]);
						posList.push_back(mat.data[2][1]);

						posList.push_back(mat.data[2][2]);
						posList.push_back(mat.data[3][0]);
						posList.push_back(mat.data[3][1]);
						posList.push_back(mat.data[3][2]);

#define COOLERSHIT(dir1, dir2)                    \
	{                                             \
		unsigned int buf = 0;                     \
                                                  \
		buf |= aoc.dir1.x0y0 << 0;                \
		buf |= aoc.dir1.x0y1 << 2;                \
		buf |= aoc.dir1.x1y0 << 4;                \
		buf |= aoc.dir1.x1y1 << 6;                \
		buf |= (long long)e.dir1.tintImage << 8;  \
                                                  \
		buf |= aoc.dir2.x0y0 << 16;               \
		buf |= aoc.dir2.x0y1 << 18;               \
		buf |= aoc.dir2.x1y0 << 20;               \
		buf |= aoc.dir2.x1y1 << 22;               \
		buf |= (long long)e.dir2.tintImage << 24; \
                                                  \
		posList.push_back(*(float *)(&buf));      \
	}

						COOLERSHIT(up, down);	  // 3 x
						COOLERSHIT(south, north); // 3 y
						COOLERSHIT(west, east);	  // 3 z

						posList.push_back(*(float *)&e.id); // 3 w

						// posList.push_back(0); // 4 x
						// posList.push_back(0); // 4 y
						// posList.push_back(0); // 4 z
						// posList.push_back(0); // 4 w
						// posList.push_back(0); // 5 x
						// posList.push_back(0); // 5 y
						// posList.push_back(0); // 5 z
						// posList.push_back(0); // 5 w
					}
				}
			}
		}
	}

	t.stop();

	std::cout << "build took " << t.get() << '\n';
}

unsigned int AmOcCalc(agl::Vec<int, 3> pos, agl::Vec<int, 3> norm, agl::Vec<int, 3> acc1, agl::Vec<int, 3> acc2,
					  BlockMap &map)
{
	bool cornerTouch = !map.get(norm + acc1 + acc2);
	bool lineTouch	 = !map.get(norm + acc1);
	bool oppo		 = !map.get(norm + acc2);

	if (lineTouch && oppo)
	{
		return 3;
	}
	else if ((lineTouch && cornerTouch) || (oppo && cornerTouch))
	{
		return 2;
	}
	else if (lineTouch || oppo || cornerTouch)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
