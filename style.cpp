#include <stdarg.h>
#include <windows.h>
#include "C:\Users\User\Desktop\VSCode\Cpp\Modules\style.h"


std::map<std::string, char> cursorCtrls = {
    { "UP", 'A' },
    { "DOWN", 'B' },
    { "RIGHT", 'C' },
    { "LEFT", 'D' }
};


std::map<std::string, std::string> colors = {
    { "RED", "255;0;0" },
    { "GREEN", "0;255;0" },
    { "BLUE", "0;0;255" },
    { "BLACK", "0;0;0" },
    { "WHITE", "255;255;255" },
    { "YELLOW", "255;255;0" },
    { "PURPLE", "205;65;225" },
    { "PINK", "255;0;125" },
    { "CYAN", "0;255;255" },
    { "ROSE", "150;0;75" },
    { "EMERALD", "0;200;105" },
    { "OCHRE", "204;119;34" },
    { "POOP", "101;67;33" },
    { "SKY", "50;155;255" },
    { "LAVENDER", "180;100;255" },
    { "BATHROOM", "100;100;255" },
    { "GRAY", "32;32;32" },
    { "CONSOLE", "12;12;12" }
};


std::map<std::string, std::string> styles = {
    { "HIDE", "?25l" },
    { "UHIDE", "?25h" },
    { "BOLD", "1m" },
    { "UBOLD", "22m" },
    { "ITALIC", "3m" },
    { "UITALIC", "23m" },
    { "LINE", "4m" },
    { "ULINE", "24m" },
    { "STRIKE", "9m" },
    { "USTRIKE", "29m" }
};


std::vector<State> Screen::LIFOSaves;
std::map<std::string, State> Screen::mapSaves;
std::string Screen::SCREEN_BG = "CONSOLE";


int wait = 0;
OutBuffer outBuff(120);
State stateNow(Coord(1, 1), "WHITE", Screen::SCREEN_BG);


void Pause(int x)
{
    outBuff.flush();
    MicroSleep(x);
}


void MicroSleep(long long microseconds)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER start, end;
    QueryPerformanceFrequency(&frequency);
    double ticksPerMicrosecond = static_cast<double>(frequency.QuadPart) / 1000000.0;
    QueryPerformanceCounter(&start);

    while (true) {
        QueryPerformanceCounter(&end);
        double elapsedMicroseconds = static_cast<double>(end.QuadPart - start.QuadPart) / ticksPerMicrosecond;

        if (elapsedMicroseconds >= microseconds) {
            break;
        }
    }
}


//bring in a token system instead
void CanvasDraw(const std::string& cmd, bool flush, bool is_line_start)
{
    int len = cmd.length();

    outBuff.IsLineStart(is_line_start);

    for (int i = 0; i < len; )
    {
        int n = 0;
        char c = cmd[i];
        
        if (c == ' ')
        {
            i++;
            continue;
        }
        if (c == '\'')
        {
            std::string buff = "";

            while (cmd[++i] != '\'')
                buff += cmd[i];

            if (i < len && cmd[++i] == '_')
                while (std::isdigit(cmd[++i]))
                    n = n * 10 + cmd[i] - 48;
            else
                n = 1;
            
            while (n--)
                outBuff << buff;
        }
        else if (c == 't')
        {
            while (++i < len && std::isdigit(cmd[i]))
                n = n * 10 + cmd[i] - 48;

            if (flush)
                outBuff.flush();
            
            MicroSleep(n * 1000);
        }
        else if (isupper(c))
        {
            std::string color = std::string(1, c);

            while (++i < len && isupper(cmd[i]))
                color += cmd[i];


            std::string fbc = "38";
            if (i < len && cmd[i] == '*')
            {
                fbc = "48";
                i++;
            }
            outBuff << "\033[" << fbc << ";2;" << colors.find(color)->second << "m";
        }
        else if (c == '-')   //reset styles
        {
            outBuff << "\033[0m";
            i++;
        }
        else if (c == 'H')
        {
            outBuff << "\033[H";
            i++;
        }
        else if (c == 'b')
        {
            while (++i < len && std::isdigit(cmd[i]))
                n = n * 10 + cmd[i] - 48;

            outBuff << "\033[" << n << "D" << std::string(n, ' ');
        }
        else if (c == '(')
        {
            int coord[] = { 0, 0 };
            for (int j = 0; j < 2; j++)
            {
                while (++i < len && std::isdigit(cmd[i]))
                    coord[j] = coord[j] * 10 + cmd[i] - 48;
            }

            outBuff << "\033[" << coord[0] << ";" << coord[1] << "H";
            i++;
        }
        else if (c == '[')
        {
            std::string spec = "";

            while (cmd[++i] != ']')
                spec += cmd[i];

            outBuff << "\033[" << styles[spec];
            i++;
        }
        else
        {
            if (cmd[i+1] == '*')
            {
                n = 300;
                i++;
            }
            else
            {
                while (++i < len && std::isdigit(cmd[i]))
                    n = n * 10 + cmd[i] - 48;
            }
            
            if (!n) n = 1;

            int direction = c == 'u' ? Dir::UP :
                            c == 'd' ? Dir::DOWN :
                            c == 'r' ? Dir::RIGHT :
                            c == 'l' ? Dir::LEFT :
                            Dir::NONE;

            Screen::MoveCursor(direction, n);
        }


        if (flush)
            outBuff.flush();
        if (wait) MicroSleep(wait * 1000);
    }
}


std::string Fmt(char *cmd, ...)
{
    std::string str = "";

    va_list args;
    va_start(args, cmd);

    for (int i = 0; cmd[i]; i++)
    {
        char c = cmd[i];

        if (c == '%')
        {
            c = cmd[++i];

            switch (c)
            {
                case 'd': {
                    str += std::to_string(va_arg(args, int));
                    break;
                }

                case 'c': {
                    str += c;
                    break;
                }

                case 's': {
                    char *ptr = va_arg(args, char *);

                    for (int j = 0; ptr[j]; j++)
                        str += ptr[j];
                }
            }
        }
        else
            str += c;
    }

    return str;
}


void disp_coord(Coord coord)
{
    outBuff.flush();
    std::cout << coord.ROW << ", " << coord.COL << std::endl;
}