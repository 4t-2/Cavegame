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

				/*if (!block.update)*/
				{
					calcAOCandExposed(block, pos, chunkPosBig, world, blockDefs, chunk);
					/*block.update = false;*/
				}

				if (!block.exposed.nonvis)
				{
					for (auto &e : blockDefs[block.type].elements)
					{
						e.draw(block.exposed, block.aoc, posBuffer, UVBuffer, lightBuffer, pos + chunkPosBig);
					}
				}
			}
		}
	}

	chunk.meshedBefore = true;

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
