// EndlessPacman.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <SFML/Graphics.hpp>

#define GRID_SIZE 64
#define FIXED_UPDATE_HZ 144.0f
#define FIXED_UPDATE_SECONDS (1.0f/FIXED_UPDATE_HZ)

namespace WallDirection
{
	enum Type
	{
		NW,
		N,
		NE,
		W,
		CENTER,
		E,
		SW,
		S,
		SE,
		COUNT,
	};
}

namespace WallSprite
{
	enum Type
	{
		NW,
		N,
		NE,
		N_END,
		W,
		CENTER,
		E,
		W_END,
		SW,
		S,
		SE,
		S_END,
		E_END,
		NS_SPAN,
		EW_SPAN,
		ISLAND,
		COUNT,
	};
}

enum MoveDir
{	
	LEFT,
	UP,
	RIGHT,
	DOWN,	
};

enum CellState
{
	HAS_PILL,
	BLOCK,
	EMPTY,
};

struct Vec2f
{
	float x, y;
	Vec2f operator *(float value) const { Vec2f result{ x,y }; result.x *= value; result.y *= value; return result; }
	Vec2f operator +(float value) const{ Vec2f result{ x,y }; result.x += value; result.y += value; return result; }
	Vec2f operator +(const Vec2f& rhs) const { Vec2f result{ x,y }; result += rhs; return result; }
	void operator *=(float rhs) { x *= rhs; y *= rhs; }
	void operator +=(const Vec2f& rhs) { x += rhs.x; y += rhs.y; }
};

struct Vec2i
{
	int x, y;
	operator Vec2f() { return Vec2f({ (float)x, (float)y }); }
	Vec2i operator *(int value) const { Vec2i result{ x,y }; result.x *= value; result.y *= value; return result; }
	Vec2i operator -(const Vec2i& rhs) const { Vec2i result{ x,y }; result += rhs * -1; return result; }
	Vec2i operator +(const Vec2i& rhs) const { Vec2i result{ x,y }; result += rhs; return result; }
	void operator +=(const Vec2i& rhs) { x += rhs.x; y += rhs.y; }
	bool operator ==(const Vec2i& rhs) const { return x == rhs.x && y == rhs.y; }
};

struct Cell
{
	int x, y;	
	int state;
	WallSprite::Type type;
};

struct Board
{		
	Board(const char* fileName)
	{
		m_initialized = false;
		m_texture.loadFromFile("tilesheet.png");

		for (int i = 0; i < WallSprite::COUNT; i++)
		{
			m_wallSprites[i].setTexture(m_texture);
			m_wallSprites[i].setTextureRect(sf::IntRect(GRID_SIZE * (i%4), GRID_SIZE * (i/4),GRID_SIZE, GRID_SIZE));
		}

		std::ifstream file(fileName);
		int row = 0;
		int col = 0;
		int maxColumns = 0;
		if (file.is_open())
		{
			std::string line;
			while (std::getline(file, line))
			{
				for (int i = 0; i<line.size(); i++)
				{
					Cell cell;
					cell.x = col;
					cell.y = row;
					cell.state = CellState::EMPTY;
					if (line.at(i) == 'X')
					{
						cell.state = CellState::BLOCK;
					}
					else
					{
						cell.state = CellState::HAS_PILL;
					}

					m_cells.push_back(cell);
					col++;
					if (maxColumns < col)
					{
						maxColumns = col;
					}
				}
				m_cols = col;
				m_rows = row;
				row++;
				col = 0;
			}
			file.close();
			m_initialized = true;

			AssignTypes();
		}		
	}

	bool IsBlocked(WallDirection::Type dir, Cell** surroundingCells)
	{
		return surroundingCells[dir] == nullptr || surroundingCells[dir]->state == CellState::BLOCK;
	}

