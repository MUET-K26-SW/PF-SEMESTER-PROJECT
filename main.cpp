#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>

using namespace std;

// =====================================================================
//  THEME
// =====================================================================
const COLORREF COL_BG       = RGB(241, 245, 249);
const COLORREF COL_HEAD_A   = RGB(15, 23, 42);     // gradient start (navy)
const COLORREF COL_HEAD_B   = RGB(67, 56, 202);    // gradient end (indigo)
const COLORREF COL_ACCENT   = RGB(99, 102, 241);
const COLORREF COL_CARD     = RGB(255, 255, 255);
const COLORREF COL_SHADOW   = RGB(218, 225, 235);
const COLORREF COL_BORDER   = RGB(226, 232, 240);
const COLORREF COL_INPUT    = RGB(248, 250, 252);
const COLORREF COL_LABEL    = RGB(100, 116, 139);
const COLORREF COL_TEXT     = RGB(15, 23, 42);
const COLORREF COL_SUBTITLE = RGB(165, 180, 252);
const COLORREF COL_OK       = RGB(22, 163, 74);
const COLORREF COL_ERR      = RGB(220, 38, 38);
const COLORREF COL_STATUSBG = RGB(226, 232, 240);

// Button ids
enum { ID_PREV = 101, ID_NEXT, ID_FIND, ID_ADD, ID_UPDATE, ID_DELETE, ID_CLEAR };

const COLORREF BTN_COLOR[7] = {
    RGB(71, 85, 105),   // Prev
    RGB(71, 85, 105),   // Next
    RGB(99, 102, 241),  // Find
    RGB(22, 163, 74),   // Add
    RGB(217, 119, 6),   // Update
    RGB(220, 38, 38),   // Delete
    RGB(100, 116, 139)  // Clear
};

// Layout constants
const int CLIENT_W = 620;
const int CLIENT_H = 426;
const int HEADER_H = 80;
const int COL_L = 36;
const int COL_R = 322;
const int FIELD_W = 262;
const int FIELD_H = 34;

HBRUSH hBgBrush, hCardBrush, hInputBrush;
HFONT hFontInput, hFontLabel, hFontTitle, hFontSub, hFontButton, hFontStatus, hFontBadge;

HWND g_hwnd;
HWND hName, hCaste, hFather, hRoll, hGPA;
int hotButton = 0;

string statusMsg = "Ready";
COLORREF statusColor = RGB(71, 85, 105);
int recordCount = 0;

const char* FILE_NAME = "Students.txt";

struct Student
{
    string roll, name, caste, father, gpa;
};

// =====================================================================
//  COLOR HELPERS
// =====================================================================
COLORREF lerpColor(COLORREF a, COLORREF b, int t, int maxT)
{
    int r = GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t / maxT;
    int g = GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t / maxT;
    int bl = GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t / maxT;
    return RGB(r, g, bl);
}

COLORREF shade(COLORREF c, int amount)   // + lighter, - darker
{
    int r = GetRValue(c) + amount;
    int g = GetGValue(c) + amount;
    int b = GetBValue(c) + amount;
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return RGB(r, g, b);
}

// =====================================================================
//  CONTROL HELPERS
// =====================================================================
string getText(HWND control)
{
    char buffer[256] = "";
    GetWindowTextA(control, buffer, sizeof(buffer));
    return string(buffer);
}

void setText(HWND control, const string& value)
{
    SetWindowTextA(control, value.c_str());
}

void clearFields()
{
    setText(hName, "");
    setText(hCaste, "");
    setText(hFather, "");
    setText(hRoll, "");
    setText(hGPA, "");
}

void setStatus(const string& msg, COLORREF color)
{
    statusMsg = msg;
    statusColor = color;
    RECT r = { 0, CLIENT_H - 28, CLIENT_W, CLIENT_H };
    InvalidateRect(g_hwnd, &r, FALSE);
}

