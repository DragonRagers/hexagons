#include <SFML/Graphics.hpp>
#include <iostream>
#include <math.h>
#include <vector>
#include <thread>
#include <random>
#include <chrono>

#define PI 3.14159265359
using namespace std;

const int numThreads = 4;
const int width =  1280;
const int height = 720;
const int hexRadius = 35; //35
const int lightRadius = 2;
const int stepPerMove = 8; //8
const int maxNumLights = 200;
const int spawnRate = 8;
const int trailLength = 18;
const int contactRadius = 5;
const int maxNumParticle = 3;
const int maxParticleAge = 10;
const bool showBackground = true;
const int maxQuadTreeLevel = 4;

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
	int hexPerCol = ceil(height / (3 * radius) * 2) + 4;

	int xOffSet = 0;
	int yOffSet = 0;
	vector<sf::CircleShape> hexagons(hexPerCol  * hexPerRow);
	for (int i = 0; i < ceil(hexPerCol / 2); i++) {
		if (i % 2 == 0) {
			xOffSet = ceil(cos(PI / 6) * radius);
		}
		else {
			xOffSet = 0;
		}

		for (int j = 0; j < ceil(hexPerRow / 2); j++) {
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
int randInt(int min, int max) {
	static thread_local mt19937* generator = nullptr;
	if (!generator) generator = new mt19937(clock());// + this_thread::get_id().hash());
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(*generator);
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
		vector<sf::CircleShape> particles;
		vector<int> particleAge;

		Light(int radius, int x, int y) {
			//color = colors[rand() % 6];
			color = sf::Color(rand() % 255, rand() % 255, rand() % 255);
			shape = hexagon(radius, x, y, color, color);
			moving = false;
			step = 0;
			parity = true;
			angleId = rand();
		}

		void setColor(sf::Color c) {
			color = c;
			shape.setFillColor(c);
			shape.setOutlineColor(c);
		}

		void move() {
			if (!moving) {
				int temp = randInt(0, 1);
				if (parity) {
					angleId = (angleId + temp + 1) % 3;
					angle = angles[0][angleId];
					parity = false;
				}
				else {
					angleId = (angleId + temp + 1) % 3;
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
			
			trail.push_back(hexagon(1, shape.getPosition().x, shape.getPosition().y, sf::Color::Transparent, color));
			if (trail.size() > trailLength) {
				trail.erase(trail.begin());
			}

			for (int i = 0; i < trail.size(); i++) {
				sf::Color c = trail[i].getFillColor();
				c.a -= 255 / trailLength;
				trail[i].setFillColor(c);
				trail[i].setOutlineColor(c);	
			}

			if (particles.empty() || particles.size() < maxNumParticle && particleAge[particleAge.size() - 1] > maxParticleAge / maxNumParticle) {
				particles.push_back(hexagon(1, shape.getPosition().x + randInt(-hexRadius / 3, hexRadius / 3), shape.getPosition().y + randInt(-hexRadius / 3, hexRadius / 3), sf::Color::Transparent, color));
				particleAge.push_back(0);
			} 

			for (int i = 0; i < particleAge.size(); i++) {
				particleAge[i]++;
				if (particleAge[i] > maxParticleAge) {
					particles[i].setPosition(shape.getPosition().x + randInt(-hexRadius / 3, hexRadius / 3), shape.getPosition().y + randInt(-hexRadius / 3, hexRadius / 3));
					particleAge[i] = 0;
				}
			}
		}

};

class QuadTree {
	public:
		vector<Light> lights;
		vector<QuadTree> nodes;
		bool leaf;
		int startX, startY, endX, endY;
		int level;
		int midX;
		int midY;

		QuadTree(int lvl, int x1, int y1, int x2, int y2) {
			level = lvl;
			startX = x1;
			startY = y1;
			endX = x2;
			endY = y2;
			leaf = true;
			midX = ceil((startX + endX) / 2);
			midY = ceil((startY + endY) / 2);
		}

		void insert(Light l) {
			if (leaf) {
				lights.push_back(l);
			}
			else {
				if (l.shape.getPosition().x > ceil((startX + endX) / 2)) {
					if (l.shape.getPosition().y > ceil((startY + endY) / 2)) {
						nodes[0].insert(l);
					}
					else {
						nodes[1].insert(l);
					}
				}
				else {
					if (l.shape.getPosition().y <= ceil((startY + endY) / 2)) {
						nodes[2].insert(l);
					}
					else {
						nodes[3].insert(l);
					}

				}
			}

			if (level < maxQuadTreeLevel && lights.empty() && lights.size() > 3) {
				nodes.push_back(QuadTree(level + 1, midX, startY, endX, midY));
				nodes.push_back(QuadTree(level + 1, startX, startY, midX, midY));
				nodes.push_back(QuadTree(level + 1, startX, midY, midX, endY));
				nodes.push_back(QuadTree(level + 1, midX, midY, endX, endY));
			}	
		}

		bool hasLights() {
			return !lights.empty();
		}

};

float distance(Light l1, Light l2) {
	return sqrt(pow(l1.shape.getPosition().x -l2.shape.getPosition().x, 2) + pow(l1.shape.getPosition().y - l2.shape.getPosition().y, 2));

}

std::vector<Light> lights;
void moveLights(int start = 0, int end = lights.size()) {

	for (int i = start; i < end; i++) {
		lights[i].move();
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
			lights.push_back(Light(lightRadius, originX, originY));
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
			lights[i].move();
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

		//draw
		//draws background
		
		if (showBackground) {
			for (int i = 0; i < hexs.size(); i++) {
				window.draw(hexs[i]);
			}
		}

		//draws lights and their trails
		for (int i = 0; i < lights.size(); i++) {
			window.draw(lights[i].shape);
			for (int j = 0; j < lights[i].trail.size(); j++) {
				window.draw(lights[i].trail[j]);
			} 
			for (int j = 0; j < lights[i].particles.size(); j++) {
				window.draw(lights[i].particles[j]);
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
