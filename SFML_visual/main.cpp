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
const int width = 1280;
const int height = 720;
const int hexRadius = 35; //35
const int lightRadius = 2;
const int stepPerMove = 8; //8
const int maxMoves = 30;
const int maxNumLights = 500;
const int spawnRate = 1;
const int trailLength = 10;
const int contactRadius = 3;
const int maxNumParticle = 2;
const int maxParticleAge = 10;
const bool showBackground = false;
const bool collide = false;
const int maxLightPerTree = 3;
const int maxQuadTreeLevel = 5;

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
	int originX = width / 2;
	int originY = height / 2;

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

int r = 0;
bool rBol = true;
int g = 0;
bool gBol = true;
int b = 0;
bool bBol = true;
int incr = 5;
int numLight = 0;
class Light {
	public:
		sf::CircleShape shape;
		bool moving;
		float angle;
		int moves;
		int step;
		bool parity;
		int angleId;
		int id;
		sf::Color color;
		vector<sf::CircleShape> trail;
		vector<sf::CircleShape> particles;
		vector<int> particleAge;

		Light(int radius, int x, int y, int i) {
			//color = colors[rand() % 6];
			//color = sf::Color(rand() % 255, rand() % 255, rand() % 255);
			//color = sf::Color(255, 255, 255);
			
			color = sf::Color(r, g, b);
			b += bBol ? incr : -incr;
			if (bBol ? b >= 255: b <= 0) {
				g += gBol ? incr : -incr;
				bBol = !bBol;
			}
			else if (gBol ? g >= 255 : g <= 0) {
				if (rBol ? r >= 255 : r <= 0) {
					rBol != rBol;
				}
				r += rBol ? incr : -incr;				
			} 
			
			id = i;
			shape = hexagon(radius, x, y, color, color);
			moving = false;
			moves = 0;
			step = 0;
			parity = true;
			angleId = rand();
			numLight++;
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
				moves++;
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
		int startX, startY, endX, endY;
		int level;
		int midX;
		int midY;
		
		QuadTree() { //should only be called when other parameters are set before use
			level = 0;
		}

		QuadTree(int lvl, int x1, int y1, int x2, int y2) {
			level = lvl;
			startX = x1;
			startY = y1;
			endX = x2;
			endY = y2;
			midX = ceil((startX + endX) / 2);
			midY = ceil((startY + endY) / 2);
		}

		void setMid() {
			midX = (startX + endX) / 2;
			midY = (startY + endY) / 2;
		}

		void clear() {
			lights.clear();
			nodes.clear();
		}

		void split() {
			nodes.push_back(QuadTree(level + 1, midX, startY, endX, midY));
			nodes.push_back(QuadTree(level + 1, startX, startY, midX, midY));
			nodes.push_back(QuadTree(level + 1, startX, midY, midX, endY));
			nodes.push_back(QuadTree(level + 1, midX, midY, endX, endY));
		}

		int getIndex(Light& l) {
			if (l.shape.getPosition().x > midX) {
				if (l.shape.getPosition().y > midY) {
					return 3;
				}
				else {
					return 0;
				}
			}
			else {
				if (l.shape.getPosition().y > midY) {
					return 2;
				}
				else {
					return 1;
				}				
			}
		}
	
		void insert(Light& l) {
			if (!nodes.empty()) {
				int index = getIndex(l);
				nodes[index].insert(l);
				return;
				
			}
			lights.push_back(l);
			if (lights.size() > maxLightPerTree && level < maxQuadTreeLevel) {
				if (nodes.empty()) {
					split();
				}

				for (int i = 0; i < lights.size(); i++) {
					int index = getIndex(lights[i]);
					nodes[index].insert(lights[i]);		
				}
				lights.clear();
			}
		}

		vector<Light> retrieve(Light& l) {
			int index = getIndex(l);
			if (!nodes.empty()) {
				return nodes[index].retrieve(l);
			}
			else {
				return lights;
			}
		}
};

float distance(Light l1, Light l2) {
	return sqrt(pow(l1.shape.getPosition().x -l2.shape.getPosition().x, 2) + pow(l1.shape.getPosition().y - l2.shape.getPosition().y, 2));

}

class Source {
	public:
		int originX, originY;
		std::vector<Light> lights;
		QuadTree tree;