// =====================================================================
//  FILE STORAGE
// =====================================================================
vector<Student> loadStudents()
{
    vector<Student> students;
    ifstream file(FILE_NAME);
    string line;

    while (getline(file, line))
    {
        stringstream ss(line);
        Student s;

        if (getline(ss, s.roll, ',') &&
            getline(ss, s.name, ',') &&
            getline(ss, s.caste, ',') &&
            getline(ss, s.father, ',') &&
            getline(ss, s.gpa, ','))
        {
            students.push_back(s);
        }
    }

    return students;
}

void refreshCount()
{
    recordCount = (int)loadStudents().size();
    RECT r = { 0, CLIENT_H - 28, CLIENT_W, CLIENT_H };
    InvalidateRect(g_hwnd, &r, FALSE);
}

bool saveStudents(const vector<Student>& students)
{
    ofstream file(FILE_NAME, ios::trunc);

    if (!file)
        return false;

    for (size_t i = 0; i < students.size(); i++)
    {
        file << students[i].roll << ","
             << students[i].name << ","
             << students[i].caste << ","
             << students[i].father << ","
             << students[i].gpa << "\n";
    }

    file.close();
    return !file.fail();
}

void showStudent(const Student& s)
{
    setText(hRoll, s.roll);
    setText(hName, s.name);
    setText(hCaste, s.caste);
    setText(hFather, s.father);
    setText(hGPA, s.gpa);
}

