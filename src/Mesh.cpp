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

	ChunkRaw *chunkMap[3][3];

	auto it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{-1, 0, -1});
	if (it == world.loadedChunks.end())
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{-1, 0, -1});
		it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{-1, 0, -1});
	}
	chunkMap[0][0] = &it->second;

	it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{0, 0, -1});
	if (it == world.loadedChunks.end())
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{0, 0, -1});
		it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{0, 0, -1});
	}
	chunkMap[1][0] = &it->second;

	it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{1, 0, -1});
	if (it == world.loadedChunks.end())
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{1, 0, -1});
		it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{1, 0, -1});
	}
	chunkMap[2][0] = &it->second;

	it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{-1, 0, 0});
	if (it == world.loadedChunks.end())
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{-1, 0, 0});
		it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{-1, 0, 0});
	}
	chunkMap[0][1] = &it->second;

	chunkMap[1][1] = &chunk;

	it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{1, 0, 0});
	if (it == world.loadedChunks.end())
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{1, 0, 0});
		it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{1, 0, 0});
	}
	chunkMap[2][1] = &it->second;

	it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{-1, 0, 1});
	if (it == world.loadedChunks.end())
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{-1, 0, 1});
		it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{-1, 0, 1});
	}
	chunkMap[0][2] = &it->second;

	it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{0, 0, 1});
	if (it == world.loadedChunks.end())
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{0, 0, 1});
		it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{0, 0, 1});
	}
	chunkMap[1][2] = &it->second;

	it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{1, 0, 1});
	if (it == world.loadedChunks.end())
	{
		world.createChunk(chunkPos + agl::Vec<int, 3>{1, 0, 1});
		it = world.loadedChunks.find(chunkPos + agl::Vec<int, 3>{1, 0, 1});
	}
	chunkMap[2][2] = &it->second;

	for (int y = 0; y < 384; y++)
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

				if (block.update)
				{
					calcAOCandExposed(block, pos, chunkPosBig, world, blockDefs, chunkMap);
					block.update = false;
				}

				if (true)
				{
					for (auto &e : blockDefs[block.type].elements)
					{
						e.draw(block.exposed, block.aoc, posBuffer, UVBuffer, lightBuffer, pos + chunkPosBig);
					}
				}
			}
		}
	}

	t.stop();

	/*std::cout << "build took " << t.get() << '\n';*/
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

