/**
 * @file        student.cpp
 * @author      Ladislav Mosner, VUT FIT Brno, imosner@fit.vutbr.cz
 * @author      Petr Kleparnik, VUT FIT Brno, ikleparnik@fit.vutbr.cz
 * @author      Kamil Behun, VUT FIT Brno, ibehun@fit.vutbr.cz
 * @date        11.03.2018
 *
 * @brief       Deklarace funkci studentu. DOPLNUJI STUDENTI
 *
 */

// student: xmihol00, David Mihola

#include "base.h"
#include "student.h"
#include "globals.h"
#include <math.h>
#include <vector>
#include <tuple>
#include <algorithm>
#include <iostream>

/**
 * @brief Vraci barvu pixelu z pozice [x, y]
 * @param[in] x X souradnice pixelu
 * @param[in] y Y souradnice pixelu
 * @return Barva pixelu na pozici [x, y] ve formatu RGBA
 */
RGBA getPixel(int x, int y)
{
    if (x >= width || y >= height || x < 0 || y < 0) {
        IZG_ERROR("Pristup do framebufferu mimo hranice okna\n");
    }
    return framebuffer[y * width + x];
}

/**
 * @brief Nastavi barvu pixelu na pozici [x, y]
 * @param[in] x X souradnice pixelu
 * @param[in] y Y souradnice pixelu
 * @param[in] color Barva pixelu ve formatu RGBA
 */
void putPixel(int x, int y, RGBA color)
{
    if (x >= width || y >= height || x < 0 || y < 0) {
        IZG_ERROR("Pristup do framebufferu mimo hranice okna\n");
    }
    framebuffer[y * width + x] = color;
}

/**
 * @brief Vykresli usecku se souradnicemi [x1, y1] a [x2, y2]
 * @param[in] x1 X souradnice 1. bodu usecky
 * @param[in] y1 Y souradnice 1. bodu usecky
 * @param[in] x2 X souradnice 2. bodu usecky
 * @param[in] y2 Y souradnice 2. bodu usecky
 * @param[in] color Barva pixelu usecky ve formatu RGBA
 * @param[in] arrow Priznak pro vykresleni sipky (orientace hrany)
 */
void drawLine(int x1, int y1, int x2, int y2, RGBA color, bool arrow = false)
{

    if (arrow) {
        // Sipka na konci hrany
        double vx1 = x2 - x1;
        double vy1 = y2 - y1;
        double length = sqrt(vx1 * vx1 + vy1 * vy1);
        double vx1N = vx1 / length;
        double vy1N = vy1 / length;
        double vx1NN = -vy1N;
        double vy1NN = vx1N;
        int w = 3;
        int h = 10;
        int xT = (int) (x2 + w * vx1NN - h * vx1N);
        int yT = (int) (y2 + w * vy1NN - h * vy1N);
        int xB = (int) (x2 - w * vx1NN - h * vx1N);
        int yB = (int) (y2 - w * vy1NN - h * vy1N);
        pinedaTriangle(Point(x2, y2), Point(xT, yT), Point(xB, yB), color, color, false);
    }

    bool steep = abs(y2 - y1) > abs(x2 - x1);

    if (steep) {
        SWAP(x1, y1);
        SWAP(x2, y2);
    }

    if (x1 > x2) {
        SWAP(x1, x2);
        SWAP(y1, y2);
    }

    const int dx = x2 - x1, dy = abs(y2 - y1);
    const int P1 = 2 * dy, P2 = P1 - 2 * dx;
    int P = 2 * dy - dx;
    int y = y1;
    int ystep = 1;
    if (y1 > y2) ystep = -1;

    for (int x = x1; x <= x2; x++) {
        if (steep) {
            if (y >= 0 && y < width && x >= 0 && x < height) {
                putPixel(y, x, color);
            }
        } else {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                putPixel(x, y, color);
            }
        }

        if (P >= 0) {
            P += P2;
            y += ystep;
        } else {
            P += P1;
        }
    }
}

static std::pair<int, int> extreme(std::vector<Point> points, bool(*comparer)(int, int))
{
    if (points.size() == 0)
    {
        return {0, 0};
        //std::cerr << "Cannot find minimum\n";
        //throw "Cannot find minimum.";
    }

    int x_min = points[0].x;
    int y_min = points[0].y;

    for (size_t i = 1; i < points.size(); i++)
    {
        if (comparer(points[i].x, x_min))
        {
            x_min = points[i].x;
        }

        if (comparer(points[i].y, y_min))
        {
            y_min = points[i].y;
        }
    }

    return {x_min, y_min};
}

/**
 * @brief Vyplni a vykresli trojuhelnik
 * @param[in] v1 Prvni bod trojuhelniku
 * @param[in] v2 Druhy bod trojuhelniku
 * @param[in] v3 Treti bod trojuhelniku
 * @param[in] color1 Barva vyplne trojuhelniku
 * @param[in] color2 Barva hranice trojuhelniku
 * @param[in] arrow Priznak pro vykresleni sipky (orientace hrany)
 *
 * SPOLECNY UKOL. Doplnuji studenti se cvicicim.
 */