// =====================================================================
//  VALIDATION
// =====================================================================
bool validFields(HWND hwnd)
{
    if (getText(hRoll).empty() ||
        getText(hName).empty() ||
        getText(hCaste).empty() ||
        getText(hFather).empty() ||
        getText(hGPA).empty())
    {
        MessageBoxA(hwnd, "Please fill all fields!",
                    "Warning", MB_OK | MB_ICONWARNING);
        return false;
    }

    // The file is comma-separated, so commas inside values would corrupt it.
    HWND all[] = { hRoll, hName, hCaste, hFather, hGPA };
    for (int i = 0; i < 5; i++)
    {
        if (getText(all[i]).find(',') != string::npos)
        {
            MessageBoxA(hwnd, "Commas (,) are not allowed in fields!",
                        "Warning", MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    string gpa = getText(hGPA);
    char* end = NULL;
    double g = strtod(gpa.c_str(), &end);
    if (*end != '\0' || g < 0.0 || g > 4.0)
    {
        MessageBoxA(hwnd, "GPA must be a number between 0 and 4.0!",
                    "Warning", MB_OK | MB_ICONWARNING);
        return false;
    }

    return true;
}

// =====================================================================
//  ACTIONS
// =====================================================================
void addStudent(HWND hwnd)
{
    if (!validFields(hwnd))
        return;

    vector<Student> students = loadStudents();
    string roll = getText(hRoll);

    for (size_t i = 0; i < students.size(); i++)
    {
        if (students[i].roll == roll)
        {
            MessageBoxA(hwnd, "This Roll No already exists!",
                        "Warning", MB_OK | MB_ICONWARNING);
            return;
        }
    }

    Student s;
    s.roll = roll;
    s.name = getText(hName);
    s.caste = getText(hCaste);
    s.father = getText(hFather);
    s.gpa = getText(hGPA);

    students.push_back(s);

    if (saveStudents(students))
    {
        refreshCount();
        setStatus("Student added successfully", COL_OK);
    }
    else
        MessageBoxA(hwnd, "Could not save the file!",
                    "Error", MB_OK | MB_ICONERROR);
}

void updateStudent(HWND hwnd)
{
    if (!validFields(hwnd))
        return;

    vector<Student> students = loadStudents();
    string roll = getText(hRoll);
    bool found = false;

    for (size_t i = 0; i < students.size(); i++)
    {
        if (students[i].roll == roll)
        {
            students[i].name = getText(hName);
            students[i].caste = getText(hCaste);
            students[i].father = getText(hFather);
            students[i].gpa = getText(hGPA);
            found = true;
            break;
        }
    }

    if (!found)
    {
        MessageBoxA(hwnd, "Roll No not found!",
                    "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    if (saveStudents(students))
        setStatus("Student updated successfully", COL_OK);
    else
        MessageBoxA(hwnd, "Could not save the file!",
                    "Error", MB_OK | MB_ICONERROR);
}

void deleteStudent(HWND hwnd)
{
    string roll = getText(hRoll);

    if (roll.empty())
    {
        MessageBoxA(hwnd, "Enter Roll No to delete!",
                    "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    if (MessageBoxA(hwnd, "Delete this student record?",
                    "Confirm", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    vector<Student> students = loadStudents();
    vector<Student> remaining;
    bool found = false;

    for (size_t i = 0; i < students.size(); i++)
    {
        if (students[i].roll == roll)
            found = true;
        else
            remaining.push_back(students[i]);
    }

    if (!found)
    {
        MessageBoxA(hwnd, "Roll No not found!",
                    "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    if (saveStudents(remaining))
    {
        clearFields();
        refreshCount();
        setStatus("Student deleted", COL_OK);
    }
    else
    {
        MessageBoxA(hwnd, "Could not save the file!",
                    "Error", MB_OK | MB_ICONERROR);
    }
}

void findStudent(HWND hwnd)
{
    string roll = getText(hRoll);

    if (roll.empty())
    {
        MessageBoxA(hwnd, "Enter a Roll No first!",
                    "Warning", MB_OK | MB_ICONWARNING);
        return;
    }

    vector<Student> students = loadStudents();

    for (size_t i = 0; i < students.size(); i++)
    {
        if (students[i].roll == roll)
        {
            showStudent(students[i]);
            setStatus("Student found", COL_OK);
            return;
        }
    }

    setStatus("No student with that Roll No", COL_ERR);
}

// Step through records: dir = +1 (next) or -1 (previous), wraps around.
void stepStudent(int dir)
{
    vector<Student> students = loadStudents();
    int n = (int)students.size();

    if (n == 0)
    {
        setStatus("No records saved yet", COL_ERR);
        return;
    }

    string roll = getText(hRoll);
    int cur = -1;
    for (int i = 0; i < n; i++)
        if (students[i].roll == roll) { cur = i; break; }

    int idx;
    if (cur < 0)
        idx = (dir > 0) ? 0 : n - 1;
    else
        idx = (cur + dir + n) % n;

    showStudent(students[idx]);

    char buf[64];
    wsprintfA(buf, "Record %d of %d", idx + 1, n);
    setStatus(buf, RGB(71, 85, 105));
}

// =====================================================================
//  DRAWING
// =====================================================================
void drawButton(LPDRAWITEMSTRUCT dis)
{
    int id = (int)dis->CtlID;
    COLORREF base = BTN_COLOR[id - ID_PREV];

    bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    bool hot = (id == hotButton);

    COLORREF fill = base;
    if (pressed)  fill = shade(base, -35);
    else if (hot) fill = shade(base, 22);

    FillRect(dis->hDC, &dis->rcItem, hBgBrush);

    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, fill);
    HGDIOBJ oldBrush = SelectObject(dis->hDC, brush);
    HGDIOBJ oldPen = SelectObject(dis->hDC, pen);
    RoundRect(dis->hDC, dis->rcItem.left, dis->rcItem.top,
              dis->rcItem.right, dis->rcItem.bottom, 12, 12);

    // Keyboard focus ring
    if (dis->itemState & ODS_FOCUS)
    {
        HPEN ring = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        SelectObject(dis->hDC, ring);
        SelectObject(dis->hDC, GetStockObject(NULL_BRUSH));
        RoundRect(dis->hDC, dis->rcItem.left + 3, dis->rcItem.top + 3,
                  dis->rcItem.right - 3, dis->rcItem.bottom - 3, 8, 8);
        SelectObject(dis->hDC, pen);
        DeleteObject(ring);
    }

    SelectObject(dis->hDC, oldBrush);
    SelectObject(dis->hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    char text[64] = "";
    GetWindowTextA(dis->hwndItem, text, sizeof(text));

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, RGB(255, 255, 255));
    HGDIOBJ oldFont = SelectObject(dis->hDC, hFontButton);

    RECT r = dis->rcItem;
    if (pressed) OffsetRect(&r, 0, 1);
    DrawTextA(dis->hDC, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dis->hDC, oldFont);
}

struct LabelInfo { const char* text; int x, y; };

void paintWindow(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT client;
    GetClientRect(hwnd, &client);
    int w = client.right, h = client.bottom;

    // ---- Double buffer ----
    HDC dc = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ oldBmp = SelectObject(dc, bmp);

    // Background + subtle dot texture
    FillRect(dc, &client, hBgBrush);
    COLORREF bgDot = shade(COL_BG, -12);
    for (int y = HEADER_H + 10; y < h - 28; y += 14)
        for (int x = ((y / 14) % 2) * 7 + 4; x < w; x += 14)
            SetPixelV(dc, x, y, bgDot);

    // ---- Gradient header with dot texture ----
    HBRUSH dcBrush = (HBRUSH)GetStockObject(DC_BRUSH);
    for (int x = 0; x < w; x += 4)
    {
        SetDCBrushColor(dc, lerpColor(COL_HEAD_A, COL_HEAD_B, x, w));
        RECT strip = { x, 0, x + 4, HEADER_H };
        FillRect(dc, &strip, dcBrush);
    }
    for (int y = 8; y < HEADER_H - 2; y += 12)
    {
        for (int x = ((y / 12) % 2) * 6 + 4; x < w; x += 12)
        {
            COLORREF dot = shade(lerpColor(COL_HEAD_A, COL_HEAD_B, x, w), 26);
            SetPixelV(dc, x, y, dot);
            SetPixelV(dc, x + 1, y, dot);
        }
    }

    // Accent strip under header
    SetDCBrushColor(dc, COL_ACCENT);
    RECT accent = { 0, HEADER_H, w, HEADER_H + 3 };
    FillRect(dc, &accent, dcBrush);

    // Badge + title text
    {
        HBRUSH bb = CreateSolidBrush(COL_ACCENT);
        HGDIOBJ ob = SelectObject(dc, bb);
        HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
        Ellipse(dc, 24, 17, 68, 61);
        SelectObject(dc, ob);
        SelectObject(dc, op);
        DeleteObject(bb);

        SetBkMode(dc, TRANSPARENT);
        SelectObject(dc, hFontBadge);
        SetTextColor(dc, RGB(255, 255, 255));
        RECT br = { 24, 17, 68, 61 };
        DrawTextA(dc, "SR", -1, &br, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(dc, hFontTitle);
        SetTextColor(dc, RGB(255, 255, 255));
        TextOutA(dc, 84, 13, "Student Record Management", 25);

        SelectObject(dc, hFontSub);
        SetTextColor(dc, COL_SUBTITLE);
        TextOutA(dc, 85, 49, "Add, find, update and delete student records", 44);
    }

    // ---- Card with soft shadow ----
    {
        HGDIOBJ op = SelectObject(dc, GetStockObject(NULL_PEN));
        HBRUSH sb = CreateSolidBrush(COL_SHADOW);
        HGDIOBJ ob = SelectObject(dc, sb);
        RoundRect(dc, 22, 106, w - 18, 330, 16, 16);
        SelectObject(dc, ob);
        DeleteObject(sb);

        HPEN cp = CreatePen(PS_SOLID, 1, COL_BORDER);
        SelectObject(dc, cp);
        SelectObject(dc, hCardBrush);
        RoundRect(dc, 20, 100, w - 20, 324, 16, 16);
        SelectObject(dc, op);
        DeleteObject(cp);
    }

    // ---- Labels ----
    LabelInfo labels[] = {
        { "STUDENT NAME",  COL_L, 116 }, { "FATHER NAME", COL_R, 116 },
        { "STUDENT CASTE", COL_L, 184 }, { "ROLL NO",     COL_R, 184 },
        { "STUDENT GPA",   COL_L, 252 }
    };
    SetBkMode(dc, TRANSPARENT);
    SelectObject(dc, hFontLabel);
    SetTextColor(dc, COL_LABEL);
    for (int i = 0; i < 5; i++)
        TextOutA(dc, labels[i].x, labels[i].y, labels[i].text,
                 (int)strlen(labels[i].text));

    // Tip text in the free slot
    {
        SelectObject(dc, hFontSub);
        SetTextColor(dc, RGB(148, 163, 184));
        RECT tip = { COL_R, 266, COL_R + FIELD_W, 310 };
        DrawTextA(dc, "Tip: type a Roll No and press Enter\nto find a student quickly.",
                  -1, &tip, DT_LEFT | DT_WORDBREAK);
    }

    // ---- Input field frames (accent border when focused) ----
    HWND edits[] = { hName, hFather, hCaste, hRoll, hGPA };
    HWND focus = GetFocus();
    for (int i = 0; i < 5; i++)
    {
        RECT er;
        GetWindowRect(edits[i], &er);
        POINT tl = { er.left, er.top }, br = { er.right, er.bottom };
        ScreenToClient(hwnd, &tl);
        ScreenToClient(hwnd, &br);

        bool focused = (focus == edits[i]);
        HPEN fp = CreatePen(PS_SOLID, focused ? 2 : 1,
                            focused ? COL_ACCENT : RGB(203, 213, 225));
        HGDIOBJ op = SelectObject(dc, fp);
        HGDIOBJ ob = SelectObject(dc, hInputBrush);
        RoundRect(dc, tl.x - 10, tl.y - 6, br.x + 10, br.y + 6, 10, 10);
        SelectObject(dc, op);
        SelectObject(dc, ob);
        DeleteObject(fp);
    }

    // ---- Status bar ----
    {
        SetDCBrushColor(dc, COL_STATUSBG);
        RECT sr = { 0, h - 28, w, h };
        FillRect(dc, &sr, dcBrush);
        SetDCBrushColor(dc, RGB(203, 213, 225));
        RECT line = { 0, h - 28, w, h - 27 };
        FillRect(dc, &line, dcBrush);

        SelectObject(dc, hFontStatus);

        SetTextColor(dc, statusColor);
        RECT lr = { 20, h - 27, w / 2, h };
        DrawTextA(dc, statusMsg.c_str(), -1, &lr,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        char cnt[48];
        wsprintfA(cnt, "Records: %d", recordCount);
        SetTextColor(dc, RGB(71, 85, 105));
        RECT rr = { w / 2, h - 27, w - 20, h };
        DrawTextA(dc, cnt, -1, &rr, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    BitBlt(hdc, 0, 0, w, h, dc, 0, 0, SRCCOPY);

    SelectObject(dc, oldBmp);
    DeleteObject(bmp);
    DeleteDC(dc);
    EndPaint(hwnd, &ps);
}

// =====================================================================
//  CONTROL FACTORIES
// =====================================================================
HWND makeEdit(HWND parent, int x, int labelY)
{
    int frameTop = labelY + 20;
    HWND h = CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
        x + 10, frameTop + 6, FIELD_W - 20, FIELD_H - 12,
        parent, NULL, NULL, NULL);
    SendMessageA(h, WM_SETFONT, (WPARAM)hFontInput, TRUE);
    SendMessageA(h, EM_LIMITTEXT, 60, 0);
    return h;
}

HWND makeButton(HWND parent, const char* text, int id, int x)
{
    HWND h = CreateWindowA("BUTTON", text,
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_OWNERDRAW,
        x, 342, 76, 40, parent, (HMENU)(INT_PTR)id, NULL, NULL);
    SendMessageA(h, WM_SETFONT, (WPARAM)hFontButton, TRUE);
    return h;
}

// =====================================================================
//  WINDOW PROCEDURE
// =====================================================================
LRESULT CALLBACK WindowProcedure(
    HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            g_hwnd = hwnd;

            hName   = makeEdit(hwnd, COL_L, 116);
            hFather = makeEdit(hwnd, COL_R, 116);
            hCaste  = makeEdit(hwnd, COL_L, 184);
            hRoll   = makeEdit(hwnd, COL_R, 184);
            hGPA    = makeEdit(hwnd, COL_L, 252);

            const char* names[7] = { "Prev", "Next", "Find", "Add",
                                     "Update", "Delete", "Clear" };
            for (int i = 0; i < 7; i++)
                makeButton(hwnd, names[i], ID_PREV + i, 20 + i * 84);

            recordCount = (int)loadStudents().size();
            SetTimer(hwnd, 1, 60, NULL);   // hover tracking
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            paintWindow(hwnd);
            return 0;

        case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wp;
            SetTextColor(hdc, COL_TEXT);
            SetBkColor(hdc, COL_INPUT);
            return (LRESULT)hInputBrush;
        }

        case WM_DRAWITEM:
            drawButton((LPDRAWITEMSTRUCT)lp);
            return TRUE;

        case WM_TIMER:
        {
            POINT p;
            GetCursorPos(&p);
            HWND h = WindowFromPoint(p);
            int id = 0;
            if (h && GetParent(h) == hwnd)
            {
                int cid = GetDlgCtrlID(h);
                if (cid >= ID_PREV && cid <= ID_CLEAR) id = cid;
            }
            if (id != hotButton)
            {
                int old = hotButton;
                hotButton = id;
                if (old) InvalidateRect(GetDlgItem(hwnd, old), NULL, FALSE);
                if (id)  InvalidateRect(GetDlgItem(hwnd, id), NULL, FALSE);
            }
            return 0;
        }

        case WM_SETCURSOR:
        {
            HWND h = (HWND)wp;
            if (h && GetParent(h) == hwnd)
            {
                int cid = GetDlgCtrlID(h);
                if (cid >= ID_PREV && cid <= ID_CLEAR)
                {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            // Edit focus changes -> redraw the field frames
            if (lp && (HIWORD(wp) == EN_SETFOCUS || HIWORD(wp) == EN_KILLFOCUS))
            {
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            switch (LOWORD(wp))
            {
                case IDOK:      findStudent(hwnd);   break;  // Enter key
                case ID_PREV:   stepStudent(-1);     break;
                case ID_NEXT:   stepStudent(+1);     break;
                case ID_FIND:   findStudent(hwnd);   break;
                case ID_ADD:    addStudent(hwnd);    break;
                case ID_UPDATE: updateStudent(hwnd); break;
                case ID_DELETE: deleteStudent(hwnd); break;
                case ID_CLEAR:
                    clearFields();
                    setStatus("Fields cleared", RGB(71, 85, 105));
                    SetFocus(hName);
                    break;
            }
            return 0;
        }

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wp, lp);
}

// =====================================================================
//  ENTRY POINT
// =====================================================================
HFONT makeFont(int height, int weight, const char* face)
{
    return CreateFontA(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE,
                   LPSTR, int nCmdShow)
{
    hBgBrush    = CreateSolidBrush(COL_BG);
    hCardBrush  = CreateSolidBrush(COL_CARD);
    hInputBrush = CreateSolidBrush(COL_INPUT);

    hFontInput  = makeFont(-17, FW_NORMAL,   "Segoe UI");
    hFontLabel  = makeFont(-12, FW_SEMIBOLD, "Segoe UI");
    hFontTitle  = makeFont(-26, FW_SEMIBOLD, "Segoe UI");
    hFontSub    = makeFont(-13, FW_NORMAL,   "Segoe UI");
    hFontButton = makeFont(-14, FW_SEMIBOLD, "Segoe UI");
    hFontStatus = makeFont(-13, FW_NORMAL,   "Segoe UI");
    hFontBadge  = makeFont(-17, FW_BOLD,     "Segoe UI");

    const char CLASS_NAME[] = "StudentRecordWindow";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProcedure;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = NULL;               // we paint everything ourselves
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc))
        return 1;

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                  WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    RECT rc = { 0, 0, CLIENT_W, CLIENT_H };
    AdjustWindowRect(&rc, style, FALSE);

    HWND hwnd = CreateWindowA(
        CLASS_NAME,
        "Student Record Management System",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL);

    if (!hwnd)
        return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    SetFocus(hName);

    MSG msg = {};

    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        // Enables Tab navigation and Enter key handling
        if (!IsDialogMessageA(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    DeleteObject(hBgBrush);
    DeleteObject(hCardBrush);
    DeleteObject(hInputBrush);
    DeleteObject(hFontInput);
    DeleteObject(hFontLabel);
    DeleteObject(hFontTitle);
    DeleteObject(hFontSub);
    DeleteObject(hFontButton);
    DeleteObject(hFontStatus);
    DeleteObject(hFontBadge);

    return (int)msg.wParam;
}
