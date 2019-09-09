#include <SFML/Graphics.hpp>
#include <iostream>
#include <math.h>
#include <vector>
#include <thread>
#include <random>
#include <chrono>

#define PI 3.14159265359
using namespace std;

const int numThreads = 1;
const int width =  1280;
const int height = 720;
const int hexRadius = 35;
const int stepPerMove = 5;
const int maxNumLights = 200;
const int spawnRate = 6;
const int trailLength = 18;
const int contactRadius = 5;

int originX = width / 2;
int originY = height / 2;

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
			xOffSet = ceil(cos(PI / 6) * radius);
		}
		else {
			xOffSet = 0;
		}

		for (int j = 0; j < hexPerRow / 2; j++) {
			hexagons.push_back(hexagon(radius, originX + xOffSet, originY + yOffSet - backgroundOffSet, backgroundHexColor));
			hexagons.push_back(hexagon(radius, originX + xOffSet, originY - yOffSet - backgroundOffSet, backgroundHexColor));
			hexagons.push_back(hexagon(radius, originX - xOffSet, originY + yOffSet - backgroundOffSet, backgroundHexColor));
			hexagons.push_back(hexagon(radius, originX - xOffSet, originY - yOffSet - backgroundOffSet, backgroundHexColor));
			xOffSet += ceil(2 * cos(PI / 6) * radius);
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

		void move(int direction) {
			if (!moving) {
				if (parity) {
					angleId = (angleId + direction % 2 + 1) % 3;
					angle = angles[0][angleId];
					parity = false;
				}
				else {
					angleId = (angleId + direction % 2 + 1) % 3;
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

float distance(Light l1, Light l2) {
	return sqrt(pow(l1.shape.getPosition().x -l2.shape.getPosition().x, 2) + pow(l1.shape.getPosition().y - l2.shape.getPosition().y, 2));

}

std::vector<Light> lights;

void moveLights(int start = 0, int end = lights.size()) {
	static thread_local mt19937* generator = nullptr;
	if (!generator) generator = new mt19937(clock());// + this_thread::get_id().hash());
	std::uniform_int_distribution<int> distribution(0, 1);

	for (int i = start; i < end; i++) {
		lights[i].move(distribution(*generator));
	} 
}

int main() {
	srand(time(NULL));

	sf::RenderWindow window(sf::VideoMode(width, height), "Lights");
	window.setFramerateLimit(30);

	vector<sf::CircleShape> hexs = getBackground(hexRadius);

	int tick = 0;
	int timeSum = 0;

	while (window.isOpen()) {
		auto startTime = std::chrono::high_resolution_clock::now();
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear(sf::Color::Black);

		//calculations
		//spawn new lights
		if (tick % spawnRate == 0 && lights.size() < maxNumLights) {
			lights.push_back(Light(1, originX, originY));
		}

		//moves lights
		
		thread threads[numThreads];
		int lightsPerThread = floor(lights.size() / numThreads);
		for (int i = 0; i < numThreads - 1; i++) {
			threads[i] = thread(moveLights, lightsPerThread* i, lightsPerThread * (i + 1));
		}
		threads[numThreads - 1] = thread(moveLights, lightsPerThread * (numThreads - 1), lights.size());
		for (int i = 0; i < numThreads; i++) {
			threads[i].join();
		} 
		
		/*
		for (int i = 0; i < lights.size(); i++) {
			lights[i].move(rand());
		} */
	
		//delete lights that go off screen
		for (int i = 0; i < lights.size(); i++) {

			int x = lights[i].shape.getPosition().x;
			int y = lights[i].shape.getPosition().y;
			if (x < -2 * hexRadius ||
				x > width + 2 * hexRadius ||
				y < -2 * hexRadius ||
				y > height + 2 * hexRadius) {
				lights.erase(lights.begin() + i);
			}
		}

		//delete lights that come into contact
		/*
		for (int i = 0; i < lights.size(); i++) {
			for (int j = i; j < lights.size(); j++) {
				if (i != j) {
					if (distance(lights[i], lights[j]) < contactRadius) {
						lights.erase(lights.begin() + i);
						lights.erase(lights.begin() + j - 1);
					}
				}
			}
		} */
		
		//draw
		//draws background
		
		for (int i = 0; i < hexs.size(); i++) {
			window.draw(hexs[i]);
		} 

		//draws lights and their trails
		for (int i = 0; i < lights.size(); i++) {
			window.draw(lights[i].shape);
			for (int j = 0; j < lights[i].trail.size(); j++) {
				window.draw(lights[i].trail[j]);
			} 
		}

		window.display();
		tick++;
		std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - startTime;

		timeSum += round(elapsed.count() * 1000);
		cout << "\nTick " << tick << ": " << round(elapsed.count() * 1000) << " (ms), Average: " << timeSum / tick << " (ms)";
	}
	return 0;
}
