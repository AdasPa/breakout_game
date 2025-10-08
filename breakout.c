/***************************************************************************************************
 * DON'T REMOVE THE VARIABLES BELOW THIS COMMENT                                                   *
 **************************************************************************************************/
unsigned long long __attribute__((used)) VGAaddress = 0xc8000000; // Memory storing pixels
unsigned int __attribute__((used)) red = 0x0000F0F0;
unsigned int __attribute__((used)) green = 0x00000F0F;
unsigned int __attribute__((used)) blue = 0x000000FF;
unsigned int __attribute__((used)) white = 0x0000FFFF;
unsigned int __attribute__((used)) black = 0x0;

// Don't change the name of this variables
#define NCOLS 10 // <- Supported value range: [1,18]
#define NROWS 16 // <- This variable might change.
#define TILE_SIZE 15 // <- Tile size, might change.


char *won = "You Won";       // DON'T TOUCH THIS - keep the string as is
char *lost = "You Lost";     // DON'T TOUCH THIS - keep the string as is
unsigned short height = 240; // DON'T TOUCH THIS - keep the value as is
unsigned short width = 320;  // DON'T TOUCH THIS - keep the value as is
char font8x8[128][8];        // DON'T TOUCH THIS - this is a forward declaration
unsigned char tiles[NROWS][NCOLS] __attribute__((used)) = { 0 }; // DON'T TOUCH THIS - this is the tile map
/**************************************************************************************************/

/***
 * TODO: Define your variables below this comment
 */

 
#define VGA_WIDTH 320
#define VGA_HEIGHT 240
#define FRAME_SKIP 1024

//TODOOOOOOO:
// continue colision detection
// fix restart, to Clear Screen inside

int frame_counter = 0;
const unsigned int PLAYING_FIELD_TOP_LEFT_X = VGA_WIDTH - NCOLS * TILE_SIZE;

/***
 * You might use and modify the struct/enum definitions below this comment
 */
typedef struct _block
{
    unsigned char destroyed;
    unsigned char deleted;
    unsigned int pos_x;
    unsigned int pos_y;
    unsigned int color;
} Block;

typedef enum _gameState
{
    Stopped = 0,
    Running = 1,
    Won = 2,
    Lost = 3,
    Exit = 4,
} GameState;
GameState currentState = Stopped;

typedef struct _ball
{
    unsigned int pos_x;  // X-coordinate of ball center (0 to 319)
    unsigned int pos_y;  // Y-coordinate of ball center (0 to 239)
    int vx;              // X velocity (e.g., -1, 0, 1)
    int vy;              // Y velocity (e.g., -1, 0, 1)
    unsigned int color;
} Ball;
Ball ball;

typedef struct _bar
{
    unsigned int pos_y;  // Y-coordinate of bar top (0 to 239)
    int vy;              // Y velocity (e.g., -1, 0, 1)
} Bar;
Bar bar;


/***
 * Here follow the C declarations for our assembly functions
 */

// TODO: Add a C declaration for the ClearScreen assembly procedure
// Don't modify the function headers
void SetPixel(unsigned int x_coord, unsigned int y_coord, unsigned int color);
int ReadUart();
void WriteUart(char c);

/***
 * Now follow the assembly implementations
 */

// It must only clear the VGA screen, and not clear any game state
asm("ClearScreen: \n\t"
    "    PUSH {LR} \n\t"
    "    PUSH {R4, R5} \n\t"
    // DONE: Add ClearScreen implementation in assembly here
    // Sets all the memory from =VGAaddress to =VGAaddress + 1024*240 to 0 -> white

   "    ldr r0, =VGAaddress \n\t"
   "    ldr r0, [r0] \n\t"
    "    mov r1, #0 \n\t"
    "    add r1, r0, #245760 \n\t" // first address out of vga memory (=VGAaddress + 1024*240)
    "    mov r2, #0xffffffff \n\t"         // white
    "clear_loop: \n\t"
    "    strh r2, [r0], #2 \n\t"
    
    "    cmp r0, r1 \n\t"
    "    blt clear_loop \n\t"

    "    POP {R4,R5}\n\t"
    "    POP {LR} \n\t"
    "    BX LR");

// assumes R0 = x-coord, R1 = y-coord, R2 = colorvalue
asm("SetPixel: \n\t"
    "LDR R3, =VGAaddress \n\t"
    "LDR R3, [R3] \n\t"
    "LSL R1, R1, #10 \n\t"
    "LSL R0, R0, #1 \n\t"
    "ADD R1, R0 \n\t"
    "STRH R2, [R3,R1] \n\t"
    "BX LR");

