#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <utility>
//! LEFT OFF 32:32
using namespace std;
sf::Vector2u WINDOW_SIZE{800,480};

struct Map {
	Map() {
		nBorderWidth = 4;
		nCellSize = 32;
		nMapWidth = WINDOW_SIZE.x / nCellSize;
		nMapHeight = WINDOW_SIZE.y / nCellSize;

		nStartX = 3;
		nStartY = 7;

		nEndX = 12;
		nEndY = 7;

		nWave = 1;

		bEightConnectivity = false;
		bShowText = false;
		bShowArrows = false;

		bObstacleMap = new bool[nMapWidth * nMapHeight]{false};
		nFlowFieldZ = new int[nMapWidth * nMapHeight]{0};
		fFlowFieldX = new float[nMapWidth * nMapHeight]{0};
		fFlowFieldY = new float[nMapWidth * nMapHeight]{0};

			for(int x = 0; x < nMapWidth; x++) {
			for(int y= 0; y < nMapHeight; y++) {
				sfShape.setFillColor(sf::Color::Blue);
				sfShape.setSize(sf::Vector2f{(float)nCellSize - nBorderWidth,(float)nCellSize - nBorderWidth});
			}
		}

			sfFont.loadFromFile("assets/fonts/Amita-Bold.ttf");
			sfText.setFont(sfFont);
			sfText.setCharacterSize(14);
			sfText.setFillColor(sf::Color::White);
			sfText.setString(" ");
			sfText.setStyle(sf::Text::Style::Bold);
			
	}

	~Map() {
		delete[] bObstacleMap;
		delete[] nFlowFieldZ;
	
	}

	bool onUserUpdate(sf::RenderWindow* window) {

		// little convience lambda 2D -> 1D
		auto p = [&](int x, int y) { return y * nMapWidth + x; };

		// user input
		sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);

