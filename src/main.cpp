#include <SFML/Graphics.hpp>

const unsigned int TEXTURE_SIZE = 256;

int main(int argc, char *argv[])
{
  sf::RenderWindow window(sf::VideoMode(TEXTURE_SIZE, TEXTURE_SIZE), "3D Worley Noise");

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

    window.display();
  }

  return 0;
}