asm("ReadUart:\n\t"
    "LDR R1, =0xFF201000 \n\t"
    "LDR R0, [R1]\n\t"
    "BX LR");

// TODO: Add the WriteUart assembly procedure here that respects the WriteUart C declaration on line 46

asm("WriteUart: \n\t"
    "    LDR R1, =0xFF201000 \n\t" // Control register
    "wait_space: \n\t"
    "    LDR R2, [R1, #4] \n\t"       // Read control register
    "    LSR R2, R2, #16 \n\t"    // Shift to get WSPACE (bits 31-16)
    "    CMP R2, #0 \n\t"         // Compare with 0
    "    BEQ wait_space \n\t"     // Loop if no space
    "    STR R0, [R1] \n\t"       // Write character
    "    BX LR \n\t");


// TODO: Implement the C functions below
// Don't modify any function header
void draw_block(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int color)
{
    for(int i = x; i < x + width; i++)
    {
        if (i > VGA_WIDTH)
            return;

        for(int j = y; j < y + height; j++)
        {
            if (j > VGA_HEIGHT)
                continue;
            SetPixel(i, j, color);
        }
    }
}

void draw_bar(unsigned int y)
{
    if (y > height - 45) {
        y = height - 45; // 240 - 45 = 195
    }
    // Draw three regions: red (top), blue (middle), red (bottom)
    draw_block(0, y, 7, 15, red);         // Top: y to y+14, red
    draw_block(0, y + 15, 7, 15, blue);  // Middle: y+15 to y+29, blue
    draw_block(0, y + 30, 7, 15, red);   // Bottom: y+30 to y+44, red
}

void clear_bar(unsigned int y)
{
    if (y > height - 45) {
        y = height - 45; // 240 - 45 = 195
    }
    // Draw bar rectangle all white
    draw_block(0, y, 7, 45, white);
}

void draw_ball()
{
    int ball_half = 3; // 7x7 diamond
    for (int dy = -ball_half; dy <= ball_half; dy++) 
    {
        for (int dx = -ball_half; dx <= ball_half; dx++) 
        {
            if (abs(dx) + abs(dy) <= ball_half) 
            {
                int px = ball.pos_x + dx;
                int py = ball.pos_y + dy;
                if (px >= 0 && px < VGA_WIDTH && py >= 0 && py < VGA_HEIGHT) 
                {
                    SetPixel(px, py, ball.color);
                }
            }
        }
    }
}

void clear_ball(unsigned int ball_pos_x, unsigned int ball_pos_y)
{
    int ball_half = 3; // 7x7 diamond
    for (int dy = -ball_half; dy <= ball_half; dy++) 
    {
        for (int dx = -ball_half; dx <= ball_half; dx++) 
        {
            if (abs(dx) + abs(dy) <= ball_half) 
            {
                int px = ball_pos_x + dx;
                int py = ball_pos_y + dy;
                if (px >= 0 && px < VGA_WIDTH && py >= 0 && py < VGA_HEIGHT) 
                {
                    SetPixel(px, py, white);
                }
            }
        }
    }
}

void draw_playing_field() 
{
    int ctr = 0; // counter to keep the colors changing
    for (unsigned int row = 0; row < NROWS; row++) 
    {
        for (unsigned int col = 0; col < NCOLS; col++) 
        {
            if (tiles[row][col] != 0) 
            {
                unsigned int x = VGA_WIDTH - NCOLS * TILE_SIZE + col * TILE_SIZE;
                unsigned int y = row * TILE_SIZE;
                unsigned int color = (ctr % 3 == 0) ? red : (ctr % 3 == 1) ? green : blue;
                draw_block(x, y, TILE_SIZE, TILE_SIZE, color);
                ctr++;
            }
        }
    }
}

