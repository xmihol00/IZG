/******************************************************************************
 * Laborator 01 - Zaklady pocitacove grafiky - IZG
 * ihulik@fit.vutbr.cz
 *
 * $Id: xmihol00
 * 
 * Popis: Hlavicky funkci pro funkce studentu
 *
 * Opravy a modifikace:
 * - ibobak@fit.vutbr.cz, orderedDithering
 */

#include "student.h"
#include "globals.h"

#include <time.h>

#define TRASHOLD 128

const int M[] = {
    0, 204, 51, 255,
    68, 136, 187, 119,
    34, 238, 17, 221,
    170, 102, 153, 85
};

const int M_SIDE = 4;

/******************************************************************************
 ******************************************************************************
 Funkce vraci pixel z pozice x, y. Je nutne hlidat frame_bufferu, pokud 
 je dana souradnice mimo hranice, funkce vraci barvu (0, 0, 0).
 Ukol za 0.25 bodu */
S_RGBA getPixel(int x, int y)
{
    if (x < width && y < height && x >= 0 && y >= 0)
	{
		return frame_buffer[y * width + x];
	}
    
    return COLOR_BLACK; //vraci barvu (0, 0, 0)
}
/******************************************************************************
 ******************************************************************************
 Funkce vlozi pixel na pozici x, y. Je nutne hlidat frame_bufferu, pokud 
 je dana souradnice mimo hranice, funkce neprovadi zadnou zmenu.
 Ukol za 0.25 bodu */
void putPixel(int x, int y, S_RGBA color)
{
    if (x < width && y < height && x >= 0 && y >= 0)
	{
		frame_buffer[y * width + x] = color;
	}
}
/******************************************************************************
 ******************************************************************************
 Funkce prevadi obrazek na odstiny sedi. Vyuziva funkce GetPixel a PutPixel.
 Ukol za 0.5 bodu */
void grayScale()
{
	S_RGBA pixel;
	unsigned char gray;
    for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixel = getPixel(j, i);
			gray = (unsigned char) ROUND(pixel.red * 0.299 + pixel.green * 0.587 + pixel.blue * 0.114);
			pixel.red = pixel.blue = pixel.green = gray;
			putPixel(j, i, pixel);
		}
	}
}

/******************************************************************************
 ******************************************************************************
 Funkce prevadi obrazek na cernobily pomoci algoritmu maticoveho rozptyleni.
 Ukol za 1 bod */

void orderedDithering()
{
    S_RGBA pixel;
	unsigned char gray;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixel = getPixel(j, i);
			// gray scale calculation
			gray = (unsigned char) ROUND(pixel.red * 0.299 + pixel.green * 0.587 + pixel.blue * 0.114);

			//if (gray > M[(i % M_SIDE) * M_SIDE + (j % M_SIDE)]) // using modulo, same result as below
			if (gray > M[(i & 0b11) * M_SIDE + (j & 0b11)]) // faster but only for M_SIDE == 2^n, in this case 4 
			{
				putPixel(j, i, COLOR_WHITE);
			}
			else
			{
				putPixel(j, i, COLOR_BLACK);
			}
		}
	}
}

/******************************************************************************
 ******************************************************************************
 Funkce prevadi obrazek na cernobily pomoci algoritmu distribuce chyby.
 Ukol za 1 bod */
void errorDistribution()
{   
    int err;
	int correction;
	S_RGBA pixel;

	grayScale();

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			pixel = getPixel(j, i);

			if (pixel.green <= TRASHOLD)
			{
				err = pixel.green;
				putPixel(j, i, COLOR_BLACK);
			}
			else
			{
				err = pixel.green - COLOR_WHITE.green;
				putPixel(j, i, COLOR_WHITE);
			}

			// next to
			pixel = getPixel(j + 1, i);
			correction = ROUND(pixel.green + err * 3.0 / 8.0);
			if (correction > 255)
			{
				correction = 255;
			}
			else if (correction < 0)
			{
				correction = 0;
			}
			pixel.green = pixel.blue = pixel.red = (unsigned char) correction;
			putPixel(j + 1, i, pixel);

			// under
			pixel = getPixel(j, i + 1);
			correction = ROUND(pixel.green + err * 3.0 / 8.0);
			if (correction > 255)
			{
				correction = 255;
			}
			else if (correction < 0)
			{
				correction = 0;
			}
			pixel.green = pixel.blue = pixel.red = (unsigned char) correction;
			putPixel(j, i + 1, pixel);

			// diagonal
			pixel = getPixel(j + 1, i + 1);
			correction = ROUND(pixel.green + err * 0.25);
			if (correction > 255)
			{
				correction = 255;
			}
			else if (correction < 0)
			{
				correction = 0;
			}
			pixel.green = pixel.blue = pixel.red = (unsigned char) correction;
			putPixel(j + 1, i + 1, pixel);
		}
	}
}

/******************************************************************************
 ******************************************************************************
 Funkce prevadi obrazek na cernobily pomoci metody prahovani.
 Demonstracni funkce */
void thresholding(int Threshold)
{
	/* Prevedeme obrazek na grayscale */
	grayScale();

	/* Projdeme vsechny pixely obrazku */
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			/* Nacteme soucasnou barvu */
			S_RGBA color = getPixel(x, y);

			/* Porovname hodnotu cervene barevne slozky s prahem.
			   Muzeme vyuzit jakoukoli slozku (R, G, B), protoze
			   obrazek je sedotonovy, takze R=G=B */
			if (color.red > Threshold)
				putPixel(x, y, COLOR_WHITE);
			else
				putPixel(x, y, COLOR_BLACK);
		}
}

/******************************************************************************
 ******************************************************************************
 Funkce prevadi obrazek na cernobily pomoci nahodneho rozptyleni. 
 Vyuziva funkce GetPixel, PutPixel a GrayScale.
 Demonstracni funkce. */
void randomDithering()
{
	/* Prevedeme obrazek na grayscale */
	grayScale();

	/* Inicializace generatoru pseudonahodnych cisel */
	srand((unsigned int)time(NULL));

	/* Projdeme vsechny pixely obrazku */
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			/* Nacteme soucasnou barvu */
			S_RGBA color = getPixel(x, y);
			
			/* Porovname hodnotu cervene barevne slozky s nahodnym prahem.
			   Muzeme vyuzit jakoukoli slozku (R, G, B), protoze
			   obrazek je sedotonovy, takze R=G=B */
			if (color.red > rand()%255)
			{
				putPixel(x, y, COLOR_WHITE);
			}
			else
				putPixel(x, y, COLOR_BLACK);
		}
}
/*****************************************************************************/
/*****************************************************************************/