#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include <SFML/Graphics.hpp>

const unsigned int TEXTURE_SIZE = 512;
const unsigned int WORLEY_POINTS = 20;
const unsigned int CHANNELS = 4;

struct Point
{
    int x;
    int y;

    int distanceTo(Point other)
    {
        int x = other.x - this->x;
        int y = other.y - this->y;
        return sqrt(x * x + y * y);
    }
};

int remap(int value, int min, int max, int newMin, int newMax)
{
    return newMin + (value - min) * (newMax - newMin) / (max - min);
}

sf::Uint8 *generateWorleyNoise(std::mt19937 &rand)
{
    // generate points
    Point worleyPoint;
    Point worleyPoints[WORLEY_POINTS];
    std::uniform_int_distribution<int> d(0, TEXTURE_SIZE);
    for (int i = 0; i < WORLEY_POINTS; i++)
    {
        worleyPoint.x = d(rand);
        worleyPoint.y = d(rand);
        worleyPoints[i] = worleyPoint;
    }

    // generate tiled points
    const unsigned int tiledWorleyPointsCount = WORLEY_POINTS * 9;
    Point tiledWorleyPoints[tiledWorleyPointsCount];
    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            int index = (y * 3 + x) * WORLEY_POINTS;
            for (int i = 0; i < WORLEY_POINTS; i++)
            {
                worleyPoint = worleyPoints[i];
                worleyPoint.x += x * TEXTURE_SIZE;
                worleyPoint.y += y * TEXTURE_SIZE;
                tiledWorleyPoints[i + index] = worleyPoint;
            }
        }
    }

    // get worley noise pixels
    int index, color, x, y;
    Point current, worley;

    const unsigned int tiledTextureSize = TEXTURE_SIZE * 3;
    const unsigned int pixelCount = tiledTextureSize * tiledTextureSize * CHANNELS;
    sf::Uint8 *result = new sf::Uint8[pixelCount];
    for (int i = 0; i < pixelCount; i += CHANNELS)
    {
        // get current coordinates
        index = i / 4;
        current.x = index % tiledTextureSize;
        current.y = floor(index / tiledTextureSize);

        // get closest worley point distance
        std::vector<int> distances = std::vector<int>();
        for (int i = 0; i < tiledWorleyPointsCount; i++)
        {
            worleyPoint = tiledWorleyPoints[i];
            distances.push_back(current.distanceTo(worleyPoint));
        }
        std::sort(distances.begin(), distances.end());
        color = std::min(distances[0], 255);

        // remap for pretty
        color = 255 - std::min(remap(color, 0, 255, 0, 512), 255);

        // assign the channel colors
        result[i + 0] = color; // red
        result[i + 1] = color; // green
        result[i + 2] = color; // blue
        result[i + 3] = 255;   // alpha
    }

    return result;
}

sf::Uint8* generatePerlinNoise(std::mt19937 &rand)
{
    sf::Uint8 *result = new sf::Uint8[0];
    return result;
}

int main(int argc, char *argv[])
{
    // initialize random engine
    std::random_device randomDevice;
    std::mt19937 mt19937(randomDevice());

    // create noises
    std::cout << "Generating Worley noise" << std::endl;
    sf::Texture worleyNoise;
    worleyNoise.create(TEXTURE_SIZE * 3, TEXTURE_SIZE * 3);
    worleyNoise.update(generateWorleyNoise(mt19937));

    sf::Sprite worleySprite;
    worleySprite.setTexture(worleyNoise);
    worleySprite.setTextureRect(sf::IntRect(TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_SIZE));

    // std::cout << "Generating Perlin noise" << std::endl;
    // sf::Texture perlinNoise;
    // perlinNoise.create(TEXTURE_SIZE, TEXTURE_SIZE);
    // perlinNoise.update(generatePerlinNoise(mt19937));

    // sf::Sprite perlinSprite;
    // perlinSprite.setTexture(perlinNoise);

    std::cout << "Initializing window" << std::endl;
    sf::RenderWindow window(sf::VideoMode(TEXTURE_SIZE, TEXTURE_SIZE), "Perlin-Worley Noise");
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }

        window.clear();
        window.draw(worleySprite);
        window.display();
    }

    return 0;
}