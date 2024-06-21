#include "../inc/World.hpp"

agl::Vec<int, 3> conv(enkiMICoordinate p)
{
	return {p.x, p.y, p.z};
}

long long xorshift(long long num)
{
	long long x = num;

	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;

	return x;
}

int noiseFunc(agl::Vec<float, 3> pos)
{
	return sin(pos.x) * 5 + 100;
}

unsigned long long genRand(unsigned long long seed)
{
	unsigned int	   a = seed;
	unsigned int	   b = seed >> 32;
	const unsigned int w = 8 * sizeof(unsigned int);
	const unsigned int s = w / 2;
	a *= 3284157443;

	b ^= a << s | a >> (w - s);
	b *= 1911520717;

	a ^= b << s | b >> (w - s);
	a *= 2048419325;

	return a;
}

agl::Vec<float, 2> randomGradient(agl::Vec<int, 2> pos, unsigned long long seed)
{
	const unsigned w = 8 * sizeof(unsigned);
	const unsigned s = w / 2;
	unsigned	   a = pos.x, b = pos.y;
	a *= 3284157443;

	b ^= a << s | a >> (w - s);
	b *= 1911520717;

	a ^= b << s | b >> (w - s);
	a *= 2048419325;

	a += genRand(seed);

	float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]

	agl::Vec<float, 2> v;
	v.x = sin(random);
	v.y = cos(random);

	return v;
}

// Computes the dot product of the distance and gradient vectors.
float dotGridGradient(agl::Vec<int, 2> chunkPos, agl::Vec<float, 2> pos, unsigned long long seed)
{
	// Get gradient from integer coordinates
	agl::Vec<float, 2> gradient = randomGradient(chunkPos, seed);

	// Compute the distance vector
	agl::Vec<float, 2> dist = pos - chunkPos;

	// Compute the dot-product
	return (dist.dot(gradient));
}

float interpolate(float a0, float a1, float w)
{
	return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
}

// Sample Perlin noise at coordinates x, y
float perlin(agl::Vec<float, 2> pos, unsigned long long seed)
{
	// Determine grid cell corner coordinates
	agl::Vec<int, 2> chunk00 = pos;

	// Compute Interpolation weights
	agl::Vec<float, 2> s = pos - chunk00;

	// Compute and interpolate top two corners
	float n0  = dotGridGradient({chunk00.x, chunk00.y}, pos, seed);
	float n1  = dotGridGradient({chunk00.x + 1, chunk00.y}, pos, seed);
	float ix0 = interpolate(n0, n1, s.x);

	// Compute and interpolate bottom two corners
	n0		  = dotGridGradient({chunk00.x, chunk00.y + 1}, pos, seed);
	n1		  = dotGridGradient({chunk00.x + 1, chunk00.y + 1}, pos, seed);
	float ix1 = interpolate(n0, n1, s.x);

	// Final step: interpolate between the two previously interpolated values, now
	// in y
	float value = interpolate(ix0, ix1, s.y);

	return value;
}

float octavePerlin(agl::Vec<float, 2> pos, int octaves)
{
	float height = 0;

	float div = 0;

	for (int i = 1; i <= octaves; i++)
	{
		height += perlin(pos / i, i) / i;
		// div += 1. / i;
	}

	return height;
}

void World::generateRandom()
{
	for (int x1 = 0; x1 < 32; x1++)
	{
		for (int y1 = 0; y1 < 32; y1++)
		{
			ChunkRaw &cr = loadedChunks[{x1, 0, y1}];
			for (int x = 0; x < 16; x++)
			{
				for (int z = 0; z < 16; z++)
				{
					agl::Vec<float, 2> noisePos = {(x1 * 16) + x, (y1 * 16) + z};

					int height = octavePerlin(noisePos / 50, 64) * 30 + 100;

					for (int y = 0; y < height; y++)
					{
						cr.blocks[x][y][z].type = cobblestone;
					}
					for (int y = height; y < 385; y++)
					{
						cr.blocks[x][y][z].type = air;
					}
				}
			}
		}
	}

	std::cout << loadedChunks.size() << '\n';
}

void World::loadFromFile(std::string dir, std::map<std::string, int> &strToId)
{
	auto fp = fopen(dir.c_str(), "rb");

	if (!fp)
	{
		std::cout << "shit" << '\n';
	}

	auto regionFile = enkiRegionFileLoad(fp);
	std::cout << loadedChunks.size() << '\n';

	enkiNBTDataStream stream;
	// loop through every chunk in region 32*32
	for (int i = 0; i < ENKI_MI_REGION_CHUNKS_NUMBER; i++)
	{
		enkiInitNBTDataStreamForChunk(regionFile, i, &stream);
		if (stream.dataLength)
		{
			enkiChunkBlockData aChunk		  = enkiNBTReadChunk(&stream);
			agl::Vec<int, 3>   chunkOriginPos = conv(enkiGetChunkOrigin(&aChunk)); // y always 0

			ChunkRaw &cr = loadedChunks[chunkOriginPos / 16];

			// go through every section in chunk, variable
			for (int section = 0; section < ENKI_MI_NUM_SECTIONS_PER_CHUNK; ++section)
			{
				if (aChunk.sections[section])
				{
					agl::Vec		 sectionOrigin = conv(enkiGetChunkSectionOrigin(&aChunk, section)) - chunkOriginPos;
					enkiMICoordinate sPos;

					// go through each block in section
					for (sPos.y = 0; sPos.y < ENKI_MI_SIZE_SECTIONS; ++sPos.y)
					{
						for (sPos.z = 0; sPos.z < ENKI_MI_SIZE_SECTIONS; ++sPos.z)
						{
							for (sPos.x = 0; sPos.x < ENKI_MI_SIZE_SECTIONS; ++sPos.x)
							{
								auto voxel = enkiGetChunkSectionVoxelData(&aChunk, section, sPos);

								auto enkiString = aChunk.palette[section].pNamespaceIDStrings[voxel.paletteIndex];

								auto strName = std::string(enkiString.pStrNotNullTerminated, enkiString.size);

								agl::Vec<int, 3> blockPos = (conv(sPos) + sectionOrigin + agl::Vec{0, 64, 0});

								if (strName == "minecraft:water")
								{
									strName = "minecraft:blue_concrete";
								}
								cr.at(blockPos).type = strToId[strName];
							}
						}
					}
				}
			}

			enkiNBTRewind(&stream);
		}
		enkiNBTFreeAllocations(&stream);
	}

	std::cout << "loaded " << loadedChunks.size() << " chunks" << '\n';
}
