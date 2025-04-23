/*

For my set up LED_GREEN, and LED_RED ended up being switched ¯\_(ツ)_/¯

*/


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <Fonts/TomThumb.h>

const uint8_t SWITCH_BUTTON = 4;
Adafruit_BicolorMatrix matrix = Adafruit_BicolorMatrix();

const int GRID_SIZE = 8;
const unsigned long UPDATE_INTERVAL = 200;
const unsigned long INPUT_CHECK_INTERVAL = 50;

bool isFlappyBird = true;

void centerText(int value) {
    char buffer[3];
    snprintf(buffer, sizeof(buffer), "%d", value);
    
    int16_t x1, y1;
    uint16_t w, h;
    matrix.getTextBounds(buffer, 0, 0, &x1, &y1, &w, &h);
    
    int x = (8 - w) / 2;
    int y = (8 + h) / 2;
    
    matrix.clear();
    matrix.setCursor(x, y);
    
    matrix.print(buffer);
    matrix.writeDisplay();
}

void playIntro() {
    const char* gamesByChris = "Games by Chris"; // change if you want
    int x = matrix.width();
    
    matrix.setTextColor(LED_GREEN);

    for(int t = 0; t < 59; t++) {
        matrix.clear();
        matrix.setCursor(x, 5);
        matrix.print(gamesByChris);
        
        if (--x < -100) x = matrix.width();
        matrix.writeDisplay();
        delay(100);
    }
    matrix.setTextColor(LED_YELLOW);
}

class FlappyBird {
public:
    FlappyBird() { reset(); }
    
    void run() {
        update();
        render();
    }
    
    void jump() {
        if (gameOver) {
            reset();
        } else {
            velocity = JUMP_STRENGTH;
        }
    }

private:
    static constexpr int BIRD_X = 1;
    static constexpr int GRAVITY = 1;
    static constexpr int JUMP_STRENGTH = -2;
    static constexpr int GAP_SIZE = 3;
    
    int birdY, velocity, pipeX, gapY, score;
    bool gameOver;

    void reset() {
        birdY = 3;
        velocity = 0;
        pipeX = 7;
        gapY = random(1, 6);
        gameOver = false;
        score = 0;
        matrix.setTextColor(LED_YELLOW);
    }

    void update() {
        if (gameOver) return;
        
        velocity += GRAVITY;
        birdY += velocity;
        
        if (birdY < 0 || birdY > 7) gameOver = true;
        
        if (--pipeX < 0) { 
            pipeX = 7; 
            gapY = random(1, 6); 
            score++; 
        }
        
        if (BIRD_X == pipeX && (birdY < gapY || birdY > gapY + GAP_SIZE - 1)) 
            gameOver = true;
    }

    void render() {
        matrix.clear();
        
        if (!gameOver) {
            matrix.drawPixel(BIRD_X, birdY, LED_GREEN);
            
            for (int i = 0; i < 8; i++) {
                if (i < gapY || i > gapY + GAP_SIZE - 1) {
                    matrix.drawPixel(pipeX, i, LED_RED);
                }
            }
        } else {
            centerText(score);
        }
        
        matrix.writeDisplay();
    }
};

class SnakeGame {
public:
    enum Direction { UP, DOWN, LEFT, RIGHT };
    
    SnakeGame() { reset(); }
    
    void run() {
        update();
        render();
    }
    
    void changeDirection(Direction newDir) {
        if ((dir == UP && newDir != DOWN) || 
            (dir == DOWN && newDir != UP) ||
            (dir == LEFT && newDir != RIGHT) || 
            (dir == RIGHT && newDir != LEFT)) {
            dir = newDir;
        }
    }
private:
    struct Point { int x, y; };
    
    Point snake[GRID_SIZE * GRID_SIZE];
    int length;
    Direction dir;
    int foodX, foodY;
    
    void reset() {
        length = 3;
        dir = RIGHT;
        
        for (int i = 0; i < length; i++) {
            snake[i] = { GRID_SIZE / 2 - i, GRID_SIZE / 2 };
        }
        
        spawnFood();
        matrix.setTextColor(LED_YELLOW);
    }

    void update() {
        Point newHead = snake[0];
        
        switch(dir) {
            case UP:    newHead.y--; break;
            case DOWN:  newHead.y++; break;
            case LEFT:  newHead.x--; break;
            case RIGHT: newHead.x++; break;
        }
        
        newHead.x = (newHead.x + GRID_SIZE) % GRID_SIZE;
        newHead.y = (newHead.y + GRID_SIZE) % GRID_SIZE;
        
        for (int i = 0; i < length; i++) {
            if (snake[i].x == newHead.x && snake[i].y == newHead.y) {
                centerText(length - 3);
                delay(2000);
                reset();
                return;
            }
        }
        
        for (int i = length; i > 0; i--) snake[i] = snake[i - 1];
        snake[0] = newHead;
        
        if (newHead.x == foodX && newHead.y == foodY) {
            length++;
            spawnFood();
        }
    }

    void render() {
        matrix.clear();
        
        for (int i = 0; i < length; i++) 
            matrix.drawPixel(snake[i].x, snake[i].y, LED_GREEN);
        
        matrix.drawPixel(foodX, foodY, LED_RED);
        matrix.writeDisplay();
    }

    void spawnFood() { 
        foodX = random(0, GRID_SIZE); 
        foodY = random(0, GRID_SIZE); 
    }
};

FlappyBird flappyBird;
SnakeGame snakeGame;

void setup() {
    matrix.begin(0x70);
    matrix.setBrightness(5);
    matrix.setFont(&TomThumb);
    matrix.setTextWrap(false);
    matrix.setRotation(0);

    playIntro();
    
    pinMode(SWITCH_BUTTON, INPUT);
    pinMode(0, INPUT);
    pinMode(1, INPUT);
    pinMode(2, INPUT);
    pinMode(3, INPUT);

}

void handleGameInput() {
    if (!isFlappyBird) {
        if (digitalRead(0) == LOW) snakeGame.changeDirection(SnakeGame::UP);
        if (digitalRead(3) == LOW) snakeGame.changeDirection(SnakeGame::DOWN);
        if (digitalRead(2) == LOW) snakeGame.changeDirection(SnakeGame::LEFT);
        if (digitalRead(1) == LOW) snakeGame.changeDirection(SnakeGame::RIGHT);
    } else {
        if (digitalRead(0) == LOW) {
            flappyBird.jump();
        }
    }
}

void loop() {
    static unsigned long lastUpdate = 0;
    static unsigned long lastInputCheck = 0;
    unsigned long currentMillis = millis();
    
    if (digitalRead(SWITCH_BUTTON) == LOW) {
        isFlappyBird = !isFlappyBird;
        playIntro();
        delay(500);
    }
    
    if (currentMillis - lastInputCheck >= INPUT_CHECK_INTERVAL) {
        lastInputCheck = currentMillis;
        handleGameInput();
    }
    
    if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = currentMillis;
        isFlappyBird ? flappyBird.run() : snakeGame.run();
    }
}