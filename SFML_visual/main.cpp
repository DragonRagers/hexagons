#include <SFML/Graphics.hpp>
#include <iostream>
#include <math.h>
#include <vector>

#define PI 3.14159265359
using namespace std;

const int width =  1920;
const int height = 1080;
int originX = width / 2;
int originY = height / 2;
const int hexRadius = 25;
const int stepPerMove = 5;
const int spawnRate = 6;
const int trailLength = 18;
const float angles[2][3] = { PI / 6, 5 * PI / 6, 9 * PI / 6,
							7 * PI / 6, 11 * PI / 6, 3 * PI / 6};
const sf::Color colors[6] = {sf::Color::Red, sf::Color::Blue, sf::Color::Yellow,
						sf::Color::Green, sf::Color::Cyan, sf::Color::Magenta};
const sf::Color backgroundHexColor = sf::Color(50, 50, 50);

sf::CircleShape hexagon(int radius, int x, int y, sf::Color outline, sf::Color fill = sf::Color::Transparent, int thickness = 1) {
	sf::CircleShape hex(radius, 6);
	hex.setFillColor(fill);
	hex.setOutlineColor(outline);
	hex.setOutlineThickness(thickness);
	hex.setPosition(x, y);
	hex.setOrigin(radius, radius);
	return hex;
}

vector<sf::CircleShape> getBackground(int radius) {
	int backgroundOffSet = ceil(sin(PI / 6) * radius);

	//currently incorrect but makes it easier to visualize
	int hexPerRow = ceil(width / (2 * cos(PI / 6) * radius)) + 1;
	int hexPerCol = ceil(height / (3 * radius) * 2) + 2;

	int xOffSet = 0;
	int yOffSet = 0;
	vector<sf::CircleShape> hexagons(hexPerCol  * hexPerRow);
	for (int i = 0; i < hexPerCol / 2; i++) {
		if (i % 2 == 0) {
			xOffSet = cos(PI / 6) * radius;
		}
		else {
			xOffSet = 0;
		}

		for (int j = 0; j < hexPerRow / 2; j++) {
			hexagons.push_back(hexagon(radius, originX + xOffSet, originY + yOffSet - backgroundOffSet, backgroundHexColor));
			hexagons.push_back(hexagon(radius, originX + xOffSet, originY - yOffSet - backgroundOffSet, backgroundHexColor));
			hexagons.push_back(hexagon(radius, originX - xOffSet, originY + yOffSet - backgroundOffSet, backgroundHexColor));
			hexagons.push_back(hexagon(radius, originX - xOffSet, originY - yOffSet - backgroundOffSet, backgroundHexColor));
			xOffSet += 2 * cos(PI / 6) * radius;
		}

		yOffSet += ceil((1 + sin(PI / 6)) * radius);
	}
	return hexagons;
}
class Light {
	public:
		sf::CircleShape shape;
		bool moving;
		float angle;
		int step;
		bool parity;
		int angleId;
		sf::Color color;
		vector<sf::CircleShape> trail;

		Light(int radius, int x, int y) {
			//color = colors[rand() % 6];
			color = sf::Color(rand() % 255, rand() % 255, rand() % 255);
			shape = hexagon(radius, x, y, color, color);
			moving = false;
			step = 0;
			parity = true;
			angleId = rand();
		}

		void move() {
			if (!moving) {
				if (parity) {
					angleId = (angleId + rand() % 2 + 1) % 3;
					angle = angles[0][angleId];
					parity = false;
				}
				else {
					angleId = (angleId + rand() % 2 + 1) % 3;
					angle = angles[1][angleId];
					parity = true;
				}
				moving = true;
				step = 0;
			}
			if (moving) {
				step++;
				shape.move(cos(angle) * hexRadius / stepPerMove, sin(angle) * hexRadius / stepPerMove);
			}
			if (step >= stepPerMove) {
				moving = false;
			}
			
			trail.push_back(hexagon(1, shape.getPosition().x, shape.getPosition().y, color, color));
			if (trail.size() > trailLength) {
				trail.erase(trail.begin());
			}

			for (int i = 0; i < trail.size(); i++) {
				sf::Color c = trail[i].getFillColor();
				c.a -= 255 / trailLength;
				trail[i].setFillColor(c);
				trail[i].setOutlineColor(c);	
			}
		}

};

int main() {
	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	sf::RenderWindow window(sf::VideoMode(width, height), "Lights", sf::Style::Default, settings);
	window.setFramerateLimit(30);

	vector<sf::CircleShape> hexs = getBackground(hexRadius);
	std::vector<Light> lights;
	/*
	for (int i = 0; i < 100; i++) {
		lights.push_back(Light(3, originX, originY, sf::Color(255, 0, 0)));
	} */
	int tick = 0;

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear(sf::Color::Black);

		//calculations
		if (tick % spawnRate == 0 && lights.size() < 150) {
			lights.push_back(Light(1, originX, originY));
		}

		for (int i = 0; i < lights.size(); i++) {
			lights[i].move();
			int x = lights[i].shape.getPosition().x;
			int y = lights[i].shape.getPosition().y;
			if (x < -2 * hexRadius ||
				x > width + 2 * hexRadius ||
				y < -2 * hexRadius ||
				y > height + 2 * hexRadius) {
				lights.erase(lights.begin() + i);
			}
			
		}
		
		
		//draw
		for (sf::CircleShape hex : hexs) {
			window.draw(hex);
		}
		for (Light light : lights) {
			window.draw(light.shape);
			for (sf::CircleShape trailBit : light.trail) {
				window.draw(trailBit);
			}
		}

		window.display();
		tick++;
	}
	return 0;
}