void update_game_state()
{
    // TODO: Check: game won? game lost?
    char has_ball_position_changed = 0;

    if (ball.pos_x >= 320 - 3) 
    { // right wall
        currentState = Won;
        return;
    } 
    else if (ball.pos_x <= 7 + 3) 
    { // Bottom (lose)
        currentState = Lost;
        return;
    }

    // TODO: Update balls position and direction

    // Update ball position
    if (frame_counter == FRAME_SKIP)
    {
        ball.pos_x += ball.vx;
        ball.pos_y += ball.vy;

        has_ball_position_changed = 1;
        frame_counter = 0;
    }
    frame_counter++;

    // Update directions bc of walls
    if (ball.pos_y >= VGA_HEIGHT - 3) 
    { // right wall
        ball.vy = -1;
    } 
    else if (ball.pos_y <= 0 + 3) 
    { // Bottom (lose)
        ball.vy = 1;
    }


    // TODO: Hit Check with Blocks
    // HINT: try to only do this check when we potentially have a hit, as it is relatively expensive and can slow down game play a lot

    if(!has_ball_position_changed) //if ball hasn't move we don't check any colisions
    {
        return;
    }

    // leftmost pixel
    int leftmost_ball_x = ball.pos_x - 3;
    

}

void update_bar_state()
{

    // TODO: Read all chars in the UART Buffer and apply the respective bar position updates
    // HINT: w == 77, s == 73
    // HINT Format: 0x00 'Remaining Chars':2 'Ready 0x80':2 'Char 0xXX':2, sample: 0x00018077 (1 remaining character, buffer is ready, current character is 'w')

    int remaining = 0;
    do 
    {
        unsigned long long out = ReadUart();
        if (!(out & 0x8000)) break; // Not valid
        remaining = (out & 0xFF0000) >> 16; // WSPACE
        char c = out & 0xFF;
        if (c == 0x77 && bar.pos_y > 0) bar.pos_y--; // 'w': up
        else if (c == 0x73 && bar.pos_y < VGA_HEIGHT - 45) bar.pos_y++; // 's': down
    } while (remaining > 0);
}

void write(char *str) 
{
    while (*str) 
    {
        WriteUart(*str);
        str++;
    }
}

void play()
{
    //ClearScreen();
    // HINT: This is the main game loop
    unsigned int old_bar_pos_y;
    unsigned int old_ball_pos_x;
    unsigned int old_ball_pos_y;

    while (1)
    {  
        if (currentState != Running) 
        {
            break;
        }
        //ClearScreen();
        //draw_playing_field();
        old_ball_pos_x = ball.pos_x;
        old_ball_pos_y = ball.pos_y;

        
        update_game_state();

        if (old_ball_pos_x != ball.pos_x || old_ball_pos_y != ball.pos_y)
        {
            clear_ball(old_ball_pos_x, old_ball_pos_y);
            draw_ball();
        }

        
        old_bar_pos_y = bar.pos_y;
        update_bar_state();
        if (old_bar_pos_y != bar.pos_y)
        {
            clear_bar(old_bar_pos_y);
            draw_bar(bar.pos_y);
        }
    }
    if (currentState == Won)
    {
        write(won);
    }
    else if (currentState == Lost)
    {
        write(lost);
    }
    else if (currentState == Exit)
    {
        return;
    }
    currentState = Stopped;
}

// It must initialize the game
void reset()
{
    // Hint: This is draining the UART buffer



    ball.pos_x = 20;    // Starting x
    ball.pos_y = 120;   // Starting y (middle)
    ball.vx = 1;        // Move right
    ball.vy = 1;        // No vertical movement
    ball.color = black;

    bar.pos_y = 98; // approx middle 120 - (15 * 3/2)
    bar.vy = 0;

    for (unsigned int row = 0; row < NROWS; row++) 
    {
        for (unsigned int col = 0; col < NCOLS; col++) 
        {
            tiles[row][col] = 1; // All blocks active
        }
    }


    int remaining = 0;
    do
    {
        unsigned long long out = ReadUart();
        if (!(out & 0x8000))
        {
            // not valid - abort reading
            return;
        }
        remaining = (out & 0xFF0000) >> 4;
    } while (remaining > 0);
}

void wait_for_start()
{
    // TODO: Implement waiting behaviour until the user presses either w/s
    while (1) 
    {
        unsigned long long out = ReadUart();
        if (out & 0x8000) 
        {
            char c = out & 0xFF;
            if (c == 0x77 || c == 0x73) 
            {
                currentState = Running;
                break;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    reset();
    ClearScreen();
    draw_ball();
    draw_bar(bar.pos_y);
    draw_playing_field();
    

    // HINT: This loop allows the user to restart the game after loosing/winning the previous game
    while (1)
    {
        wait_for_start();
        play();
        reset();
        if (currentState == Exit)
        {
            break;
        }
    }
    return 0;
}