		Source(int x, int y) {
			originX = x;
			originY = y;
			tree.startX = 0;
			tree.startY = 0;
			tree.endX = width;
			tree.endY = height;
			tree.setMid();
		}

		void update(int tick) {
			//calculations
			//spawn new lights
			if (tick % spawnRate == 0 && numLight < maxNumLights) {
				lights.push_back(Light(lightRadius, originX, originY, tick));
			}

			//moves lights
			for (int i = 0; i < lights.size(); i++) {
				lights[i].move();
			}

			//delete lights that go off screen
			for (int i = 0; i < lights.size(); i++) {

				int x = lights[i].shape.getPosition().x;
				int y = lights[i].shape.getPosition().y;
				if (x < -2 * hexRadius ||
					x > width + 2 * hexRadius ||
					y < -2 * hexRadius ||
					y > height + 2 * hexRadius) {
					lights.erase(lights.begin() + i);
					numLight--;
				}
			}

			//delete lights that have moved more that maxMoves
			for (int i = 0; i < lights.size(); i++) {
				if (lights[i].moves > maxMoves) {
					lights.erase(lights.begin() + i);
					numLight--;
				}
			}
			
			//deletes lights that collide
			if (collide) {
				tree.clear();
				for (int i = 0; i < lights.size(); i++) {
					tree.insert(lights[i]);
				}

				for (int i = 0; i < lights.size(); i++) {

					vector<Light> nearby = tree.retrieve(lights[i]);

					for (int j = 0; j < nearby.size(); j++) {
						if (distance(lights[i], nearby[j]) < contactRadius && lights[i].id != nearby[j].id) {
							lights.erase(lights.begin() + i);
							numLight--;
							break;
						}
					}
				}
			} 
		}
}; 

void sourceUpdate(Source& s, int tick) {
	s.update(tick);
}

int main() {
	srand(time(NULL));

	sf::RenderWindow window(sf::VideoMode(width, height), "Lights");
	window.setFramerateLimit(30);

	vector<sf::CircleShape> hexs = getBackground(hexRadius);
	vector<Source> sources;
	sources.push_back(Source(width / 2 + 10 * hexRadius * cos(PI / 6), height / 2 + 3 * hexRadius));
	sources.push_back(Source(width / 2 - 10 * hexRadius * cos(PI / 6), height / 2 - 3 * hexRadius));

	int tick = 0;
	int timeSum = 0;

	while (window.isOpen()) {
		auto startTime = std::chrono::high_resolution_clock::now();
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) || event.type == sf::Event::Closed)
				window.close();
		}

		//calculations
		std::vector<std::thread> threads;
		for (int i = 0; i < sources.size(); i++) {
			threads.push_back(std::thread(sourceUpdate, std::ref(sources[i]), tick));
		}
		for (int i = 0; i < sources.size(); i++) {
			threads[i].join();
		} 

		//draw
		//draws background
		window.clear(sf::Color::Black);
		if (showBackground) {
			for (int i = 0; i < hexs.size(); i++) {
				window.draw(hexs[i]);
			}
		}
		
		//draws lights and their trails	
		for (int i = 0; i < sources.size(); i++) {
			for (int j = 0; j < sources[i].lights.size(); j++) {
				window.draw(sources[i].lights[j].shape);
				for (int k = 0; k < sources[i].lights[j].trail.size(); k++) {
					window.draw(sources[i].lights[j].trail[k]);
				} 
				for (int k = 0; k < sources[i].lights[j].particles.size(); k++) {
					window.draw(sources[i].lights[j].particles[k]);
				} 
			}
		} 
		window.display(); 
		

		tick++;
		std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - startTime;

		timeSum += round(elapsed.count() * 1000);
		printf("\nTick: %3d: %3.0f (ms), Average: %3.0f (ms), Lights: %3d / %3d", tick, round(elapsed.count() * 1000), round(timeSum / tick), numLight, maxNumLights);
		//cout << "\nTick " << tick << ": " << round(elapsed.count() * 1000) << " (ms), Average: " << timeSum / tick << " (ms)";
	}
	return 0;
}