		int nSelectedCellX = sfMousePos.x / nCellSize;
		int nSelectedCellY = sfMousePos.y / nCellSize;

			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
				// toggle obstacal if left mouse is clicked
				bObstacleMap[p(nSelectedCellX, nSelectedCellY)] = !bObstacleMap[p(nSelectedCellX, nSelectedCellY)];
			}


			// Start Goal (Right Click)
			if (sf::Mouse::isButtonPressed(sf::Mouse::Right) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
				nStartX = nSelectedCellX;
				nStartY = nSelectedCellY;
			}

			// End Goal (Left Control + Right Click)
			if (sf::Mouse::isButtonPressed(sf::Mouse::Right) && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
				nEndX = nSelectedCellX;
				nEndY = nSelectedCellY;
			}

		// resets and create a border wall of obstacales
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
			for(auto x = 0; x  < nMapWidth; x++)
				for(auto y =0; y < nMapHeight; y++) {
					bObstacleMap[p(x, y)] = false;
					if (x == 0 || y == 0 || x == nMapWidth - 1 || y == nMapHeight - 1)
						bObstacleMap[p(x, y)] = !bObstacleMap[p(x, y)];
				}
		}

		// toggle 8 way connectivity
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Num8)) {
			bEightConnectivity = !bEightConnectivity;
		}

		// toggles text on blocks
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::T)) {
			bShowText = !bShowText;
		}

			// toggles text on blocks
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
			bShowArrows = !bShowArrows;
		}

		// clear all blocks
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
				for(auto x = 0; x  < nMapWidth; x++)
				for(auto y =0; y < nMapHeight; y++) {
					bObstacleMap[p(x, y)] = false;
				}
		}


		// propagate the wave to see the path choosen Q/A
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Equal)) {
			++nWave;
			if (nWave > nMapWidth - 1)
				nWave = nMapWidth - 1;
			std::cout << nWave << "\n";
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Dash)) {
			--nWave;
			if (nWave == 0)
				nWave = 1;
		}		
		// 1) Prepare the flow field, add a boundary, and add obstacles by setting the flow field height (Z) to -1
		for (int x = 0; x < nMapWidth; x++) {
			for (int y = 0; y < nMapHeight; y++) {
				// Set border or obstacles 
				if(x == 0 || y == 0 || x == (nMapWidth - 1) || y == (nMapHeight - 1) || bObstacleMap[p(x,y)]) {
					nFlowFieldZ[p(x, y)] = -1;
				}
				else {
					nFlowFieldZ[p(x, y)] = 0;
				}
			}
		}

		// Propagate a wave (i.e. flood-fill) from target locations. We use a tuple of {x, y, distance} - though you could use a struct or similar
		std::list<std::tuple<int, int, int>> nodes(0);

		// Add the first discovered node - the target location, with a distance of 1
		nodes.emplace_back( nEndX, nEndY, 1 );

		while (!nodes.empty()) {
			// Each node through the discovered nodes may create newly discovered nodes, so I maintain a second list. 
			// It's important not to contaminate the list being iterated through.
			std::list<std::tuple<int, int, int>> newNodes;

			// Now iterate through each discovered node. 
			// If it has neighboring nodes that are empty space and undiscovered, add those locations to the new nodes list.
			for (auto& n : nodes) {

				int x = std::get<0>(n); // Map X-Coordinate
				int y = std::get<1>(n); // Map Y-Coordinate
				int d = std::get<2>(n); // Distance From Target Location

				// Set the distance count for this node. Note: That when we add nodes we add 1 to the distance. 
				// This emulates propagating a wave across the map, where the front of that wave increments each iteration.
				// In this way, we can propagate distance information 'away' from target location'
				nFlowFieldZ[p(x, y)] = d;

				// Add neighbor nodes if unmarked, i.e. their "height" is 0. Any discovered nodes or obstacle will be non-zero

				// Check East
				if ((x + 1) < nMapWidth && nFlowFieldZ[p(x + 1, y)] == 0)
					newNodes.emplace_back(x + 1, y, d + 1);

				// Check West
				if ((x - 1) >= 0 && nFlowFieldZ[p(x - 1, y)] == 0)
					newNodes.emplace_back(x - 1, y, d + 1);

				// Check South
				if ((y + 1) < nMapHeight && nFlowFieldZ[p(x, y + 1)] == 0)
					newNodes.emplace_back(x, y + 1, d + 1);

				// Check North
				if ((y - 1) >= 0 && nFlowFieldZ[p(x, y - 1)] == 0)
					newNodes.emplace_back(x, y - 1, d + 1);
			}



				// We will now have potentially multiple nodes for a single location. This means our algorithm will never complete!
				// So we must remove duplicates form our new node list. 
				// Note: Do away with overhead structures like linked list and sorts for fastest path-finding

				// Sort the nodes - This will stack up nodes that are similar: A, B, B, B, B, C, D, D, E, F, F
				newNodes.sort([&](const std::tuple<int, int, int>& n1, const std::tuple<int, int, int>& n2)
							  {
								  // In this instance, you don't care how the values are sorted, so long as nodes that represent the same location are adjacent in the list.
								  // Remember to use the lambda to get a 1D value for a 2D coordinate.
								  	return p(std::get<0>(n1), std::get<1>(n1)) < p(std::get<0>(n2), std::get<1>(n2));
							  });
				//														   -, -, -,       -,       -,
				// Use "unique" function to remove adjacent duplicates		: A, B, B, B, B, C, D, D, E, F, F
				// and also erase them									: A, B, C, D, E, F
				newNodes.unique([&](const std::tuple<int, int, int>& n1, const std::tuple<int, int, int>& n2)
								{
									return  p(std::get<0>(n1), std::get<1>(n1)) == p(std::get<0>(n2), std::get<1>(n2));
								});

				// We've now processed all the discovered nodes, so clear the list, and add the newly discovered nodes for processing on the next iteration
				nodes.clear();
				nodes.insert(nodes.begin(), newNodes.begin(), newNodes.end());

				// When there are no more newly discovered nodes, we have "flood filled" the entire map. The propagation phase of the algorithm is complete
			}

		// 3) Create path. Starting a start location, create a path of nodes until you reach target location.
		// At each node find the neighbor with the lowest "D" (distance) score.
		std::list<std::pair<int, int>> path;
		path.emplace_back(nStartX, nStartY);
		int nLocX = nStartX;
		int nLocY = nStartY;
		bool bNoPath = false;

		while(!(nLocX == nEndX && nLocY == nEndY) && !bNoPath) {
			// for each neighbor generate another list of tuples
			std::list<std::tuple<int, int, int>> listNeighbors;

			// 4-Way connectivity
			// North
			if ((nLocY - 1) >= 0 && nFlowFieldZ[p(nLocX, nLocY - 1)] > 0)
				listNeighbors.emplace_back(nLocX, nLocY - 1, nFlowFieldZ[p(nLocX, nLocY - 1)]);
			// East
			if ((nLocX + 1) >= 0 && nFlowFieldZ[p(nLocX + 1, nLocY )] > 0)
				listNeighbors.emplace_back(nLocX + 1, nLocY, nFlowFieldZ[p(nLocX + 1, nLocY)]);
			// South
			if ((nLocY + 1) >= 0 && nFlowFieldZ[p(nLocX, nLocY + 1)] > 0)
				listNeighbors.emplace_back(nLocX, nLocY + 1, nFlowFieldZ[p(nLocX, nLocY + 1)]);
			// West
			if ((nLocX - 1) >= 0 && nFlowFieldZ[p(nLocX - 1, nLocY)] > 0)
				listNeighbors.emplace_back(nLocX - 1, nLocY, nFlowFieldZ[p(nLocX - 1, nLocY)]);

			// 8-Way connectivity
			if (bEightConnectivity) {
				if ((nLocY - 1) >= 0 && (nLocX - 1) >= 0 && nFlowFieldZ[p(nLocX - 1, nLocY - 1)] > 0)
					listNeighbors.emplace_back(nLocX - 1, nLocY - 1, nFlowFieldZ[p(nLocX - 1, nLocY - 1)]);

				if ((nLocY - 1) >= 0 && (nLocX + 1) < nMapWidth && nFlowFieldZ[p(nLocX + 1, nLocY - 1)] > 0)
					listNeighbors.emplace_back(nLocX + 1, nLocY - 1, nFlowFieldZ[p(nLocX + 1, nLocY - 1)]);

				if ((nLocY + 1) < nMapHeight && (nLocX - 1) >= 0 && nFlowFieldZ[p(nLocX - 1, nLocY + 1)] > 0)
					listNeighbors.emplace_back(nLocX - 1, nLocY + 1, nFlowFieldZ[p(nLocX - 1, nLocY + 1)]);

				if ((nLocY + 1) < nMapHeight && (nLocX + 1) < nMapWidth && nFlowFieldZ[p(nLocX + 1, nLocY + 1)] > 0)
					listNeighbors.emplace_back(nLocX + 1, nLocY + 1, nFlowFieldZ[p(nLocX + 1, nLocY + 1)]);
			}
			// Sprt neigbours based on height, so lowest neighbour is at front
			// of list
			listNeighbors.sort([&](const std::tuple<int, int, int> &n1, const std::tuple<int, int, int> &n2)
			{
				return std::get<2>(n1) < std::get<2>(n2); // Compare distances
			});

			if (listNeighbors.empty()) // Neighbour is invalid or no possible path
				bNoPath = true;
			else
			{
				nLocX = std::get<0>(listNeighbors.front());
				nLocY = std::get<1>(listNeighbors.front());
				path.push_back({ nLocX, nLocY });
			}
		}

		// 4) Create Flow "Field"
		for(auto x = 0; x < nMapWidth - 1; x++)
			for(auto y = 0; y < nMapHeight - 1; y++) {

				float vx = 0.f;
				float vy = 0.f;

				vy -= static_cast<float>((nFlowFieldZ[p(x, y + 1)] <= 0 ? nFlowFieldZ[p(x, y)] : nFlowFieldZ[p(x, y + 1)]) - nFlowFieldZ[p(x, y)]);
				vx -= static_cast<float>((nFlowFieldZ[p(x + 1, y)] <= 0 ? nFlowFieldZ[p(x, y)] : nFlowFieldZ[p(x + 1, y)]) - nFlowFieldZ[p(x, y)]);
				vy += static_cast<float>((nFlowFieldZ[p(x, y - 1)] <= 0 ? nFlowFieldZ[p(x, y)] : nFlowFieldZ[p(x, y - 1)]) - nFlowFieldZ[p(x, y)]);
				vx += static_cast<float>((nFlowFieldZ[p(x - 1, y)] <= 0 ? nFlowFieldZ[p(x, y)] : nFlowFieldZ[p(x - 1, y)]) - nFlowFieldZ[p(x, y)]);

				float r = 1.0f / sqrtf(vx*vx + vy * vy);
				fFlowFieldX[p(x, y)] = vx * r;
				fFlowFieldY[p(x, y)] = vy * r;
			}

		int ob = 0;
		for (int x = 0; x < nMapWidth; x++) {
			for (int y = 0; y < nMapHeight; y++) {
				
				if (bObstacleMap[p(x, y)])
				{
					sfShape.setFillColor(sf::Color{170,170,170,255});
					++ob;
				}
				else
					sfShape.setFillColor(sf::Color::Blue);

				// shows the wave that is propagated
				if (nWave == nFlowFieldZ[p(x, y)])
					sfShape.setFillColor(sf::Color::Cyan);
				
				if (x == nStartX && y == nStartY)
					sfShape.setFillColor(sf::Color::Green);

				if (x == nEndX && y == nEndY)
					sfShape.setFillColor(sf::Color::Red);


				sfShape.setPosition(x * nCellSize - nBorderWidth, y * nCellSize - nBorderWidth);
				// Draw Base
				window->draw(sfShape);

				// Draw "potential" or "distance" or "height" 
				sfText.setString(std::to_string(nFlowFieldZ[p(x, y)]));
				sfText.setOrigin(sfText.getCharacterSize() / 2, sfText.getCharacterSize()/2-1);
				sfText.setPosition(x* nCellSize + nBorderWidth, y* nCellSize + 1);

				// draw text on blocks
				if(bShowText)
					window->draw(sfText);

				// Drawing Arrow
				if(nFlowFieldZ[p(x,y)] > 0) {
					float ax[4], ay[4];
					
					float fAngle = atan2f(fFlowFieldY[p(x, y)], fFlowFieldX[p(x, y)]);
					float fRadius = static_cast<float>(nCellSize - nBorderWidth) / 2.f;
					
					int offsetX = x * nCellSize + ((nCellSize - nBorderWidth) / 2);
					int offsetY = y * nCellSize + ((nCellSize - nBorderWidth) / 2);

					ax[0] = cosf(fAngle) * fRadius + offsetX;
					ay[0] = sinf(fAngle) * fRadius + offsetY;

					ax[1] = cosf(fAngle) * -fRadius + offsetX;
					ay[1] = sinf(fAngle) * -fRadius + offsetY;

					ax[2] = cosf(fAngle + 0.1f) * fRadius * 0.7f + offsetX;
					ay[2] = sinf(fAngle + 0.1f) * fRadius * 0.7f + offsetY;

					ax[3] = cosf(fAngle - 0.1f) * fRadius * 0.7f + offsetX;
					ay[3] = sinf(fAngle - 0.1f) * fRadius * 0.7f + offsetY;

					sf::Vertex arrow[] =
					{
						sf::Vertex(sf::Vector2f(ax[0],ay[0])),
						sf::Vertex(sf::Vector2f(ax[1],ay[1])),
			
						sf::Vertex(sf::Vector2f(ax[0],ay[0])),
						sf::Vertex(sf::Vector2f(ax[2],ay[2])),
						
						sf::Vertex(sf::Vector2f(ax[0],ay[0])),
						sf::Vertex(sf::Vector2f(ax[3],ay[3]))
					};

					// draw arrows
					if(bShowArrows)
						window->draw(arrow, 6, sf::Lines);
				}
			}
		}

		// extract first point seperately for visual appeasing
		bool bFirstPoint = true;
		int ox, oy;

		for(auto& a : path) {
			if(bFirstPoint)
			{
				// access pairs
				ox = a.first;
				oy = a.second;
				bFirstPoint = false;
			}
			else {

				sf::Vertex line[] =
				{
					sf::Vertex(sf::Vector2f(static_cast<float>(ox) * nCellSize + ((nCellSize - nBorderWidth) / 2), 
													  static_cast<float>(oy)* nCellSize + ((nCellSize - nBorderWidth) / 2))),
					sf::Vertex(sf::Vector2f(static_cast<float>(a.first) * nCellSize + ((nCellSize - nBorderWidth) / 2), 
													  static_cast<float>(a.second) * nCellSize + ((nCellSize - nBorderWidth) / 2)))
				};

				line->color = sf::Color::Yellow;
			
				window->draw(line,2,sf::Lines);

				ox = a.first;
				oy = a.second;
			
				sf::CircleShape circle(9);
				circle.setOrigin(11,9);		//? manually set
				circle.setPosition(static_cast<float>(ox) * nCellSize + ((nCellSize - nBorderWidth) / 2),
								   static_cast<float>(oy) * nCellSize + ((nCellSize - nBorderWidth) / 2));
				circle.setFillColor(sf::Color::Yellow);
		

				window->draw(circle);


				auto title = "[W]all Reset [" +std::to_string(ob) +"]    [C]lear    [=]/[-] WaveLength" +
							 "    [8]-Connect:"  + std::to_string(static_cast<int>(bEightConnectivity)) +
							 "    Show[T]ext:"   + std::to_string(static_cast<int>(bShowText)) + 
							 "    Show[A]rrows:" + std::to_string(static_cast<int>(bShowArrows));
				window->setTitle(title);
			}
		}

		return true;
	}
	int nMapWidth;
	int nMapHeight;
	int nCellSize;
	int nBorderWidth;
	sf::RectangleShape sfShape;
	bool* bObstacleMap;

	int nStartX;
	int nEndX;

	int nStartY;
	int nEndY;

	int* nFlowFieldZ;		// represents 'D' value
	float* fFlowFieldX;
	float* fFlowFieldY;

	int nWave;
	bool bEightConnectivity;
	bool bShowText;
	bool bShowArrows;

	sf::Font sfFont;
	sf::Text sfText;
};

int main() {
	sf::RenderWindow window{sf::VideoMode{WINDOW_SIZE.x,WINDOW_SIZE.y},"SFML Sandbox"};
	window.setFramerateLimit(10);
	window.setPosition(sf::Vector2i{window.getPosition().x,0});
	sf::Clock dtClock;

	Map grid;

	while(window.isOpen()) {
		sf::Event event;
		while(window.pollEvent(event)) {
			if(event.type == sf::Event::Closed || event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
				window.close();
			if(event.type == sf::Event::KeyPressed) {
				switch(event.key.code) {
				case sf::Keyboard::Enter: cout<<"Enter Pressed\n";break;
				case sf:: Keyboard::Space: cout<<"Space Pressed\n"; break;
					default: break;
				}
			}
		}

	
		window.clear();

		grid.onUserUpdate(&window);

		window.setView(window.getDefaultView());
		window.display();

	}
	return 0;
}