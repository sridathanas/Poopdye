#include <windows.h>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>



class Coord
{
    public:
        int ROW, COL;

        Coord(int x = -1, int y = -1)
        {
            ROW = x;
            COL = y;
        }


        Coord operator+(const Coord& other) const
        {
            return Coord(ROW + other.ROW, COL + other.COL);
        }


        Coord operator+(int offset)
        {
            return Coord(ROW + offset, COL + offset);
        }


        Coord operator-(const Coord& other) const
        {
            return Coord(ROW - other.ROW, COL - other.COL);
        }


        void operator+=(const Coord& other)
        {
            ROW += other.ROW;
            COL += other.COL;
        }


        void operator+=(int offset)
        {
            ROW += offset;
            COL += offset;
        }


        void operator-=(const Coord& other)
        {
            ROW -= other.ROW;
            COL -= other.COL;
        }


        bool operator==(const Coord& other)
        {
            return ROW == other.ROW && COL == other.COL;
        }


        bool operator!=(const Coord& other)
        {
            return !(*this == other);
        }


        bool isValid() const
        {
            return ROW > 0 && COL > 0;
        }
};


class State
{
    public:
        Coord coord;
        std::string fgColor, bgColor, style;

        State(const Coord& coord = { -1, -1 }, const std::string& fgColor = "", const std::string& bgColor = "", const std::string& style = "")
        {
            this->coord = coord;
            this->fgColor = fgColor;
            this->bgColor = bgColor;
            this->style = style;
        }


        State(const State& state)
        {
            coord = state.coord;
            fgColor = state.fgColor;
            bgColor = state.bgColor;
            style = state.style;
        }


        bool operator==(State other)
        {
            return coord == other.coord && fgColor == other.fgColor && bgColor == other.bgColor;
        }


        bool operator!=(State other)
        {
            return !(*this == other);
        }
};


extern State stateNow;


class OutBuffer
{
    private:
        std::string buffer;
        bool line_start = true;


    public:
        int lpad = 0, bufferSize;
        bool modifyStateNow = true;
        
        OutBuffer(int bufferSize)
        {
            this->bufferSize = bufferSize;
        }


        bool IsLineStart(bool is_it)
        {
            line_start = is_it;
        }


        OutBuffer& operator<<(const std::string& other)
        {
            if (lpad)
            {
                std::string padding = std::string(lpad, ' ');

                if (line_start)
                {
                    buffer += padding;
                    line_start = false;
                    if (modifyStateNow)
                        stateNow.coord.COL += lpad;
                }

                if (other.find('\n') != std::string::npos)
                {
                    for (int i = 0; i < other.length(); i++)
                    {
                        buffer += other[i];
                        if (modifyStateNow)
                            stateNow.coord.COL++;

                        if (other[i] == '\n')
                        {
                            buffer += padding;
                            if (modifyStateNow)
                            {
                                stateNow.coord.ROW++;
                                stateNow.coord.COL = lpad + 1;
                            }
                        }
                    }
                }
                else
                {
                    buffer += other;
                    if (modifyStateNow)
                        stateNow.coord.COL += other.length();
                }
            }
            else
            {
                buffer += other;

                int newlCount = std::count(other.begin(), other.end(), '\n');

                if (modifyStateNow)
                {
                    if (newlCount == 0)
                    {
                        stateNow.coord.COL += other.length();
                    }
                    else
                    {
                        stateNow.coord.ROW += newlCount;
                        stateNow.coord.COL += other.length() - other.rfind('\n') - 1;
                    }
                }
            }

            if (buffer.length() > bufferSize) flush();
            
            return *this;
        }


        OutBuffer& operator<<(int other)
        {
            return *this << std::to_string(other);
        }


        OutBuffer& operator<<(char other)
        {
            return *this << std::string(1, other);
        }


        void flush()
        {
            std::cout << buffer << std::flush;
            buffer = "";
        }


        ~OutBuffer()
        {
            *this << "\033[0m";
            flush();
        }
};


namespace Dir
{
    enum Direction {  
        NONE = 0,
        UP    = 1,
        DOWN  = 2,
        RIGHT = 4,
        LEFT  = 8
    };
}


void CanvasDraw(const std::string& cmd, bool flush = false, bool is_line_start = false);
std::string Fmt(char *cmd, ...);
void MicroSleep(long long microseconds);

extern std::map<std::string, char> cursorCtrls;
extern std::map<std::string, std::string> colors;
extern std::map<std::string, std::string> styles;
extern OutBuffer outBuff;
extern int wait;