	void AssignTypes()
	{
		for (int i = 0; i < m_cells.size(); i++)
		{
			m_cells[i].type = WallSprite::NE;

			Cell* surroundingCells[WallDirection::COUNT];
			for (int j = 0; j < WallDirection::COUNT; j++)
			{
				surroundingCells[j] = GetCell(m_cells[i].x + ((j % 3) - 1), m_cells[i].y + (j / 3 - 1));
			}

			// see if NW
			if (!IsBlocked(WallDirection::W, surroundingCells) && 
				!IsBlocked(WallDirection::N, surroundingCells) &&
				IsBlocked(WallDirection::E, surroundingCells) &&
				IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::NW;
			}
			// check if N
			else if (IsBlocked(WallDirection::W, surroundingCells) &&
				!IsBlocked(WallDirection::N, surroundingCells) &&
				IsBlocked(WallDirection::E, surroundingCells) &&
				IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::N;
			}
			// check if NE
			else if (IsBlocked(WallDirection::W, surroundingCells) &&
				!IsBlocked(WallDirection::N, surroundingCells) &&
				!IsBlocked(WallDirection::E, surroundingCells) &&
				IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::NE;
			}
			// check if E
			else if (IsBlocked(WallDirection::W, surroundingCells) &&
				IsBlocked(WallDirection::N, surroundingCells) &&
				!IsBlocked(WallDirection::E, surroundingCells) &&
				IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::E;
			}
			// check if SE
			else if (IsBlocked(WallDirection::W, surroundingCells) &&
				IsBlocked(WallDirection::N, surroundingCells) &&
				!IsBlocked(WallDirection::E, surroundingCells) &&
				!IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::SE;
			}
			// check if S
			else if (IsBlocked(WallDirection::W, surroundingCells) &&
				IsBlocked(WallDirection::N, surroundingCells) &&
				IsBlocked(WallDirection::E, surroundingCells) &&
				!IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::S;
			}
			// check if SW
			else if (!IsBlocked(WallDirection::W, surroundingCells) &&
				IsBlocked(WallDirection::N, surroundingCells) &&
				IsBlocked(WallDirection::E, surroundingCells) &&
				!IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::SW;
			}
			// check if W
			else if (!IsBlocked(WallDirection::W, surroundingCells) &&
				IsBlocked(WallDirection::N, surroundingCells) &&
				IsBlocked(WallDirection::E, surroundingCells) &&
				IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::W;
			}
			// check if N_END
			else if (!IsBlocked(WallDirection::W, surroundingCells) &&
				!IsBlocked(WallDirection::N, surroundingCells) &&
				!IsBlocked(WallDirection::E, surroundingCells) &&
				IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::N_END;
			}
			// check if E_END
			else if (IsBlocked(WallDirection::W, surroundingCells) &&
				!IsBlocked(WallDirection::N, surroundingCells) &&
				!IsBlocked(WallDirection::E, surroundingCells) &&
				!IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::E_END;
			}
			// check if S_END
			else if (!IsBlocked(WallDirection::W, surroundingCells) &&
				IsBlocked(WallDirection::N, surroundingCells) &&
				!IsBlocked(WallDirection::E, surroundingCells) &&
				!IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::S_END;
			}
			// check if W_END
			else if (!IsBlocked(WallDirection::W, surroundingCells) &&
				!IsBlocked(WallDirection::N, surroundingCells) &&
				IsBlocked(WallDirection::E, surroundingCells) &&
				!IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::W_END;
			}
			// check if NS_SPAN
			else if (!IsBlocked(WallDirection::W, surroundingCells) &&
				IsBlocked(WallDirection::N, surroundingCells) &&
				!IsBlocked(WallDirection::E, surroundingCells) &&
				IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::NS_SPAN;
			}
			// check if EW_SPAN
			else if (IsBlocked(WallDirection::W, surroundingCells) &&
				!IsBlocked(WallDirection::N, surroundingCells) &&
				IsBlocked(WallDirection::E, surroundingCells) &&
				!IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::EW_SPAN;
			}
			// Check if Island
			else if (!IsBlocked(WallDirection::W, surroundingCells) &&
				!IsBlocked(WallDirection::N, surroundingCells) &&
				!IsBlocked(WallDirection::E, surroundingCells) &&
				!IsBlocked(WallDirection::S, surroundingCells))
			{
				m_cells[i].type = WallSprite::ISLAND;
			}
			else
			{
				m_cells[i].type = WallSprite::CENTER;
			}
		}
	}

