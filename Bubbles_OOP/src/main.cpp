#include <windows.h>
#include <windowsx.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <random>
#include <cmath>
#include <queue>
#include <sstream>
#include <iomanip>

const double Pi = 3.14;

using std::vector;
using std::wstring;

const int WinWidth = 900;
const int WinHeight = 720;
const int LeftField = 40;
const int TopField = 80;
const int RightField = 860;
const int BottomField = 650;
const int BubbleRadius = 18;
const int CellWidth = BubbleRadius * 2;
const int RowHeight = 31;
const int Columns = 22;
const int StartRows = 7;
const int EmptyPercent = 24;        // процент пустых мест при генерации карты
const int RepeatPercent = 20;       // шанс повторения предыдущего цвета
const double BubbleSpeed = 25.0;    // скорость полёта шара
const int DownPercent = 0.5;        // коэфф-т скорости падения потолка
const int ShooterX = WinWidth / 2;
const int ShooterY = BottomField;
const wchar_t* ScoreFile = L"bubble_records.txt";

struct Vec2
{
    double x = 0;
    double y = 0;
};

class Graphics
{
private:
    HDC hdc;
public:
    Graphics(HDC dc) : hdc(dc) {}

    void FillRectColor(int x, int y, int w, int h, COLORREF color)
    {
        HBRUSH brush = CreateSolidBrush(color);
        RECT rc{ x, y, x + w, y + h };
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
    }

