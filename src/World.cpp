#include "../inc/World.hpp"

agl::Vec<int, 3> conv(enkiMICoordinate p)
{
	return {p.x, p.y, p.z};
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
