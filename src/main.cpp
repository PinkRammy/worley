#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

const unsigned int SPRITESHEET_SIZE = 512;
const unsigned int TEXTURE_SLICES = 64;
const unsigned int TEXTURE_SLICE_ROW = sqrt(TEXTURE_SLICES);
const unsigned int TEXTURE_SIZE = SPRITESHEET_SIZE / TEXTURE_SLICE_ROW;
const unsigned int TILED_TEXTURE_SIZE = TEXTURE_SIZE * 3;
const unsigned int WORLEY_POINTS = 9;
const unsigned int TEXTURE_PIXELS = TEXTURE_SIZE * TEXTURE_SIZE;

std::unordered_map<int, std::vector<sf::Uint8>> worleyTiles;
std::mutex _MUTEX;

struct Point
{
    int x;
    int y;

    double distanceTo(const Point &other) const
    {
        return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
    }
};

int remap(int value, int min, int max, int newMin, int newMax)
{
    return newMin + (value - min) * (newMax - newMin) / (max - min);
}

std::vector<sf::Uint8> generateTiledWorleyNoise(std::random_device &randomDevice)
{
    // generate random seed
    std::mt19937 rand(randomDevice());

    // generate points
    std::vector<Point> worleyPoints;
    std::uniform_int_distribution<int> d(0, TEXTURE_SIZE - 1);
    for (int i = 0; i < WORLEY_POINTS; i++)
    {
        Point worleyPoint{d(rand), d(rand)};
        worleyPoints.push_back(worleyPoint);
    }

    // tile the points
    const unsigned int TILED_WORLEY_POINTS = WORLEY_POINTS * 9;
    std::vector<Point> tiledWorleyPoints;
    unsigned int worleyTile;
    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            worleyTile = y * 3 + x;
            for (int i = 0; i < WORLEY_POINTS; i++)
            {
                Point worleyPoint = worleyPoints.at(i);
                worleyPoint.x += x * TEXTURE_SIZE;
                worleyPoint.y += y * TEXTURE_SIZE;
                tiledWorleyPoints.push_back(worleyPoint);
            }
        }
    }

    // generate the tiled texture
    const unsigned int TILED_TEXTURE_PIXELS = TILED_TEXTURE_SIZE * TILED_TEXTURE_SIZE * 4;
    std::vector<sf::Uint8> tiledPixels(TILED_TEXTURE_PIXELS);
    unsigned int color, index;
    for (int y = 0; y < TILED_TEXTURE_SIZE; y++)
    {
        for (int x = 0; x < TILED_TEXTURE_SIZE; x++)
        {
            // get current pixel coordinates
            Point current{x, y};

            // get closest worley point distance
            std::vector<int> distances = std::vector<int>();
            for (int i = 0; i < TILED_WORLEY_POINTS; i++)
            {
                distances.push_back(current.distanceTo(tiledWorleyPoints[i]));
            }
            std::sort(distances.begin(), distances.end());
            color = std::min(distances[0], 255);
            color = 255 - std::min(remap(color, 0, 255, 64, 512), 255); // remap for pretty

            // set the pixel value
            index = (y * TILED_TEXTURE_SIZE + x) * 4;
            tiledPixels[index + 0] = color;
            tiledPixels[index + 1] = color;
            tiledPixels[index + 2] = color;
            tiledPixels[index + 3] = 255;
        }
    }

    return tiledPixels;
}

void generateWorleyNoiseSlices(int index, int count, std::random_device &randomDevice)
{
    for (int i = index; i < index + count; i++)
    {
        std::lock_guard<std::mutex> lock(_MUTEX);
        worleyTiles[i] = generateTiledWorleyNoise(randomDevice);
    }
}

std::vector<sf::Uint8> getWorleyNoiseSlice(int index)
{
    std::lock_guard<std::mutex> lock(_MUTEX);
    auto slicesIt = worleyTiles.find(index);
    if (slicesIt != worleyTiles.end())
    {
        return slicesIt->second;
    }

    return {};
}

int main(int argc, char *argv[])
{
    // initialize random engine
    std::random_device randomDevice;

    // create noises
    const unsigned int numThreads = std::min(std::thread::hardware_concurrency(), TEXTURE_SLICES);
    unsigned int slicesPerThread = TEXTURE_SLICES / numThreads;
    std::cout << "Using " << numThreads << " threads to generate " << TEXTURE_SLICES << " noises" << std::endl;
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++)
    {
        int threadSliceIndex = i * slicesPerThread;
        int threadSlices = (i == numThreads - 1) ? TEXTURE_SLICES - threadSliceIndex : slicesPerThread;
        threads.emplace_back(generateWorleyNoiseSlices, threadSliceIndex, threadSlices, std::ref(randomDevice));
    }
    for (std::thread &thread : threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    std::cout << "Generating spritesheet" << std::endl;
    std::vector<std::string> spritesheet;
    sf::IntRect spritesheetRect(TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_SIZE);
    const unsigned int TILED_TEXTURE_PIXELS = TILED_TEXTURE_SIZE * TILED_TEXTURE_SIZE * 4;
    for (int i = 0; i < TEXTURE_SLICES; i++)
    {
        std::vector<sf::Uint8> slicePixels = getWorleyNoiseSlice(i);
        if (slicePixels.empty())
        {
            std::cout << "\tError: Slice #" << i << " is empty.";
            return -1;
        }

        std::string filename = "worleySlice_" + std::to_string(i) + ".bmp";
        stbi_write_bmp(filename.c_str(), TILED_TEXTURE_SIZE, TILED_TEXTURE_SIZE, 4, slicePixels.data());
        spritesheet.push_back(filename);
    }

    std::cout << "Initializing window" << std::endl;
    sf::RenderWindow window(sf::VideoMode(SPRITESHEET_SIZE, SPRITESHEET_SIZE), "Perlin-Worley Noise");
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                sf::Texture tex;
                tex.create(SPRITESHEET_SIZE, SPRITESHEET_SIZE);
                tex.update(window);
                tex.copyToImage().saveToFile("worleySpritesheet.png");

                window.close();
            }
        }

        window.clear();

        float sliceX, sliceY;
        for (int i = 0; i < TEXTURE_SLICES; i++)
        {
            sf::Texture worleySliceTexture;
            worleySliceTexture.loadFromFile(spritesheet.at(i));
            sf::Sprite worleySlice(worleySliceTexture, spritesheetRect);
            sliceX = i % TEXTURE_SLICE_ROW;
            sliceY = std::floor(i / TEXTURE_SLICE_ROW);
            worleySlice.move(sliceX * TEXTURE_SIZE, sliceY * TEXTURE_SIZE);
            window.draw(worleySlice);
        }

        window.display();
    }

    return 0;
}