class Screen
{
    public:
        static std::vector<State> LIFOSaves;
        static std::map<std::string, State> mapSaves;


    
        static const int WIDTH = 188;
        static const int HEIGHT = 50;
        static std::string SCREEN_BG;

    
        static void SetColor(const std::string& fgColor, const std::string& bgColor = "")
        {
            outBuff.modifyStateNow = false;

            if (fgColor != "" && fgColor != stateNow.fgColor)
            {
                outBuff << "\033[38;2;" << colors[fgColor] << "m";
                stateNow.fgColor = fgColor;
            }
            
            if (bgColor != "" && bgColor != stateNow.bgColor)
            {
                outBuff << "\033[48;2;" << colors[bgColor] << "m";
                stateNow.bgColor = bgColor;
            }

            outBuff.modifyStateNow = true;
        }


        static void SetStyle(const std::string& style)
        {
            outBuff.modifyStateNow = false;

            if (style != "")
            {
                outBuff << "\033[" << styles[style];

                if (style != "HIDE" && style != "UHIDE")
                    stateNow.style = style;
            }

            outBuff.modifyStateNow = true;
        }


        static void MoveCursor(int where, int n = 1)
        {
            outBuff.modifyStateNow = false;

            outBuff << "\033[" << n;

            switch (where)
            {
                case Dir::UP:
                    outBuff << cursorCtrls["UP"];
                    stateNow.coord.ROW -= n;
                    break;
                
                case Dir::DOWN:
                    outBuff << cursorCtrls["DOWN"];
                    stateNow.coord.ROW += n;
                    break;
                
                case Dir::RIGHT:
                    outBuff << cursorCtrls["RIGHT"];
                    stateNow.coord.COL += n;
                    break;
                
                case Dir::LEFT:
                    outBuff << cursorCtrls["LEFT"];
                    stateNow.coord.COL -= n;
                    break;

                case Dir::RIGHT + Dir::UP:
                    MoveCursor(Dir::RIGHT);
                    MoveCursor(Dir::UP);
                    break;

                case Dir::RIGHT + Dir::DOWN:
                    MoveCursor(Dir::RIGHT);
                    MoveCursor(Dir::DOWN);
                    break;

                case Dir::LEFT + Dir::UP:
                    MoveCursor(Dir::LEFT);
                    MoveCursor(Dir::UP);
                    break;

                case Dir::LEFT + Dir::DOWN:
                    MoveCursor(Dir::LEFT);
                    MoveCursor(Dir::DOWN);
                    break;
            }

            outBuff.modifyStateNow = true;
        }


        static Coord GetCurs(bool actual = false)
        {
            if (!actual)
                return stateNow.coord;

            HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi = { };
            GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);
            COORD res = csbi.dwCursorPosition;

            return Coord(res.X + 1, res.Y + 1);
        }


        static void AtCoord(const Coord& dest = { 1, 1 })
        {
            outBuff.modifyStateNow = false;

            outBuff << "\033[" << dest.ROW << ";" << dest.COL << "H";
            stateNow.coord = dest;

            outBuff.modifyStateNow = true;
        }


        static void Paint(const std::string& color)
        {
            Screen::AtCoord();
            Screen::SetColor("", color);
            Screen::SCREEN_BG = color;
            
            for (int i = 0; i < HEIGHT; i++)
            {
                outBuff << std::string(WIDTH, ' ');

                Screen::MoveCursor(Dir::DOWN);
                Screen::MoveCursor(Dir::LEFT, WIDTH);
            }

            Screen::AtCoord();
            outBuff.flush();
        }


        static void Puts(const Coord& point = { -1, -1 }, const std::string& string = "*", bool getCoord = true, bool getBg = true, bool getFg = false, bool getStyle = false)
        {
            bool dontOptimize = !(getCoord || getBg || getFg);

            if (dontOptimize)
                Screen::SaveState();
                
            if (point.isValid())
                Screen::AtCoord(point);

            outBuff << string;

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        static void SaveState()
        {
            LIFOSaves.push_back(stateNow);
        }


        static void SaveState(const std::string& tag, Coord coord = { -1, -1 }, const std::string& fgColor = "", const std::string& bgColor = "", const std::string& style = "")
        {
            mapSaves[tag] = State((coord.ROW == -1) ? GetCurs() : coord, fgColor, bgColor, style);
        }


        static void RetrieveState(bool getCoord = true, bool getBg = true, bool getFg = false, bool getStyle = false)
        {
            if (!LIFOSaves.empty())
            {
                State state = LIFOSaves.back();
                
                if (getCoord)
                    Screen::AtCoord(state.coord);
                if (getStyle)
                    Screen::SetStyle(state.style);

                if (getFg && getBg)
                    Screen::SetColor(state.fgColor, state.bgColor);
                else if (getBg)
                    Screen::SetColor("", state.bgColor);
                else if (getFg)
                    Screen::SetColor(state.fgColor, "");
            }
        }


        static void RetrieveState(const std::string& tag, bool getCoord = true, bool getBg = true, bool getFg = false, bool getStyle = false)
        {
            if (mapSaves.count(tag))
            {
                State state = mapSaves[tag];
                
                if (getCoord)
                    Screen::AtCoord(state.coord);
                if (getStyle)
                    Screen::SetStyle(state.style);

                if (getFg && getBg)
                    Screen::SetColor(state.fgColor, state.bgColor);
                else if (getBg)
                    Screen::SetColor("", state.bgColor);
                else if (getFg)
                    Screen::SetColor(state.fgColor, "");
            }
        }


        static void UpdateState(const State& state)
        {
            Screen::AtCoord(state.coord);
            Screen::SetColor(state.fgColor, state.bgColor);
            Screen::SetStyle(state.style);
        }
};