    void DrawTextCenter(const wstring& text, int x, int y, int w, int h, int size, COLORREF color, bool bold = false)
    {
        HFONT font = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
            RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");
        HFONT oldFont = (HFONT)SelectObject(hdc, font);
        SetTextColor(hdc, color);
        SetBkMode(hdc, TRANSPARENT);
        RECT rc{ x, y, x + w, y + h };
        DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    void DrawTextLeft(const wstring& text, int x, int y, int w, int h, int size, COLORREF color, bool bold = false)
    {
        HFONT font = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
            RUSSIAN_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Tahoma");
        HFONT oldFont = (HFONT)SelectObject(hdc, font);
        SetTextColor(hdc, color);
        SetBkMode(hdc, TRANSPARENT);
        RECT rc{ x, y, x + w, y + h };
        DrawTextW(hdc, text.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    void Line(int x1, int y1, int x2, int y2, int width, COLORREF color)
    {
        HPEN pen = CreatePen(PS_SOLID, width, color);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        MoveToEx(hdc, x1, y1, nullptr);
        LineTo(hdc, x2, y2);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    void Dot(int cx, int cy, int r, COLORREF color)
    {
        HBRUSH brush = CreateSolidBrush(color);
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    void Circle(int cx, int cy, int r, COLORREF fill, COLORREF border)
    {
        HBRUSH brush = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 2, border);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
        HBRUSH shine = CreateSolidBrush(RGB(255, 255, 255));
        SelectObject(hdc, shine);
        SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, cx - r / 2, cy - r / 2, cx - r / 6, cy - r / 6);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(shine);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    void RectBorder(int x, int y, int w, int h, COLORREF color, int width = 2)
    {
        HPEN pen = CreatePen(PS_SOLID, width, color);
        HBRUSH brush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        Rectangle(hdc, x, y, x + w, y + h);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
    }
};

class Button
{
private:
    RECT rect{};
    wstring caption;
public:
    Button() = default;
    Button(int x, int y, int w, int h, const wstring& text) : caption(text)
    {
        rect = { x, y, x + w, y + h };
    }
    bool Contains(int x, int y) const
    {
        return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
    }
    void Draw(Graphics& g) const
    {
        g.FillRectColor(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, RGB(245, 247, 255));
        g.RectBorder(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, RGB(70, 90, 140), 2);
        g.DrawTextCenter(caption, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 22, RGB(30, 45, 80), true);
    }
};

class HighScoreTable
{
private:
    vector<int> scores;
public:
    void Load()
    {
        scores.clear();
        std::wifstream in(ScoreFile);
        int value;
        while (in >> value) scores.push_back(value);
        SortAndTrim();
    }
    void AddWinScore(int score)
    {
        scores.push_back(score);
        SortAndTrim();
        std::wofstream out(ScoreFile);
        for (int s : scores) out << s << L"\n";
    }
    const vector<int>& GetScores() const { return scores; }
private:
    void SortAndTrim()
    {
        std::sort(scores.begin(), scores.end(), std::greater<int>());
        if (scores.size() > 10) scores.resize(10);
    }
};

enum class BubbleColor { Red, Blue, Green, Yellow, Violet };

COLORREF ToColor(BubbleColor color)
{
    switch (color)
    {
    case BubbleColor::Red: return RGB(236, 82, 92);
    case BubbleColor::Blue: return RGB(72, 133, 237);
    case BubbleColor::Green: return RGB(70, 185, 115);
    case BubbleColor::Yellow: return RGB(244, 196, 64);
    case BubbleColor::Violet: return RGB(157, 98, 220);
    }
    return RGB(255, 255, 255);
}

class Bubble
{
private:
    int row = 0;
    int col = 0;
    BubbleColor color = BubbleColor::Red;
public:
    Bubble() = default;
    Bubble(int r, int c, BubbleColor clr) : row(r), col(c), color(clr) {}
    int Row() const { return row; }
    int Col() const { return col; }
    BubbleColor Color() const { return color; }
};

class BubbleGrid
{
private:
    vector<vector<int>> cells;
    vector<Bubble> bubbles;
    double ceilY = TopField;
    std::mt19937 rng;
public:
    BubbleGrid() : rng((unsigned int)GetTickCount64()) { Reset(); }

    void Reset()
    {
        cells.assign(40, vector<int>(Columns, -1));
        bubbles.clear();
        ceilY = TopField;

        vector<BubbleColor> colors = {
            BubbleColor::Red,
            BubbleColor::Blue,
            BubbleColor::Green,
            BubbleColor::Yellow,
            BubbleColor::Violet
        };

        std::uniform_int_distribution<int> colorDist(0, (int)colors.size() - 1);
        std::uniform_int_distribution<int> percentDist(1, 100);

        for (int r = 0; r < StartRows; ++r)
        {
            int maxCols = (r % 2 == 0) ? Columns : Columns - 1;
            bool hasPreviousColor = false;
            BubbleColor previousColor = BubbleColor::Red;

            for (int c = 0; c < maxCols; ++c)
            {
                if (r > 3 && percentDist(rng) <= EmptyPercent)
                    continue;

                BubbleColor currentColor = colors[colorDist(rng)];

                if (hasPreviousColor && percentDist(rng) <= RepeatPercent)
                    currentColor = previousColor;

                AddBubble(r, c, currentColor);
                previousColor = currentColor;
                hasPreviousColor = true;
            }
        }
    }

    void MoveDown(double dy) { ceilY += dy; }
    double CeilY() const { return ceilY; }
    bool Empty() const { return bubbles.empty(); }
    const vector<Bubble>& Bubbles() const { return bubbles; }

    Vec2 CellCenter(int row, int col) const
    {
        double x = LeftField + BubbleRadius + col * CellWidth + (row % 2 ? BubbleRadius : 0);
        double y = ceilY + BubbleRadius + row * RowHeight;
        return { x, y };
    }

    bool ValidCell(int row, int col) const
    {
        if (row < 0 || row >= (int)cells.size()) return false;
        if (col < 0 || col >= Columns) return false;
        if (row % 2 == 1 && col >= Columns - 1) return false;
        return true;
    }

    bool IsOccupied(int row, int col) const
    {
        return ValidCell(row, col) && cells[row][col] != -1;
    }

    int AddBubble(int row, int col, BubbleColor color)
    {
        if (!ValidCell(row, col)) return -1;
        if (cells[row][col] != -1) return cells[row][col];
        int id = (int)bubbles.size();
        bubbles.emplace_back(row, col, color);
        cells[row][col] = id;
        return id;
    }

    void RemoveIds(const vector<int>& ids)
    {
        vector<bool> remove(bubbles.size(), false);
        for (int id : ids) if (id >= 0 && id < (int)remove.size()) remove[id] = true;
        cells.assign(40, vector<int>(Columns, -1));
        vector<Bubble> fresh;
        for (int i = 0; i < (int)bubbles.size(); ++i)
        {
            if (!remove[i])
            {
                int newId = (int)fresh.size();
                fresh.push_back(bubbles[i]);
                cells[fresh.back().Row()][fresh.back().Col()] = newId;
            }
        }
        bubbles.swap(fresh);
    }

    vector<std::pair<int, int>> Neighbors(int row, int col) const
    {
        static const int evenDirs[6][2] = { {-1,-1},{-1,0},{0,-1},{0,1},{1,-1},{1,0} };
        static const int oddDirs[6][2] = { {-1,0},{-1,1},{0,-1},{0,1},{1,0},{1,1} };
        vector<std::pair<int, int>> result;
        const int(*dirs)[2] = (row % 2 == 0) ? evenDirs : oddDirs;
        for (int i = 0; i < 6; ++i)
        {
            int nr = row + dirs[i][0];
            int nc = col + dirs[i][1];
            if (ValidCell(nr, nc)) result.push_back({ nr, nc });
        }
        return result;
    }

    int PopGroupFrom(int startId)
    {
        if (startId < 0 || startId >= (int)bubbles.size()) return 0;
        BubbleColor target = bubbles[startId].Color();
        vector<int> group;
        vector<bool> visited(bubbles.size(), false);
        std::queue<int> q;
        q.push(startId);
        visited[startId] = true;
        while (!q.empty())
        {
            int id = q.front(); q.pop();
            group.push_back(id);
            for (auto [nr, nc] : Neighbors(bubbles[id].Row(), bubbles[id].Col()))
            {
                int nid = cells[nr][nc];
                if (nid != -1 && !visited[nid] && bubbles[nid].Color() == target)
                {
                    visited[nid] = true;
                    q.push(nid);
                }
            }
        }
        if (group.size() >= 3)
        {
            RemoveIds(group);
            RemoveFloating();
            return (int)group.size();
        }
        return 0;
    }

    void RemoveFloating()
    {
        if (bubbles.empty()) return;
        vector<bool> connected(bubbles.size(), false);
        std::queue<int> q;
        for (int c = 0; c < Columns; ++c)
        {
            if (IsOccupied(0, c))
            {
                int id = cells[0][c];
                connected[id] = true;
                q.push(id);
            }
        }
        while (!q.empty())
        {
            int id = q.front(); q.pop();
            for (auto [nr, nc] : Neighbors(bubbles[id].Row(), bubbles[id].Col()))
            {
                int nid = cells[nr][nc];
                if (nid != -1 && !connected[nid])
                {
                    connected[nid] = true;
                    q.push(nid);
                }
            }
        }
        vector<int> floating;
        for (int i = 0; i < (int)bubbles.size(); ++i)
            if (!connected[i]) floating.push_back(i);
        if (!floating.empty()) RemoveIds(floating);
    }

    BubbleColor RandomExistingColor()
    {
        vector<BubbleColor> colors;
        for (const auto& b : bubbles)
        {
            if (std::find(colors.begin(), colors.end(), b.Color()) == colors.end()) colors.push_back(b.Color());
        }
        if (colors.empty()) colors = { BubbleColor::Red, BubbleColor::Blue, BubbleColor::Green };
        std::uniform_int_distribution<int> dist(0, (int)colors.size() - 1);
        return colors[dist(rng)];
    }

    bool HitsExisting(double x, double y) const
    {
        for (const auto& b : bubbles)
        {
            Vec2 p = CellCenter(b.Row(), b.Col());
            double dx = p.x - x, dy = p.y - y;
            if (std::sqrt(dx * dx + dy * dy) <= BubbleRadius * 2 - 2) return true;
        }
        return false;
    }

    int AttachBubble(double x, double y, BubbleColor color)
    {
        int bestR = 0, bestC = 0;
        double bestD = 1e18;
        for (int r = 0; r < 30; ++r)
        {
            int maxCols = (r % 2 == 0) ? Columns : Columns - 1;
            for (int c = 0; c < maxCols; ++c)
            {
                if (IsOccupied(r, c)) continue;
                Vec2 p = CellCenter(r, c);
                double dx = p.x - x, dy = p.y - y;
                double d = dx * dx + dy * dy;
                if (d < bestD)
                {
                    bestD = d;
                    bestR = r;
                    bestC = c;
                }
            }
        }
        return AddBubble(bestR, bestC, color);
    }

    bool TouchesFloor() const
    {
        for (const auto& b : bubbles)
        {
            Vec2 p = CellCenter(b.Row(), b.Col());
            if (p.y + BubbleRadius >= BottomField) return true;
        }
        return false;
    }
};

class Shooter
{
private:
    double angle = -90.0;
public:
    void RotateLeft() { angle -= 1.5; if (angle < -170.0) angle = -170.0; }
    void RotateRight() { angle += 1.5; if (angle > -10.0) angle = -10.0; }
    Vec2 Direction() const
    {
        double rad = angle * Pi / 180.0;
        return { std::cos(rad), std::sin(rad) };
    }
    Vec2 MouthPosition() const
    {
        Vec2 d = Direction();
        return { ShooterX + d.x * 58.0, ShooterY + d.y * 58.0 };
    }
    void Draw(Graphics& g) const
    {
        Vec2 d = Direction();
        int x2 = (int)(ShooterX + d.x * 70);
        int y2 = (int)(ShooterY + d.y * 70);
        g.Line(ShooterX, ShooterY, x2, y2, 12, RGB(110, 90, 70));
        g.Line(ShooterX, ShooterY, x2, y2, 6, RGB(220, 185, 145));
        g.Circle(ShooterX, ShooterY, 22, RGB(225, 225, 230), RGB(90, 90, 110));
    }
};

class FlyingBubble
{
private:
    bool active = false;
    Vec2 pos;
    Vec2 vel;
    BubbleColor color = BubbleColor::Red;
public:
    bool Active() const { return active; }
    BubbleColor Color() const { return color; }
    Vec2 Position() const { return pos; }
    void Launch(BubbleColor clr, Vec2 direction)
    {
        active = true;
        color = clr;
        pos = { ShooterX + direction.x * 58.0, ShooterY + direction.y * 58.0 };
        vel = { direction.x * BubbleSpeed, direction.y * BubbleSpeed };
    }
    void Stop() { active = false; }
    void Update()
    {
        if (!active) return;
        pos.x += vel.x;
        pos.y += vel.y;
        if (pos.x - BubbleRadius <= LeftField)
        {
            pos.x = LeftField + BubbleRadius;
            vel.x *= -1;
        }
        if (pos.x + BubbleRadius >= RightField)
        {
            pos.x = RightField - BubbleRadius;
            vel.x *= -1;
        }
    }
};

enum class ScreenState { Menu, Playing, Records, GameOver };

class Game
{
private:
    HWND hwnd = nullptr;
    ScreenState state = ScreenState::Menu;
    BubbleGrid grid;
    Shooter shooter;
    FlyingBubble flying;
    HighScoreTable records;
    BubbleColor nextColor = BubbleColor::Red;
    int score = 0;
    bool win = false;
    bool leftPressed = false;
    bool rightPressed = false;
    DWORD lastTick = 0;
    Button btnStart, btnRecords, btnExit, btnMenu;
public:
    Game()
    {
        btnStart = Button(350, 285, 200, 50, L"Начать игру");
        btnRecords = Button(350, 360, 200, 50, L"Рекорды");
        btnExit = Button(350, 435, 200, 50, L"Выход");
        btnMenu = Button(350, 600, 200, 45, L"В меню");
    }

    void SetWindow(HWND h) { hwnd = h; records.Load(); NewGame(); state = ScreenState::Menu; }

    void NewGame()
    {
        grid.Reset();
        flying.Stop();
        score = 0;
        win = false;
        nextColor = grid.RandomExistingColor();
        leftPressed = false;
        rightPressed = false;
        lastTick = GetTickCount();
    }

    void Start()
    {
        NewGame();
        state = ScreenState::Playing;
    }

    void OnKeyDown(WPARAM key)
    {
        if (state == ScreenState::Playing)
        {
            if (key == VK_LEFT) leftPressed = true;
            if (key == VK_RIGHT) rightPressed = true;
            if (key == VK_ESCAPE) state = ScreenState::Menu;
            if (key == VK_SPACE && !flying.Active())
            {
                flying.Launch(nextColor, shooter.Direction());
                nextColor = grid.RandomExistingColor();
            }
        }
        else if (key == VK_ESCAPE)
        {
            state = ScreenState::Menu;
        }
    }

    void OnKeyUp(WPARAM key)
    {
        if (key == VK_LEFT) leftPressed = false;
        if (key == VK_RIGHT) rightPressed = false;
    }

    void OnMouseDown(int x, int y)
    {
        if (state == ScreenState::Menu)
        {
            if (btnStart.Contains(x, y)) Start();
            else if (btnRecords.Contains(x, y)) { records.Load(); state = ScreenState::Records; }
            else if (btnExit.Contains(x, y)) PostQuitMessage(0);
        }
        else if (state == ScreenState::Records || state == ScreenState::GameOver)
        {
            if (btnMenu.Contains(x, y)) state = ScreenState::Menu;
        }
    }

    void Update()
    {
        if (state != ScreenState::Playing) return;
        DWORD now = GetTickCount();
        double dt = (now - lastTick) / 1000.0;
        lastTick = now;

        if (leftPressed) shooter.RotateLeft();
        if (rightPressed) shooter.RotateRight();

        double downSpeed = (3.5 + score * 0.025) * DownPercent;
        grid.MoveDown(downSpeed * dt);

        if (flying.Active())
        {
            flying.Update();
            Vec2 p = flying.Position();
            if (p.y - BubbleRadius <= grid.CeilY() || grid.HitsExisting(p.x, p.y))
            {
                int id = grid.AttachBubble(p.x, p.y, flying.Color());
                flying.Stop();
                int popped = grid.PopGroupFrom(id);
                score += popped;
                if (!grid.Empty()) nextColor = grid.RandomExistingColor();
            }
        }

        if (grid.Empty())
        {
            win = true;
            records.Load();
            records.AddWinScore(score);
            state = ScreenState::GameOver;
        }
        else if (grid.TouchesFloor())
        {
            win = false;
            state = ScreenState::GameOver;
        }
    }

    void Draw(HDC hdc)
    {
        Graphics g(hdc);
        g.FillRectColor(0, 0, WinWidth, WinHeight, RGB(232, 238, 252));
        if (state == ScreenState::Menu) DrawMenu(g);
        else if (state == ScreenState::Playing) DrawPlaying(g);
        else if (state == ScreenState::Records) DrawRecords(g);
        else if (state == ScreenState::GameOver) DrawGameOver(g);
    }

private:
    void DrawMenu(Graphics& g)
    {
        g.DrawTextCenter(L"Игра «Пузыри»", 0, 145, WinWidth, 60, 42, RGB(40, 55, 100), true);
        btnStart.Draw(g); btnRecords.Draw(g); btnExit.Draw(g);
        g.DrawTextCenter(L"Управление: ← → — прицеливание, Пробел — выстрел", 0, 545, WinWidth, 35, 20, RGB(60, 70, 100));
    }

    void DrawAimTrajectory(Graphics& g)
    {
        if (flying.Active()) return;
        Vec2 d = shooter.Direction();
        Vec2 p = shooter.MouthPosition();
        for (int i = 0; i < 95; ++i)
        {
            p.x += d.x * 11.0;
            p.y += d.y * 11.0;
            if (p.x - BubbleRadius <= LeftField)
            {
                p.x = LeftField + BubbleRadius;
                d.x *= -1.0;
            }
            if (p.x + BubbleRadius >= RightField)
            {
                p.x = RightField - BubbleRadius;
                d.x *= -1.0;
            }
            if (p.y - BubbleRadius <= grid.CeilY() || grid.HitsExisting(p.x, p.y)) break;
            if (i % 2 == 0) g.Dot((int)p.x, (int)p.y, 3, RGB(120, 135, 165));
        }
    }

    void DrawPlaying(Graphics& g)
    {
        g.FillRectColor(LeftField, TopField - 20, RightField - LeftField, BottomField - TopField + 20, RGB(250, 252, 255));
        g.RectBorder(LeftField, TopField - 20, RightField - LeftField, BottomField - TopField + 20, RGB(110, 130, 170), 3);
        g.FillRectColor(LeftField, (int)grid.CeilY() - 8, RightField - LeftField, 10, RGB(100, 115, 145));
        g.DrawTextLeft(L"Счет: " + std::to_wstring(score), 40, 20, 180, 40, 24, RGB(30, 45, 80), true);
        g.DrawTextCenter(L"← → — прицеливание    Пробел — выстрел", 0, 20, WinWidth, 40, 20, RGB(55, 70, 110), false);

        DrawAimTrajectory(g);

        for (const auto& b : grid.Bubbles())
        {
            Vec2 p = grid.CellCenter(b.Row(), b.Col());
            g.Circle((int)p.x, (int)p.y, BubbleRadius, ToColor(b.Color()), RGB(65, 65, 80));
        }
        if (flying.Active())
        {
            Vec2 p = flying.Position();
            g.Circle((int)p.x, (int)p.y, BubbleRadius, ToColor(flying.Color()), RGB(65, 65, 80));
        }
        shooter.Draw(g);
        if (!flying.Active())
        {
            Vec2 mouth = shooter.MouthPosition();
            g.Circle((int)mouth.x, (int)mouth.y, BubbleRadius, ToColor(nextColor), RGB(60, 60, 70));
        }
        g.FillRectColor(LeftField, BottomField, RightField - LeftField, 42, RGB(95, 80, 70));
    }

    void DrawRecords(Graphics& g)
    {
        g.DrawTextCenter(L"10 лучших результатов", 0, 70, WinWidth, 50, 36, RGB(40, 55, 100), true);
        const auto& s = records.GetScores();
        if (s.empty())
        {
            g.DrawTextCenter(L"Пока нет выигранных игр.", 0, 200, WinWidth, 40, 24, RGB(60, 70, 110));
        }
        else
        {
            for (int i = 0; i < (int)s.size(); ++i)
            {
                std::wstringstream ss;
                ss << std::setw(2) << (i + 1) << L". " << s[i] << L" очков";
                g.DrawTextCenter(ss.str(), 0, 150 + i * 38, WinWidth, 32, 24, RGB(40, 55, 90), i == 0);
            }
        }
        btnMenu.Draw(g);
    }

    void DrawGameOver(Graphics& g)
    {
        g.DrawTextCenter(win ? L"Победа!" : L"Поражение", 0, 150, WinWidth, 60, 44, win ? RGB(50, 150, 90) : RGB(190, 70, 70), true);
        g.DrawTextCenter(L"Ваш счет: " + std::to_wstring(score), 0, 235, WinWidth, 45, 30, RGB(45, 55, 90), true);
        if (win)
            g.DrawTextCenter(L"Результат сохранен в таблицу рекордов.", 0, 300, WinWidth, 40, 22, RGB(55, 70, 110));
        else
            g.DrawTextCenter(L"В рекорды попадают только выигранные игры.", 0, 300, WinWidth, 40, 22, RGB(55, 70, 110));
        btnMenu.Draw(g);
    }
};

Game g_game;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        g_game.SetWindow(hwnd);
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    case WM_TIMER:
        g_game.Update();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_KEYDOWN:
        g_game.OnKeyDown(wParam);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_KEYUP:
        g_game.OnKeyUp(wParam);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
        g_game.OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, WinWidth, WinHeight);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
        g_game.Draw(memDC);
        BitBlt(hdc, 0, 0, WinWidth, WinHeight, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"BubblesOOPWindowClass";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    RECT rc{ 0, 0, WinWidth, WinHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    int windowWidth = rc.right - rc.left;
    int windowHeight = rc.bottom - rc.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Игра Пузырьки",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        windowX, windowY, windowWidth, windowHeight,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}