	void Draw(sf::RenderWindow& window)
	{
		circleShape.setRadius(GRID_SIZE * .15f);
		circleShape.setFillColor(sf::Color(255, 255, 50));
		circleShape.setOrigin(-GRID_SIZE * .5f + circleShape.getRadius(), -GRID_SIZE * .5f + circleShape.getRadius());

		for (int i = 0; i < m_cells.size(); i++)
		{		
			m_wallSprites[m_cells[i].type].setPosition(m_cells[i].x * GRID_SIZE, m_cells[i].y * GRID_SIZE);
			circleShape.setPosition(m_cells[i].x * GRID_SIZE, m_cells[i].y * GRID_SIZE);
			if (m_cells[i].state == CellState::BLOCK)
			{
				window.draw(m_wallSprites[m_cells[i].type]);
			}
			else
			{
				if (m_cells[i].state == CellState::HAS_PILL)
				{
					window.draw(circleShape);
				}
			}
		}
	}	

	Cell* GetCell(int x, int y)
	{
		if (x >= 0 &&
			x < m_cols &&
			y >= 0 &&
			y < m_rows)
		{
			return &m_cells[x + y * m_cols];
		}
		return nullptr;
	}

	Cell* GetCell(const Vec2i& pos)
	{
		return GetCell(pos.x, pos.y);
	}

	bool Initialized()
	{
		return m_initialized;
	}

	int m_cols;
	int m_rows;
	std::vector<Cell> m_cells;	
	sf::CircleShape circleShape;	
	sf::RectangleShape gridCell;
	sf::Sprite m_wallSprites[WallSprite::COUNT];
	sf::Texture m_texture;
	bool m_initialized;
};

class Player
{
public:
	Player(Vec2i initialPos)
	{
		m_moveDir = MoveDir::LEFT;				
		m_gridPos = initialPos;
		m_pos = initialPos;
		m_texture.loadFromFile("pacman.png");		
		m_shape.setSize(sf::Vector2f(GRID_SIZE, GRID_SIZE));
		m_shape.setTexture(&m_texture,true);
		m_reachedDestination = true;
	}
	void Draw(sf::RenderWindow& window)
	{
		m_shape.setPosition(m_pos.x * GRID_SIZE, m_pos.y* GRID_SIZE);
		window.draw(m_shape);
	}