void pinedaTriangle(const Point &v1, const Point &v2, const Point &v3, const RGBA &color1, const RGBA &color2, bool arrow)
{
    // Nalezeni obalky (minX, maxX), (minY, maxY) trojuhleniku.
    auto points = std::vector<Point>{v1, v2, v3};
    int x_min, y_min, x_max, y_max;
    std::tie(x_min, y_min) = extreme(points, [](int a, int b){return a < b;});
    std::tie(x_max, y_max) = extreme(points, [](int a, int b){return a > b;});
    
    // Oriznuti obalky (minX, maxX, minY, maxY) trojuhleniku podle rozmeru okna.
    x_min = MAX(x_min, 0);
    y_min = MAX(y_min, 0);

    x_max = MIN(x_max, width - 1);
    y_max = MIN(y_max, height - 1);


    // Spocitani parametru hranove funkce (deltaX, deltaY) pro kazdou hranu.
	// Hodnoty deltaX, deltaY jsou souradnicemi vektoru, ktery ma pocatek
	// v prvnim vrcholu hrany, konec v druhem vrcholu.
	// Vypocet prvnotni hodnoty hranove funkce.

    // edge = A*x + B*y + C
    // smerovy vektor (x2 - x1, y2 - y1) --> normalovy (-(y2 - y1), x2 - x1) --> (y1 - y2, x2 - x1)
    // A = y1 - y2, B = x2 - x1, C = x1 * y2 - x2 * y1
    int A1, A2, A3;
    int B1, B2, B3;
    int a_edge{(A1 = (v1.y - v2.y)) * x_min + (B1 = (v2.x - v1.x)) * y_min + v1.x * v2.y - v2.x * v1.y};
    int b_edge{(A2 = (v2.y - v3.y)) * x_min + (B2 = (v3.x - v2.x)) * y_min + v2.x * v3.y - v3.x * v2.y};
    int c_edge{(A3 = (v3.y - v1.y)) * x_min + (B3 = (v1.x - v3.x)) * y_min + v3.x * v1.y - v1.x * v3.y};

    // Vyplnovani: Cyklus pres vsechny body (x, y) v obdelniku (minX, minY), (maxX, maxY).
    // Pro aktualizaci hodnot hranove funkce v bode P (x +/- 1, y) nebo P (x, y +/- 1)
    // vyuzijte hodnoty hranove funkce E (x, y) z bodu P (x, y).

    // vzdy skoci na zacatek noveho radku 
    /*for (int y = y_min; y < y_max; y++)
    {
        int edge1{a_edge};
        int edge2{b_edge};
        int edge3{c_edge};

        for (int x = x_min; x < x_max; x++)
        {
            if (edge1 > 0 && edge2 > 0 && edge3 > 0)
            {
                putPixel(x, y, color1);
            }

            edge1 += A1;
            edge2 += A2;
            edge3 += A3;
        }

        a_edge += B1;
        b_edge += B2;
        c_edge += B3;
    }*/

    // zig zag prochazeni prostoru
    int iter{1};
    for (int y = y_min; y <= y_max; y++)
    {
        for (int x = x_min; x != x_max; x += iter)
        {
            if (a_edge >= 0 && b_edge >= 0 && c_edge >= 0)
            {
                putPixel(x, y, color1);
            }

            a_edge += A1;
            b_edge += A2;
            c_edge += A3;
        }

        SWAP(x_min, x_max);
        iter = -iter;
        A1 = -A1;
        A2 = -A2;
        A3 = -A3;
        a_edge += B1;
        b_edge += B2;
        c_edge += B3;
    }

    // Prekresleni hranic trojuhelniku barvou color2.
    drawLine(v1.x, v1.y, v2.x, v2.y, color2, arrow);
    drawLine(v2.x, v2.y, v3.x, v3.y, color2, arrow);
    drawLine(v3.x, v3.y, v1.x, v1.y, color2, arrow);
}

/**
 * @brief Vyplni a vykresli polygon
 * @param[in] points Pole bodu polygonu
 * @param[in] size Pocet bodu polygonu (velikost pole "points")
 * @param[in] color1 Barva vyplne polygonu
 * @param[in] color2 Barva hranice polygonu
 *
 * SAMOSTATNY BODOVANY UKOL. Doplnuji pouze studenti.
 */