#define ms *1000   //milliseconds to microseconds (for MicroSleep)
#define s  *1000000
#define M  *60000000

extern void Pause(int x = 1 s);


class Figure
{
    public:
        State thisState;


        Figure(const Coord& vertex = { -1, -1 }, const std::string& fgColor = "", const std::string& bgColor = "")
        {
            Coord nowCursor = Screen::GetCurs();

            thisState.coord.ROW = (vertex.ROW != -1) ? vertex.ROW : nowCursor.ROW;
            thisState.coord.COL = (vertex.COL != -1) ? vertex.COL : nowCursor.COL;
            thisState.fgColor = (fgColor == "") ? "WHITE" : fgColor;
            thisState.bgColor = (bgColor == "") ? Screen::SCREEN_BG : bgColor;
        }


        static void Join(const Coord& point1, const Coord& point2, char character = '*')
        {
            Coord delta = point2 - point1;

            Screen::SaveState();
            Screen::AtCoord(point1);

            outBuff << character;

            if (delta.ROW == 0)
            {
                for (int n = delta.COL; n--; )
                    outBuff << character;
            }
            else if (delta.COL == 0)
            {
                for (int n = delta.ROW; n--; )
                {
                    Screen::MoveCursor(Dir::DOWN);
                    Screen::MoveCursor(Dir::LEFT);
                    outBuff << character;
                }
            }
            else if (delta.ROW == delta.COL)
            {
                for (int n = delta.ROW; n--; )
                {
                    Screen::MoveCursor(Dir::DOWN);
                    Screen::MoveCursor(Dir::RIGHT);
                    outBuff << character;
                }
            }

            Screen::RetrieveState();
        }


        virtual void Clear(bool getCoord = true, bool getBg = false, bool getFg = false) { };


        virtual void Draw(bool getCoord = true, bool getBg = false, bool getFg = false) { };


        virtual void MoveTo(const Coord& where, bool getCoord = true, bool getBg = false, bool getFg = false) { };


        virtual void MoveBy(const Coord& diff, bool getCoord = true, bool getBg = false, bool getFg = false)
        {
            MoveTo(thisState.coord + diff, getCoord, getBg, getFg);
        }


        virtual Coord Next(bool reset = false) { }


        virtual void ChangeColor(const std::string& fgColor = "", const std::string& bgColor = "", bool optimize = false)
        {
            if (fgColor != "")
                thisState.fgColor = fgColor;
            if (bgColor != "")
                thisState.bgColor = bgColor;
            
            if (!optimize)
                Screen::SaveState();

            Draw();

            if (!optimize)
                Screen::RetrieveState();
        }