inline void calcAOCandExposed(BlockData &bd, agl::Vec<int, 3> pos, agl::Vec<int, 3> chunkPosBig, World &world,
							  std::vector<Block> &blockDefs, ChunkRaw *chunkMap[3][3])
{
	BlockMap blockMap;

#define MACRO(X, Y, Z)                                                                            \
	{                                                                                             \
		agl::Vec<int, 3> offset = agl::Vec<int, 3>{pos.x + X - 1, pos.y + Y - 1, pos.z + Z - 1};  \
		agl::Vec<int, 3> chunkPos;                                                                \
		if (offset.x < 0)                                                                         \
		{                                                                                         \
			chunkPos.x = -1;                                                                      \
		}                                                                                         \
		else if (offset.x > 15)                                                                   \
		{                                                                                         \
			chunkPos.x = 1;                                                                       \
		}                                                                                         \
		else                                                                                      \
		{                                                                                         \
			chunkPos.x = 0;                                                                       \
		}                                                                                         \
		if (offset.z < 0)                                                                         \
		{                                                                                         \
			chunkPos.z = -1;                                                                      \
		}                                                                                         \
		else if (offset.z > 15)                                                                   \
		{                                                                                         \
			chunkPos.z = 1;                                                                       \
		}                                                                                         \
		else                                                                                      \
		{                                                                                         \
			chunkPos.z = 0;                                                                       \
		}                                                                                         \
		unsigned int id;                                                                          \
		if (inRange(offset.y, 0, 383))                                                            \
		{                                                                                         \
			id = chunkMap[chunkPos.x + 1][chunkPos.z + 1]->get(offset - (chunkPos * 16)).type;    \
		}                                                                                         \
		else                                                                                      \
		{                                                                                         \
			id = world.air;                                                                       \
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

	bd.exposed.nonvis = true;

	if (blockMap.data[1][2][1])
	{
		bd.exposed.nonvis = false;
		bd.exposed.up	  = true;

		bd.aoc.up.x0y0 = AmOcCalc(pos, {0, 1, 0}, {-1, 0, 0}, {0, 0, -1}, blockMap);
		bd.aoc.up.x1y0 = AmOcCalc(pos, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}, blockMap);
		bd.aoc.up.x0y1 = AmOcCalc(pos, {0, 1, 0}, {-1, 0, 0}, {0, 0, 1}, blockMap);
		bd.aoc.up.x1y1 = AmOcCalc(pos, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, blockMap);
	}
	else
	{
		bd.exposed.up = false;
	}

	if (blockMap.data[1][0][1])
	{
		bd.exposed.nonvis = false;
		bd.exposed.down	  = true;

		bd.aoc.down.x0y0 = AmOcCalc(pos, {0, -1, 0}, {-1, 0, 0}, {0, 0, 1}, blockMap);
		bd.aoc.down.x1y0 = AmOcCalc(pos, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, blockMap);
		bd.aoc.down.x0y1 = AmOcCalc(pos, {0, -1, 0}, {-1, 0, 0}, {0, 0, -1}, blockMap);
		bd.aoc.down.x1y1 = AmOcCalc(pos, {0, -1, 0}, {1, 0, 0}, {0, 0, -1}, blockMap);
	}
	else
	{
		bd.exposed.down = false;
	}

	// z

	if (blockMap.data[1][1][2])
	{
		bd.exposed.nonvis = false;
		bd.exposed.north  = true;

		bd.aoc.north.x0y0 = AmOcCalc(pos, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}, blockMap);
		bd.aoc.north.x1y0 = AmOcCalc(pos, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}, blockMap);
		bd.aoc.north.x0y1 = AmOcCalc(pos, {0, 0, 1}, {-1, 0, 0}, {0, -1, 0}, blockMap);
		bd.aoc.north.x1y1 = AmOcCalc(pos, {0, 0, 1}, {1, 0, 0}, {0, -1, 0}, blockMap);
	}
	else
	{
		bd.exposed.north = false;
	}

	if (blockMap.data[1][1][0])
	{
		bd.exposed.nonvis = false;
		bd.exposed.south  = true;

		bd.aoc.south.x0y0 = AmOcCalc(pos, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}, blockMap);
		bd.aoc.south.x1y0 = AmOcCalc(pos, {0, 0, -1}, {-1, 0, 0}, {0, 1, 0}, blockMap);
		bd.aoc.south.x0y1 = AmOcCalc(pos, {0, 0, -1}, {1, 0, 0}, {0, -1, 0}, blockMap);
		bd.aoc.south.x1y1 = AmOcCalc(pos, {0, 0, -1}, {-1, 0, 0}, {0, -1, 0}, blockMap);
	}
	else
	{
		bd.exposed.south = false;
	}

	// x

	if (blockMap.data[2][1][1])
	{
		bd.exposed.nonvis = false;
		bd.exposed.east	  = true;

		bd.aoc.east.x0y0 = AmOcCalc(pos, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}, blockMap);
		bd.aoc.east.x1y0 = AmOcCalc(pos, {1, 0, 0}, {0, 0, -1}, {0, 1, 0}, blockMap);
		bd.aoc.east.x0y1 = AmOcCalc(pos, {1, 0, 0}, {0, 0, 1}, {0, -1, 0}, blockMap);
		bd.aoc.east.x1y1 = AmOcCalc(pos, {1, 0, 0}, {0, 0, -1}, {0, -1, 0}, blockMap);
	}
	else
	{
		bd.exposed.east = false;
	}

	if (blockMap.data[0][1][1])
	{
		bd.exposed.nonvis = false;
		bd.exposed.west	  = true;

		bd.aoc.west.x0y0 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}, blockMap);
		bd.aoc.west.x1y0 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, blockMap);
		bd.aoc.west.x0y1 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}, blockMap);
		bd.aoc.west.x1y1 = AmOcCalc(pos, {-1, 0, 0}, {0, 0, 1}, {0, -1, 0}, blockMap);
	}
	else
	{
		bd.exposed.west = false;
	}
}
