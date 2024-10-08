#include "../inc/World.hpp"
#include <fstream>
#include <vector>

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

float octavePerlin(agl::Vec<float, 2> pos, std::vector<float> amplitudes)
{
	float height = 0;

	float div = 0;

	for (int i = 1; i <= amplitudes.size(); i++)
	{
		height += (perlin(pos / i, i)); //* amplitudes[i - 1]) / i;
		div += 1. / i;
	}

	height /= div;

	if (height > .75)
	{
		std::cout << "big" << '\n';
	}
	if (height < -0.75)
	{
		std::cout << "small" << '\n';
	}

	return height;
}

float fract(float x)
{
	return x - std::floor(x);
}

float mix(float x, float y, float a)
{
	return x * (1 - a) + y * a;
}

float random(agl::Vec<float, 2> st)
{
	// std::cout << st << " MAKES " << fract(sin(st.dot(agl::Vec<float,
	// 2>(12.9898, 78.233))) * 43758.5453123) << '\n';
	return fract(sin(st.dot(agl::Vec<float, 2>(12.9898, 78.233))) * 43758.5453123);
}

float noise(agl::Vec<float, 2> st)
{
	agl::Vec<float, 2> i = {floor(st.x), floor(st.y)};
	agl::Vec<float, 2> f = {fract(st.x), fract(st.y)};

	// Four corners in 2D of a tile
	float a = random(i);
	float b = random(i + agl::Vec<float, 2>(1.0, 0.0));
	float c = random(i + agl::Vec<float, 2>(0.0, 1.0));
	float d = random(i + agl::Vec<float, 2>(1.0, 1.0));

	auto pre = (agl::Vec<float, 2>(3.0, 3.0) - (f * 2.0));

	agl::Vec<float, 2> u = {f.x * f.x * pre.x, f.y * f.y * pre.y};

	return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(agl::Vec<float, 2> st, std::vector<float> amplitudes)
{
	// Initial values
	float value		= 0.0;
	float amplitude = .5;
	float frequency = 0.;
	//
	// Loop of octaves
	for (int i = 0; i < amplitudes.size(); i++)
	{
		value += amplitude * amplitudes[i] * noise(st);
		st *= 2.;
		amplitude *= .5;
	}
	return value;
}

class LineGraph
{
	public:
		std::map<float, float> points;

		float getValue(float x)
		{
			std::pair<float, float> last = *points.begin();

			if (last.first >= x)
			{
				return last.second;
			}

			for (auto pair : points)
			{
				if (pair.first >= x)
				{
					float xdiff = pair.first - last.first;

					float ydiff = pair.second - last.second;

					float actualdiff = x - last.first;

					return last.first + ((actualdiff / xdiff) * ydiff);
				}

				last = pair;
			}

			return std::next(points.end(), -1)->second;
		}
};

float getSplineVal(Json::Value splineRoot, float continentalness, float erosion, float ridges_folded)
{
	LineGraph lg;

	float location;
	if (splineRoot["coordinate"].asString() == "minecraft:overworld/continents")
	{
		location = continentalness;
	}
	if (splineRoot["coordinate"].asString() == "minecraft:overworld/erosion")
	{
		location = erosion;
	}
	if (splineRoot["coordinate"].asString() == "minecraft:overworld/ridges_folded")
	{
		location = ridges_folded;
	}

	for (auto &e : splineRoot["points"])
	{
		if (e["value"].isNumeric())
		{
			lg.points.emplace(std::pair(e["location"].asFloat(), e["value"].asFloat()));
		}
		else
		{
			float val = getSplineVal(e["value"], continentalness, erosion, ridges_folded);
			lg.points.emplace(std::pair(e["location"].asFloat(), val));
		}

		if (e["location"].asFloat() > location)
		{
			break;
		}
	}

	if (splineRoot["coordinate"].asString() == "minecraft:overworld/continents")
	{
		return lg.getValue(continentalness);
	}
	if (splineRoot["coordinate"].asString() == "minecraft:overworld/erosion")
	{
		return lg.getValue(erosion);
	}
	if (splineRoot["coordinate"].asString() == "minecraft:overworld/ridges_folded")
	{
		return lg.getValue(ridges_folded);
	}

	std::cout << "FUCK " << splineRoot["coordinate"].asString() << '\n';
	return -99999999.;
}

void World::createChunk(agl::Vec<int, 3> chunkPos)
{
	std::vector<float> continentalnessAmp = {1, 1, 2, 2, 2, 1, 1, 1, 1};
	std::vector<float> erosionAmp		  = {1, 1, 0, 1, 1, 1, 1, 1, 1};
	std::vector<float> ridgeAmp			  = {1, 2, 1, 0, 0, 0, 1};

	ChunkRaw &cr = loadedChunks[chunkPos];
	for (int x = 0; x < 16; x++)
	{
		for (int z = 0; z < 16; z++)
		{
			agl::Vec<float, 2> noisePos = {(chunkPos.x * 16) + x, (chunkPos.z * 16) + z};

			float con = ((fbm(noisePos / 640, continentalnessAmp) - .5) * 2);
			float ero = ((fbm((noisePos / 400) + agl::Vec<float, 2>(1000, 1000), erosionAmp) - .5) * 2);
			float rid = ((fbm((noisePos / 256) + agl::Vec<float, 2>(-1000, -0100), ridgeAmp) - .5) * 2);

			// std::cout << con << "\n";

			float folded = (std::abs(std::abs(rid) - 0.51) - 0.49) * -3;
			folded		 = std::min(std::max(-0.1f, folded), 0.1f);
			folded += .1;
			folded *= 5;

			// LineGr

			// float offset = rid;

			float offset = con / 3;
			offset *= (std::max<float>(ero - .25, 0.f) * 20) + 1;
			// if(ero > 0)
			// {
			// 	offset = 1;
			// }
			offset *= folded;

			// offset += ero * std::max(0.f, con);

			// float offset = getSplineVal(splineRoot, con, ero, folded);

			offset += -0.6;
			// offset += .5;

			int height = (((offset + 1.5) / 3.) * 384.);

			height = std::max(0, std::min(384, height));

			for (int y = MINHEIGHT; y < MAXHEIGHT; y++)
			{
				if (y <= 62 + 64)
				{
					cr.set({x, y, z}, BlockData{blue_wool});
				}
				else if (y <= height)
				{
					if ((con / 3) < .105)
					{
						cr.set({x, y, z}, BlockData{sand});
						continue;
					}

					int diff = height - y;
					if (diff == 0)
					{
						if (y > 64 + 62 + 20)
						{
							cr.set({x, y, z}, BlockData{snow});
						}
						else
						{
							cr.set({x, y, z}, BlockData{grass});
						}
					}
					else if (diff > 1)
					{
						cr.set({x, y, z}, BlockData{stone});
					}
					else
					{
						cr.set({x, y, z}, BlockData{dirt});
					}
				}
				else
				{
					cr.set({x, y, z}, BlockData{air});
				}
			}
		}
	}
}

void World::generateRandom(std::map<std::string, int> &strToId)
{
	// LinGraph lg;
	// lg.points.emplace(std::pair<float, float>(0, 0));
	// lg.points.emplace(std::pair<float, float>(1, 1));
	// lg.points.emplace(std::pair<float, float>(2, 2));
	// lg.points.emplace(std::pair<float, float>(3, 3));
	// lg.points.emplace(std::pair<float, float>(4, 6));
	//
	// std::cout << "there are " << lg.points.size() << '\n';
	//
	// std::cout << lg.getValue(2) << '\n';
	// std::cout << lg.getValue(0) << '\n';
	// std::cout << lg.getValue(-10) << '\n';
	// std::cout << lg.getValue(7) << '\n';
	// std::cout << lg.getValue(3.9) << '\n';
	//
	// exit(1);

	std::vector<float> continentalnessAmp = {1, 1, 2, 2, 2, 1, 1, 1, 1};
	std::vector<float> erosionAmp		  = {1, 1, 0, 1, 1, 1, 1, 1, 1};
	std::vector<float> ridgeAmp			  = {1, 2, 1, 0, 0, 0, 1};

	Json::Value offsetJson;
	{

		Json::Reader reader;
		std::fstream fs("resources/java/data/minecraft/worldgen/density_function/"
						"overworld/offset.json",
						std::ios::in);
		reader.parse(fs, offsetJson, false);
		fs.close();
	}

	Json::Value splineRoot = offsetJson["argument"]["argument"]["argument2"]["argument1"]["argument2"]["spline"];

	for (int x1 = 0; x1 < 16; x1++)
	{
		std::cout << x1 << '\n';
		for (int y1 = 0; y1 < 16; y1++)
		{
			createChunk({x1, 0, y1});
		}
	}

	std::cout << loadedChunks.size() << '\n';
}