        virtual int Collides(const Coord& QPoint, const std::string& lineType, int dist = 1)  //lineType = horz, vert
        {
            int res = 0;
            bool notFound[] = { true, true, true, true };  //UP, DOWN, RIGHT, LEFT

            if (lineType == "horz" || lineType == "all")
            {
                Coord thisPt;

                while ( (thisPt = Next()).isValid() )
                {
                    int diff = thisPt.ROW - QPoint.ROW;

                    if (abs(diff) <= dist)
                    {
                        if (lineType == "all")
                        {
                            if (diff < 0 && notFound[0])
                            {
                                res += Dir::UP;
                                notFound[0] = false;
                            }
                            else if (diff > 0 && notFound[1])
                            {
                                res += Dir::DOWN;
                                notFound[1] = false;
                            }
                        }
                        else
                            return (diff > 0) ? Dir::DOWN : Dir::UP;
                    }
                }
            }
            
            if (lineType == "vert" || lineType == "all")
            {
                Coord thisPt;
                
                while ( (thisPt = Next()).isValid() )
                {
                    int diff = thisPt.COL - QPoint.COL;

                    if (abs(diff) <= dist)
                    {
                        if (lineType == "all")
                        {
                            if (diff > 0 && notFound[2])
                            {
                                res += Dir::RIGHT;
                                notFound[2] = false;
                            }
                            else if (diff < 0 && notFound[3])
                            {
                                res += Dir::LEFT;
                                notFound[3] = false;
                            }
                        }
                        else
                            return (diff > 0) ? Dir::RIGHT : Dir::LEFT;
                    }
                }
            }

            return res;
        }


        virtual int Collides(Figure& other, int dist = 1)
        {
            int res = 0;
            bool notFound[] = { true, true, true, true };  //UP, DOWN, RIGHT, LEFT
            Coord thisPt, otherPt;

            while ( (thisPt = this->Next()).isValid() )
            {
                while ( (otherPt = other.Next()).isValid() )
                {
                    if (thisPt.COL == otherPt.COL)
                    {
                        int diff = thisPt.ROW - otherPt.ROW;

                        if (abs(diff) <= dist)
                        {
                            if (diff < 0 && notFound[0])
                            {
                                res += Dir::UP;
                                notFound[0] = false;
                            }
                            else if (diff > 0 && notFound[1])
                            {
                                res += Dir::DOWN;
                                notFound[1] = false;
                            }
                        }
                    }

                    if (thisPt.ROW == otherPt.ROW)
                    {
                        int diff = thisPt.COL - otherPt.COL;

                        if (abs(diff) <= dist)
                        {
                            if (diff < 0 && notFound[0])
                            {
                                res += Dir::UP;
                                notFound[0] = false;
                            }
                            else if (diff > 0 && notFound[1])
                            {
                                res += Dir::DOWN;
                                notFound[1] = false;
                            }
                        }
                    }
                }
            }

            return res;
        }
};



class Point : public Figure
{
    private:
        char pointChar;
        bool iterEnd = false;

    public:
        Point(const Coord& vert = { -1, -1 }, const std::string& fgColor = "", const std::string& bgColor = "", char pointChar = '*') : Figure(vert, fgColor, bgColor)
        {
            this->pointChar = pointChar;
            Draw();
        }