void pinedaPolygon(const Point *points, const int size, const RGBA &color1, const RGBA &color2)
{
    // Pri praci muzete vyuzit pro vas predpripravene datove typy z base.h., napriklad:
    //
    //      Pro ukladani parametru hranovych funkci muzete vyuzit prichystany vektor parametru hranovych funkci "EdgeParams":
    //
    //          EdgeParams edgeParams(size)                         // Vytvorite vektor (pole) "edgeParams" parametru hranovych funkci o velikosti "size".
    //          edgeParams[i].deltaX, edgeParams[i].deltaY          // Pristup k parametrum (deltaX, deltaY) hranove funkce v poli "edgeParams" na indexu "i".
    //
    //      Pro ukladani hodnot hranovych funkci muzete vyuzit prichystany vektor hodnot hranovych funkci "EdgeFncValues":
    //
    //          EdgeFncValues edgeFncValues(size)                   // Vytvorite vektor (pole) "edgeFncValues" hodnot hranovych funkci o velikosti "size".
    //          edgeFncValues[i]                                    // Pristup k hodnote hranove funkce v poli "edgeFncValues" na indexu "i".
    //

    // Nalezeni obalky (minX, maxX), (minY, maxY) polygonu.
    int x_min, y_min, x_max, y_max;
    std::tie(x_min, y_min) = extreme(std::vector<Point>{points, points + size}, [](int a, int b){return a < b;});
    std::tie(x_max, y_max) = extreme(std::vector<Point>{points, points + size}, [](int a, int b){return a > b;});

    // Oriznuti obalky (minX, maxX), (minY, maxY) polygonu podle rozmeru okna
    x_min = MAX(x_min, 0);
    y_min = MAX(y_min, 0);

    x_max = MIN(x_max, width - 1);
    y_max = MIN(y_max, height - 1);


    // Spocitani parametru (deltaX, deltaY) hranove funkce pro kazdou hranu.
	// Hodnoty deltaX, deltaY jsou souradnicemi vektoru, ktery ma pocatek
	// v prvnim vrcholu hrany, konec v druhem vrcholu.
	// Vypocet prvnotnich hodnot hranovych funkci pro jednotlive hrany.

    EdgeParams edge_prms(size);
    EdgeFncValues edge_fnc_vals(size);
    for (int i = 0; i < size; i++)
    {
        edge_prms[i].deltaX = points[i].y - points[(i + 1) % size].y;
        edge_prms[i].deltaY = points[(i + 1) % size].x - points[i].x;
        edge_fnc_vals[i] = edge_prms[i].deltaX * x_min + edge_prms[i].deltaY * y_min -
                           edge_prms[i].deltaX * points[i].x - edge_prms[i].deltaY * points[i].y;
    }


    // Test konvexnosti polygonu    
    /*if (size > 3)
    {
        bool sign{false};
        int cross_product;

        for (int i = 0; i < size; i++)
        {
            cross_product = (points[(i + 2) % size].x - points[(i + 1) % size].x) *
                            (points[i].y -              points[(i + 1) % size].y) -
                            (points[i].x -              points[(i + 1) % size].x) *
                            (points[(i + 2) % size].y - points[(i + 1) % size].y);
            
            if (i == 0)
            {
                sign = cross_product > 0;
            }
            else if (sign != (cross_product > 0))
            {
                return;
            }
        }
    }*/


    // Vyplnovani: Cyklus pres vsechny body (x, y) v obdelniku (minX, minY), (maxX, maxY).
    // Pro aktualizaci hodnot hranove funkce v bode P (x +/- 1, y) nebo P (x, y +/- 1)
    // vyuzijte hodnoty hranove funkce E (x, y) z bodu P (x, y) */

    // vzdy skoci na zacatek noveho radku radek
    /*for (int y = y_min; y < y_max; y++)
    {
        EdgeFncValues line_efv = edge_fnc_vals;
        for (int x = x_min; x < x_max; x++)
        {
            if (std::all_of(line_efv.begin(), line_efv.end(), [](int x){return x > 0; }))
            {
                putPixel(x, y, color1);
            }

            for (int i = 0; i < size; i++)
            {
                line_efv[i] += edge_prms[i].deltaX;
            }
        }

        for (int i = 0; i < size; i++)
        {
            edge_fnc_vals[i] += edge_prms[i].deltaY;
        }
    }*/

    // zig zag prochazeni oblasti
    int iter{1};
    for (int y = y_min; y <= y_max; y++)
    {
        for (int x = x_min; x != x_max; x += iter)
        {
            if (std::all_of(edge_fnc_vals.begin(), edge_fnc_vals.end(), [](int x){return x >= 0; }))
            {
                putPixel(x, y, color1);
            }

            for (int i = 0; i < size ; i++)
            {
                edge_fnc_vals[i] += edge_prms[i].deltaX;
            }
        }

        SWAP(x_max, x_min);
        iter = -iter;

        for (int i = 0; i < size; i++)
        {
            edge_prms[i].deltaX = -edge_prms[i].deltaX;
            edge_fnc_vals[i] += edge_prms[i].deltaY;
        }
    }

    // Prekresleni hranic polygonu barvou color2.
    for (int i = 0; i < size; i++) {
        drawLine(points[i].x, points[i].y, points[(i + 1) % size].x, points[(i + 1) % size].y, color2/*, true*/);
    }
}