	void FixedUpdate(Board* board)
	{		
		MoveDir desiredMoveDir = m_moveDir;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
		{
			desiredMoveDir = MoveDir::RIGHT;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
		{
			desiredMoveDir = MoveDir::DOWN;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
		{
			desiredMoveDir = MoveDir::LEFT;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
		{
			desiredMoveDir = MoveDir::UP;
		}

		if (!MovePlayer(desiredMoveDir, board))
		{
			desiredMoveDir = m_moveDir;
			MovePlayer(desiredMoveDir, board);
		}
		
		float speed = 15;
		switch (desiredMoveDir)
		{
		case MoveDir::LEFT:
			m_pos.x = std::max<float>(m_pos.x - speed * FIXED_UPDATE_SECONDS, m_gridPos.x);
			break;
		case MoveDir::UP:
			m_pos.y = std::max<float>(m_pos.y - speed * FIXED_UPDATE_SECONDS, m_gridPos.y);
			break;
		case MoveDir::RIGHT:
			m_pos.x = std::min<float>(m_pos.x + speed * FIXED_UPDATE_SECONDS, m_gridPos.x);
			break;
		case MoveDir::DOWN:
			m_pos.y = std::min<float>(m_pos.y + speed * FIXED_UPDATE_SECONDS, m_gridPos.y);
			break;
		}

		//printf("Position was [%f,%f] desired[%d,%d]\n", m_pos.x, m_pos.y, m_gridPos.x, m_gridPos.y);
		
		if (m_pos.x == m_gridPos.x &&
			m_pos.y == m_gridPos.y)
		{
			//printf("Reached destination [%d,%d]\n", m_gridPos.x, m_gridPos.y);
			m_reachedDestination = true;
			board->GetCell(m_gridPos)->state = CellState::EMPTY;
		}		
	}

	bool MovePlayer(MoveDir desiredMoveDir, Board * board)
	{
		bool useInput = false;
		if ((int)desiredMoveDir % 2 != (int)m_moveDir % 2)
		{
			useInput = m_reachedDestination && desiredMoveDir != m_moveDir;
		}
		else
		{
			useInput = m_reachedDestination || desiredMoveDir != m_moveDir;
		}

		if (useInput)
		{			
			Vec2f velocity = { 0,0 };
			Vec2i desiredGridOffset = { 0,0 };

			switch (desiredMoveDir)
			{
			case MoveDir::LEFT:
				desiredGridOffset.x = -1;
				break;
			case MoveDir::UP:
				desiredGridOffset.y = -1;
				break;
			case MoveDir::RIGHT:
				desiredGridOffset.x = 1;
				break;
			case MoveDir::DOWN:
				desiredGridOffset.y = 1;
				break;
			}

			Vec2i newGridPos = m_gridPos + desiredGridOffset;
			if (newGridPos.x >= 0 && newGridPos.x < board->m_cols &&
				newGridPos.y >= 0 && newGridPos.y < board->m_rows &&
				board->GetCell(newGridPos)->state != CellState::BLOCK)
			{
				m_reachedDestination = false;
				m_gridPos = newGridPos;				
				m_moveDir = desiredMoveDir;				
				return true;
			}
		}
		return false;
	}
	
	MoveDir m_moveDir;	
	Vec2f m_pos;
	Vec2f m_vel;	
	Vec2i m_gridPos;
	sf::RectangleShape m_shape;
	sf::Texture m_texture;
	bool m_reachedDestination;
};

int main()
{
	Player player(Vec2i({ 9, 12 }));

	Board board("level_2.txt");

	if (!board.Initialized())
	{
		return 1;
	}

	sf::RenderWindow window(sf::VideoMode(1280,720), "SFML works!");
	sf::View fixed = window.getView();	
	//fixed.setViewport(sf::FloatRect( 0,0,300,300));
	float scale = 3;
	fixed.zoom(scale);
	fixed.setCenter(sf::Vector2f(window.getSize().x * scale * 0.5f, window.getSize().y * scale * 0.5f));
	window.setView(fixed);

	int commandFrame = 0;
	float timeElapsed = 0.0f;
	float appTimeElapsed = 0.0f;
	sf::Clock clock;
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// Game Loop
		sf::Time elapsed = clock.restart();
		timeElapsed += elapsed.asSeconds();
		appTimeElapsed += elapsed.asSeconds();
		while (timeElapsed > FIXED_UPDATE_SECONDS)
		{
			// Ghetto spiral of death handling so I don't lock up
			if (timeElapsed > 1)
			{
				timeElapsed = FIXED_UPDATE_SECONDS;
			}

			timeElapsed -= FIXED_UPDATE_SECONDS;
			// Do Update
			//printf("Doing update %d with timeElapsed %f : TotalTime %f\n", commandFrame, timeElapsed, appTimeElapsed);
			commandFrame++;

			player.FixedUpdate(&board);

			// Do Render
			window.clear();			
			
			board.Draw(window);
			player.Draw(window);
			
			window.display();
		}		
	}

	return 0;
}