        void Clear(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();

            Screen::SetColor("", Screen::SCREEN_BG);
            Screen::AtCoord(thisState.coord);
            outBuff << " ";

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void Draw(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;
            
            if (dontOptimize)
                Screen::SaveState();
            
            Screen::UpdateState(thisState);
            outBuff << pointChar;

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void MoveTo(const Coord& where, bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;
            if (dontOptimize)
                Screen::SaveState();
            
            Clear(false);
            thisState.coord = where;
            Draw(false);
            
            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void ChangeChar(char pointChar, bool optimize = false)
        {
            this->pointChar = pointChar;

            if (!optimize)
                Screen::SaveState();

            Screen::UpdateState(thisState);
            outBuff << pointChar;
            
            if (!optimize)
                Screen::RetrieveState();
        }


        Coord Next(bool reset = false) override
        {
            if (reset || iterEnd)
            {
                iterEnd = false;
                return Coord(-1, -1);
            }

            iterEnd = true;
            return thisState.coord;
        }
};



class HorzLine : public Figure
{
    private:
        std::string pattern;
        int length, iterIndex = 0;

    public:
        HorzLine(const std::string& pattern, int length, Coord vertex = { -1, -1 }, const std::string& fgColor = "", const std::string& bgColor = "") : Figure(vertex, fgColor, bgColor)
        {
            this->pattern = pattern;
            this->length = length;
            Draw();
        }


        void Clear(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();

            Screen::SetColor("", Screen::SCREEN_BG);
            Screen::AtCoord(thisState.coord);
            outBuff << std::string(length, ' ');

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void Draw(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();
            
            int pat_len = pattern.length();

            Screen::UpdateState(thisState);

            for (int i = 0; i < length; i++)
                outBuff << pattern[i % pat_len];

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void MoveTo(const Coord& where, bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();
            
            Clear(false);
            thisState.coord = where;
            Draw(false);

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void ChangePattern(const std::string& pattern, bool getCoord = true, bool getBg = false, bool getFg = false)
        {
            this->pattern = pattern;
            Draw(getCoord, getBg, getFg);
        }


        Coord Next(bool reset = false) override
        {
            if (reset || iterIndex == length)
            {
                iterIndex = 0;
                return Coord(-1, -1);
            }

            return thisState.coord + Coord(0, iterIndex++);
        }


        ~HorzLine()
        {
            Clear();
        }
};



class VertLine : public Figure
{
    private:
        std::string pattern;
        int length, iterIndex = 0;

    public:
        VertLine(const std::string& pattern, int length, Coord vertex = { -1, -1 }, const std::string& fgColor = "", const std::string& bgColor = "") : Figure(vertex, fgColor, bgColor)
        {
            this->pattern = pattern;
            this->length = length;
            Draw();
        }


        void Clear(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();
            
            Screen::SetColor("", Screen::SCREEN_BG);
            Screen::AtCoord(thisState.coord);

            for (int n = length; n--; )
            {
                outBuff << " ";
                Screen::MoveCursor(Dir::DOWN);
                Screen::MoveCursor(Dir::LEFT);
            }

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void Draw(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {            
            int pat_len = pattern.length();

            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();
            
            
            Screen::UpdateState(thisState);
                
            for (int i = 0; i < length; i++)
            {
                outBuff << pattern[i % pat_len];
                Screen::MoveCursor(Dir::DOWN);
                Screen::MoveCursor(Dir::LEFT);
            }

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void MoveTo(const Coord& where, bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();

            Clear(false);
            thisState.coord = where;
            Draw(false);

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void ChangePattern(const std::string& pattern, bool getCoord = true, bool getBg = false, bool getFg = false)
        {
            this->pattern = pattern;
            Draw(getCoord, getBg, getFg);
        }


        Coord Next(bool reset = false) override
        {
            if (reset || iterIndex == length)
            {
                iterIndex = 0;
                return Coord(-1, -1);
            }

            return thisState.coord + Coord(iterIndex++, 0);
        }


        ~VertLine()
        {
            Clear();
        }
};



class Block : public Figure
{
    private:
        std::string pattern;
        int iterIndex = 0;

    public:
        int width, height;

        
        Block(const std::string& pattern, int width, int height = -1, Coord vertex = { -1, -1 }, const std::string& bgColor = "", const std::string& fgColor = "") : Figure(vertex, fgColor, bgColor)
        {
            this->pattern = pattern;
            this->width = width;
            this->height = (height == -1) ? width : height;
            Draw();
        }


        void Clear(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();
            
            Screen::SetColor("", Screen::SCREEN_BG);
            Screen::AtCoord(thisState.coord);

            for (int n = height; n--; )
            {
                outBuff << std::string(width, ' ');
                Screen::MoveCursor(Dir::DOWN);
                Screen::MoveCursor(Dir::LEFT, width);
            }

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void Draw(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {            
            int pat_len = pattern.length();

            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();
            

            Screen::UpdateState(thisState);

            for (int n = height; n--; )
            {
                for (int i = 0; i < width; i++)
                    outBuff << pattern[i % pat_len];

                Screen::MoveCursor(Dir::DOWN);
                Screen::MoveCursor(Dir::LEFT, width);
            }

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void MoveTo(const Coord& where, bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();

            Clear(false);
            thisState.coord = where;
            Draw(false);

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        // RETURNS FALSE IF CAN'T EXPAND/CONTRACT FURTHER
        bool Reframe(const Coord& delta, bool getCoord = true, bool getBg = false, bool getFg = false)
        {
            int oldWidth = width;
            height += delta.ROW;
            width  += delta.COL;

            if (height <= 0 || width <= 0 || height > Screen::HEIGHT || width > Screen::WIDTH)
                return false;

            bool notOptimize = !(getCoord || getBg || getFg);

            if (notOptimize)
                Screen::SaveState();
            
            int pat_len = pattern.length();
            int minWidth  = std::min(width,  width  - delta.COL);
            int minHeight = std::min(height, height - delta.ROW);


            if (delta.COL > 0)
            {
                Screen::SetColor(thisState.fgColor, thisState.bgColor);

                for (int n = 0; n < minHeight; n++ )
                {
                    Screen::AtCoord({ thisState.coord.ROW + n, thisState.coord.COL + minWidth });
                    for (int i = 0; i < delta.COL; i++)
                        outBuff << pattern[i % pat_len];
                }
            }
            else
            {
                Screen::SetColor("", Screen::SCREEN_BG);

                for (int n = 0; n < minHeight; n++ )
                {
                    Screen::AtCoord({ thisState.coord.ROW + n, thisState.coord.COL + minWidth });
                    outBuff << std::string(-delta.COL, ' ');
                }
            }
    
            if (delta.ROW > 0)
            {
                Screen::SetColor(thisState.fgColor, thisState.bgColor);

                for (int n = minHeight; n < height; n++)
                {
                    Screen::AtCoord({ thisState.coord.ROW + n, thisState.coord.COL });
                    for (int i = 0; i < width; i++)
                        outBuff << pattern[i % pat_len];
                }
            }
            else
            {
                Screen::SetColor("", Screen::SCREEN_BG);

                for (int n = 0; n < -delta.ROW; n++)
                {
                    Screen::AtCoord({ thisState.coord.ROW + minHeight + n, thisState.coord.COL });
                    outBuff << std::string(oldWidth, ' ');
                }
            }

            if (notOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);

            return true;
        }


        void ChangePattern(const std::string& pattern, bool getCoord = true, bool getBg = false, bool getFg = false)
        {
            this->pattern = pattern;
            Draw(getCoord, getBg, getFg);
        }


        Coord Next(bool reset = false) override
        {
            if (reset || iterIndex == 2 * (width + height) - 4)
            {
                iterIndex = 0;
                return Coord(-1, -1);
            }
            
            Coord offset;

            if (iterIndex < width)
                offset = { 0, iterIndex };

            else if (iterIndex < width + height - 1)
                offset = { iterIndex - width + 1, width - 1 };

            else if (iterIndex < 2 * (width - 1) + height)
                offset = { height - 1, 2 * width + height - iterIndex - 3 };

            else
                offset = { 2 * (width + height) - iterIndex - 4, 0 };


            iterIndex++;
            return Coord(thisState.coord + offset);
        }


        ~Block()
        {
            Clear();
        }
};



class Group : public Figure
{
    private:
        std::vector<Figure*> elms;
        int anchor = -1, iterIndex = 0;
    
    public:
        Group(std::vector<Figure*> vec, int anchor = -1)
        {
            elms = vec;
            this->anchor = anchor;
        }


        Group(Figure *arr[], int length, int anchor = -1)
        {
            for (int i = 0; i < length; i++)
                elms.push_back(arr[i]);
            
            this->anchor = anchor;
        }


        std::vector<Figure*> GetElements()
        {
            return elms;
        }


        void Clear(bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();

            for (Figure* elm : elms)
                elm->Clear(false);

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void MoveBy(const Coord& diff, bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            bool dontOptimize = getCoord || getBg || getFg;

            if (dontOptimize)
                Screen::SaveState();

            Clear(false);

            for (Figure* elm : elms)
            {
                elm->thisState.coord += diff;
                elm->Draw(false);
            }

            if (dontOptimize)
                Screen::RetrieveState(getCoord, getBg, getFg);
        }


        void MoveTo(const Coord& dest, bool getCoord = true, bool getBg = false, bool getFg = false) override
        {
            MoveBy(dest - elms[anchor]->thisState.coord, getCoord, getBg, getFg);
        }


        void ChangeColor(const std::string& fgColor = "", const std::string& bgColor = "", bool optimize = false) override
        {
            if (!optimize)
                Screen::SaveState();

            for (Figure* elm : elms)
                elm->ChangeColor(fgColor, bgColor, false);

            if (!optimize)
                Screen::RetrieveState();
        }


        Coord Next(bool reset = false) override
        {
            while (true)
            {
                if (reset || iterIndex == elms.size())
                {
                    if (iterIndex)
                    {
                        elms[reset ? iterIndex : iterIndex - 1]->Next(true);
                        iterIndex = 0;
                    }

                    return Coord(-1, -1);
                }

                Coord ptNow = elms[iterIndex]->Next();

                if (!ptNow.isValid())
                {
                    iterIndex++;
                    continue;
                }

                return ptNow;
            }
        }
};
